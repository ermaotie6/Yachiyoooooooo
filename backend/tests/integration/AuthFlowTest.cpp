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
 */

// ==================== JWT + Hash 端到端测试 ====================

class AuthFlowIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        hashUtil = std::make_shared<HashUtil>();
        jwtUtil = std::make_shared<JwtUtil>(
            "test_jwt_secret_key_for_integration_test_only", 3600);
    }

    std::shared_ptr<HashUtil> hashUtil;
    std::shared_ptr<JwtUtil> jwtUtil;
};

// ==================== 密码加密 + JWT 令牌完整流程 ====================

TEST_F(AuthFlowIntegrationTest, PasswordHashAndVerify) {
    std::string password = "SecurePassword123!";

    // 加密密码 (combined 格式)
    std::string hashed = hashUtil->hashPasswordCombined(password);
    EXPECT_NE(hashed, password);
    EXPECT_FALSE(hashed.empty());

    // 验证密码
    EXPECT_TRUE(hashUtil->verifyPassword(password, hashed));
    EXPECT_FALSE(hashUtil->verifyPassword("WrongPassword", hashed));
}

TEST_F(AuthFlowIntegrationTest, JwtTokenFullCycle) {
    int64_t userId = 12345;
    std::string username = "test_user";
    std::string role = "USER";

    // 生成令牌
    std::string token = jwtUtil->generateToken(userId, username, role);
    EXPECT_FALSE(token.empty());

    // 验证令牌
    auto [valid, err] = jwtUtil->verifyToken(token);
    EXPECT_TRUE(valid) << err;

    // 提取声明
    EXPECT_EQ(jwtUtil->getUserIdFromToken(token), userId);

    auto payload = jwtUtil->verifyTokenPayload(token);
    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ(payload->value("username", ""), username);
    EXPECT_EQ(payload->value("role", ""), role);
}

TEST_F(AuthFlowIntegrationTest, JwtTokenRefreshCycle) {
    int64_t userId = 67890;
    std::string username = "another_user";
    std::string role = "ADMIN";

    // 生成原始令牌
    std::string originalToken = jwtUtil->generateToken(userId, username, role);
    ASSERT_FALSE(originalToken.empty());
    auto [valid1, _] = jwtUtil->verifyToken(originalToken);
    ASSERT_TRUE(valid1);

    // 刷新令牌
    std::string refreshedToken = jwtUtil->refreshToken(originalToken);
    ASSERT_FALSE(refreshedToken.empty());

    // 刷新后的令牌应该有效
    auto [valid2, _2] = jwtUtil->verifyToken(refreshedToken);
    EXPECT_TRUE(valid2);

    // 刷新后的令牌保留原始声明
    EXPECT_EQ(jwtUtil->getUserIdFromToken(refreshedToken), userId);

    auto payload = jwtUtil->verifyTokenPayload(refreshedToken);
    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ(payload->value("username", ""), username);
    EXPECT_EQ(payload->value("role", ""), role);

    // 新令牌与旧令牌不同（过期时间不同）
    EXPECT_NE(originalToken, refreshedToken);
}

TEST_F(AuthFlowIntegrationTest, DifferentUsersGetDifferentTokens) {
    std::string token1 = jwtUtil->generateToken(1, "user1", "USER");
    std::string token2 = jwtUtil->generateToken(2, "user2", "USER");
    std::string token3 = jwtUtil->generateToken(3, "admin1", "ADMIN");

    EXPECT_NE(token1, token2);
    EXPECT_NE(token2, token3);

    EXPECT_EQ(jwtUtil->getUserIdFromToken(token1), 1);
    EXPECT_EQ(jwtUtil->getUserIdFromToken(token2), 2);
    EXPECT_EQ(jwtUtil->getUserIdFromToken(token3), 3);
}

