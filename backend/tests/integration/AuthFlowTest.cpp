#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "services/AuthServiceImpl.hpp"
#include "utils/HashUtil.hpp"
#include "utils/JwtUtil.hpp"
#include "utils/DatabaseUtil.hpp"
#include <memory>
#include <nlohmann/json.hpp>

using namespace yachiyo::services;
using namespace Yachiyo::Utils;
using json = nlohmann::json;

/**
 * 认证流程集成测试
 * 
 * 测试 AuthServiceImpl + JwtUtil + HashUtil 之间的协作:
 *   注册 → 登录 → 验证令牌 → 刷新令牌 → 注销
 *
 * 注意: 数据库相关操作需要 mock，因为测试环境无 PostgreSQL。
 *       主要验证 JWT + Hash 子系统的端到端正确性。
 */

// ==================== JWT + Hash 端到端测试 ====================

class AuthFlowIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        hashUtil = std::make_shared<HashUtil>();
        jwtUtil = std::make_shared<JwtUtil>();
        jwtUtil->initialize("test_jwt_secret_key_for_integration_test_only");
    }
    
    std::shared_ptr<HashUtil> hashUtil;
    std::shared_ptr<JwtUtil> jwtUtil;
};

// ==================== 密码加密 + JWT 令牌完整流程 ====================

TEST_F(AuthFlowIntegrationTest, PasswordHashAndVerify) {
    std::string password = "SecurePassword123!";
    
    // 加密密码
    std::string hashed = hashUtil->hashPassword(password);
    EXPECT_NE(hashed, password); // 密文不等于明文
    EXPECT_FALSE(hashed.empty());
    
    // 验证密码
    EXPECT_TRUE(hashUtil->verifyPassword(password, hashed));
    EXPECT_FALSE(hashUtil->verifyPassword("WrongPassword", hashed));
}

TEST_F(AuthFlowIntegrationTest, JwtTokenFullCycle) {
    // 模拟用户信息
    int64_t userId = 12345;
    std::string username = "test_user";
    std::string role = "USER";
    
    // 生成令牌
    std::string token = jwtUtil->generateToken(userId, username, role);
    EXPECT_FALSE(token.empty());
    
    // 验证令牌
    EXPECT_TRUE(jwtUtil->verifyToken(token));
    
    // 提取声明
    EXPECT_EQ(jwtUtil->getUserId(token), userId);
    EXPECT_EQ(jwtUtil->getUsername(token), username);
    EXPECT_EQ(jwtUtil->getRole(token), role);
}

TEST_F(AuthFlowIntegrationTest, JwtTokenRefreshCycle) {
    int64_t userId = 67890;
    std::string username = "another_user";
    std::string role = "ADMIN";
    
    // 生成原始令牌
    std::string originalToken = jwtUtil->generateToken(userId, username, role);
    ASSERT_FALSE(originalToken.empty());
    ASSERT_TRUE(jwtUtil->verifyToken(originalToken));
    
    // 刷新令牌
    std::string refreshedToken = jwtUtil->refreshToken(originalToken);
    ASSERT_FALSE(refreshedToken.empty());
    
    // 刷新后的令牌应该有效
    EXPECT_TRUE(jwtUtil->verifyToken(refreshedToken));
    
    // 刷新后的令牌保留原始声明
    EXPECT_EQ(jwtUtil->getUserId(refreshedToken), userId);
    EXPECT_EQ(jwtUtil->getUsername(refreshedToken), username);
    EXPECT_EQ(jwtUtil->getRole(refreshedToken), role);
    
    // 新令牌与旧令牌不同（过期时间不同）
    EXPECT_NE(originalToken, refreshedToken);
}

TEST_F(AuthFlowIntegrationTest, DifferentUsersGetDifferentTokens) {
    std::string token1 = jwtUtil->generateToken(1, "user1", "USER");
    std::string token2 = jwtUtil->generateToken(2, "user2", "USER");
    std::string token3 = jwtUtil->generateToken(3, "admin1", "ADMIN");
    
    EXPECT_NE(token1, token2);
    EXPECT_NE(token2, token3);
    
    EXPECT_EQ(jwtUtil->getUserId(token1), 1);
    EXPECT_EQ(jwtUtil->getUserId(token2), 2);
    EXPECT_EQ(jwtUtil->getUserId(token3), 3);
    
    EXPECT_EQ(jwtUtil->getRole(token1), "USER");
    EXPECT_EQ(jwtUtil->getRole(token3), "ADMIN");
}