TEST_F(AuthFlowIntegrationTest, InvalidTokenRejection) {
    // 空令牌
    auto [emptyValid, _] = jwtUtil->verifyToken("");
    EXPECT_FALSE(emptyValid);

    // 伪造令牌
    auto [fakeValid, _2] = jwtUtil->verifyToken("fake.token.here");
    EXPECT_FALSE(fakeValid);

    // 篡改令牌（修改 payload）
    std::string validToken = jwtUtil->generateToken(1, "user1", "USER");
    std::string tamperedToken = validToken;
    if (tamperedToken.size() > 10) {
        tamperedToken[tamperedToken.size() / 2] = 'X';
    }
    auto [tamperedValid, _3] = jwtUtil->verifyToken(tamperedToken);
    EXPECT_FALSE(tamperedValid);
}

// ==================== 密码安全性测试 ====================

TEST_F(AuthFlowIntegrationTest, PasswordHashIsSalted) {
    std::string password = "SamePassword123!";

    // 同一密码两次加密应该产生不同的哈希值（因为 salt 不同）
    std::string hash1 = hashUtil->hashPasswordCombined(password);
    std::string hash2 = hashUtil->hashPasswordCombined(password);

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
        "a"  // 最短密码
    };

    for (const auto& password : passwords) {
        std::string hashed = hashUtil->hashPasswordCombined(password);
        EXPECT_FALSE(hashed.empty()) << "Failed to hash: " << password;
        EXPECT_TRUE(hashUtil->verifyPassword(password, hashed))
            << "Failed to verify: " << password;
    }
}

// ==================== JWT + Hash 组合流程测试 ====================

TEST_F(AuthFlowIntegrationTest, SimulateRegistrationLoginFlow) {
    std::string rawPassword = "MySecurePass123!";

    // 模拟注册: 用户提交密码 → 加密存储 (combined 格式)
    std::string storedHash = hashUtil->hashPasswordCombined(rawPassword);

    // 模拟登录: 验证密码 → 生成 JWT
    ASSERT_TRUE(hashUtil->verifyPassword(rawPassword, storedHash));

    int64_t userId = 42;
    std::string username = "test_user";
    std::string role = "USER";

    // 登录成功 → 生成令牌
    std::string accessToken = jwtUtil->generateToken(userId, username, role);
    EXPECT_FALSE(accessToken.empty());

    // 模拟后续请求: 验证令牌 → 提取用户信息
    auto [valid, _] = jwtUtil->verifyToken(accessToken);
    ASSERT_TRUE(valid);
    EXPECT_EQ(jwtUtil->getUserIdFromToken(accessToken), userId);

    auto payload = jwtUtil->verifyTokenPayload(accessToken);
    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ(payload->value("username", ""), username);
    EXPECT_EQ(payload->value("role", ""), role);

    // 模拟令牌刷新
    std::string newToken = jwtUtil->refreshToken(accessToken);
    ASSERT_FALSE(newToken.empty());
    auto [refValid, _2] = jwtUtil->verifyToken(newToken);
    ASSERT_TRUE(refValid);
    EXPECT_EQ(jwtUtil->getUserIdFromToken(newToken), userId);

    // 模拟错误密码登录
    EXPECT_FALSE(hashUtil->verifyPassword("WrongPassword!", storedHash));
}

TEST_F(AuthFlowIntegrationTest, MultipleRoleTokens) {
    std::string userToken = jwtUtil->generateToken(1, "user1", "USER");
    std::string adminToken = jwtUtil->generateToken(2, "admin1", "ADMIN");
    std::string modToken = jwtUtil->generateToken(3, "mod1", "MODERATOR");

    auto p1 = jwtUtil->verifyTokenPayload(userToken);
    auto p2 = jwtUtil->verifyTokenPayload(adminToken);
    auto p3 = jwtUtil->verifyTokenPayload(modToken);

    ASSERT_TRUE(p1.has_value());
    ASSERT_TRUE(p2.has_value());
    ASSERT_TRUE(p3.has_value());

    EXPECT_EQ(p1->value("role", ""), "USER");
    EXPECT_EQ(p2->value("role", ""), "ADMIN");
    EXPECT_EQ(p3->value("role", ""), "MODERATOR");
}