TEST_F(AuthFlowIntegrationTest, InvalidTokenRejection) {
    // 空令牌
    EXPECT_FALSE(jwtUtil->verifyToken(""));
    
    // 伪造令牌
    EXPECT_FALSE(jwtUtil->verifyToken("fake.token.here"));
    
    // 篡改令牌（修改 payload）
    std::string validToken = jwtUtil->generateToken(1, "user1", "USER");
    std::string tamperedToken = validToken;
    if (tamperedToken.size() > 10) {
        tamperedToken[tamperedToken.size() / 2] = 'X'; // 改变中间字符
    }
    EXPECT_FALSE(jwtUtil->verifyToken(tamperedToken));
}

// ==================== 密码安全性测试 ====================

TEST_F(AuthFlowIntegrationTest, PasswordHashIsSalted) {
    std::string password = "SamePassword123!";
    
    // 同一密码两次加密应该产生不同的哈希值（因为 salt 不同）
    std::string hash1 = hashUtil->hashPassword(password);
    std::string hash2 = hashUtil->hashPassword(password);
    
    // 两个哈希值不同（因为 salt 随机）
    EXPECT_NE(hash1, hash2);
    
    // 但两个都可以验证通过
    EXPECT_TRUE(hashUtil->verifyPassword(password, hash1));
    EXPECT_TRUE(hashUtil->verifyPassword(password, hash2));
}

TEST_F(AuthFlowIntegrationTest, PasswordWithSpecialCharacters) {
    std::vector<std::string> passwords = {
        "P@ssw0rd!#$%^&*()",
        "密码测试123",
        "パスワード🔐",
        "very long password that exceeds typical length requirements and includes spaces",
        "a"  // 最短密码（应该能加密，验证是服务层逻辑）
    };
    
    for (const auto& password : passwords) {
        std::string hashed = hashUtil->hashPassword(password);
        EXPECT_FALSE(hashed.empty()) << "Failed to hash: " << password;
        EXPECT_TRUE(hashUtil->verifyPassword(password, hashed)) << "Failed to verify: " << password;
    }
}

// ==================== JWT + Hash 组合流程测试 ====================

TEST_F(AuthFlowIntegrationTest, SimulateRegistrationLoginFlow) {
    // 模拟注册: 用户提交密码 → 加密存储
    std::string rawPassword = "MySecurePass123!";
    std::string storedHash = hashUtil->hashPassword(rawPassword);
    
    // 模拟登录: 用户提交密码 → 验证 → 生成 JWT
    ASSERT_TRUE(hashUtil->verifyPassword(rawPassword, storedHash));
    
    int64_t userId = 42;
    std::string username = "test_user";
    std::string role = "USER";
    
    // 登录成功 → 生成令牌
    std::string accessToken = jwtUtil->generateToken(userId, username, role);
    EXPECT_FALSE(accessToken.empty());
    
    // 模拟后续请求: 验证令牌 → 提取用户信息
    ASSERT_TRUE(jwtUtil->verifyToken(accessToken));
    EXPECT_EQ(jwtUtil->getUserId(accessToken), userId);
    EXPECT_EQ(jwtUtil->getUsername(accessToken), username);
    EXPECT_EQ(jwtUtil->getRole(accessToken), role);
    
    // 模拟令牌刷新
    std::string newToken = jwtUtil->refreshToken(accessToken);
    ASSERT_FALSE(newToken.empty());
    ASSERT_TRUE(jwtUtil->verifyToken(newToken));
    EXPECT_EQ(jwtUtil->getUserId(newToken), userId);
    
    // 模拟错误密码登录
    EXPECT_FALSE(hashUtil->verifyPassword("WrongPassword!", storedHash));
}

TEST_F(AuthFlowIntegrationTest, MultipleRoleTokens) {
    // 不同角色的令牌
    std::string userToken = jwtUtil->generateToken(1, "user1", "USER");
    std::string adminToken = jwtUtil->generateToken(2, "admin1", "ADMIN");
    std::string modToken = jwtUtil->generateToken(3, "mod1", "MODERATOR");
    
    // 各自角色正确
    EXPECT_EQ(jwtUtil->getRole(userToken), "USER");
    EXPECT_EQ(jwtUtil->getRole(adminToken), "ADMIN");
    EXPECT_EQ(jwtUtil->getRole(modToken), "MODERATOR");
}
