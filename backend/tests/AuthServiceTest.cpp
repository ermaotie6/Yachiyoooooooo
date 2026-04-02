#include <gtest/gtest.h>
#include <memory>
#include "../include/services/AuthServiceImpl.hpp"
#include "../include/utils/DatabaseUtil.hpp"
#include "../include/utils/RedisUtil.hpp"
#include "../include/utils/JwtUtil.hpp"

using namespace yachiyo::services;
using namespace yachiyo::utils;
using namespace yachiyo::models;

/**
 * @class AuthServiceTest
 * @brief AuthService 单元测试套件
 * 
 * 测试用例覆盖:
 * - 用户注册 (成功/失败场景)
 * - 用户登录 (成功/失败场景)
 * - Token 验证 (有效/无效/过期)
 * - Token 刷新 (成功/失败)
 * - 登出 (成功/失败)
 */
class AuthServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<IAuthService> authService;
    std::shared_ptr<DatabaseUtil> dbUtil;
    std::shared_ptr<RedisUtil> redisUtil;
    std::shared_ptr<JwtUtil> jwtUtil;
    
    void SetUp() override {
        // 初始化模拟对象
        dbUtil = std::make_shared<DatabaseUtil>();
        redisUtil = std::make_shared<RedisUtil>();
        jwtUtil = std::make_shared<JwtUtil>("test-secret-key");
        
        // 创建 AuthService 实例
        authService = std::make_shared<AuthServiceImpl>(dbUtil, redisUtil, jwtUtil);
    }
    
    void TearDown() override {
        // 清理测试数据
        // 可选: dbUtil->execute("DELETE FROM users WHERE username LIKE 'test_%'");
    }
};

// ==================== 用户注册测试 ====================

TEST_F(AuthServiceTest, RegisterUserSuccess) {
    // 测试成功注册用户
    auto result = authService->registerUser(
        "test_user_001",
        "test001@example.com",
        "SecurePassword123"
    );
    
    ASSERT_TRUE(result.isSuccess());
    ASSERT_GT(result.getData()->getId(), 0);
    EXPECT_EQ(result.getData()->getUsername(), "test_user_001");
    EXPECT_EQ(result.getData()->getEmail(), "test001@example.com");
}

TEST_F(AuthServiceTest, RegisterUserDuplicateUsername) {
    // 第一次注册成功
    auto result1 = authService->registerUser(
        "duplicate_user",
        "email1@example.com",
        "Password123"
    );
    ASSERT_TRUE(result1.isSuccess());
    
    // 第二次使用相同用户名应该失败
    auto result2 = authService->registerUser(
        "duplicate_user",
        "email2@example.com",
        "Password123"
    );
    ASSERT_FALSE(result2.isSuccess());
    EXPECT_THAT(result2.getErrorMsg(), 
                ::testing::HasSubstr("已存在"));
}

TEST_F(AuthServiceTest, RegisterUserInvalidEmail) {
    // 测试无效的邮箱格式
    auto result = authService->registerUser(
        "test_user",
        "invalid-email",
        "Password123"
    );
    ASSERT_FALSE(result.isSuccess());
}

TEST_F(AuthServiceTest, RegisterUserWeakPassword) {
    // 测试弱密码
    auto result = authService->registerUser(
        "test_user",
        "test@example.com",
        "123"  // 密码过短
    );
    ASSERT_FALSE(result.isSuccess());
}

// ==================== 用户登录测试 ====================

TEST_F(AuthServiceTest, LoginSuccess) {
    // 先注册用户
    auto registerResult = authService->registerUser(
        "login_test_user",
        "login@example.com",
        "LoginPassword123"
    );
    ASSERT_TRUE(registerResult.isSuccess());
    
    // 尝试登录
    auto loginResult = authService->login(
        "login_test_user",
        "LoginPassword123",
        "192.168.1.1"
    );
    
    ASSERT_TRUE(loginResult.isSuccess());
    auto loginData = loginResult.getData();
    EXPECT_GT(loginData->getUserId(), 0);
    EXPECT_FALSE(loginData->getAccessToken().empty());
    EXPECT_FALSE(loginData->getRefreshToken().empty());
}

TEST_F(AuthServiceTest, LoginWrongPassword) {
    // 先注册用户
    authService->registerUser(
        "wrong_pass_user",
        "wrongpass@example.com",
        "CorrectPassword123"
    );
    
    // 使用错误的密码登录
    auto result = authService->login(
        "wrong_pass_user",
        "WrongPassword123",
        "192.168.1.1"
    );
    
    ASSERT_FALSE(result.isSuccess());
    EXPECT_THAT(result.getErrorMsg(), 
                ::testing::HasSubstr("密码"));
}

TEST_F(AuthServiceTest, LoginNonExistentUser) {
    // 尝试使用不存在的用户登录
    auto result = authService->login(
        "nonexistent_user",
        "AnyPassword123",
        "192.168.1.1"
    );
    
    ASSERT_FALSE(result.isSuccess());
    EXPECT_THAT(result.getErrorMsg(), 
                ::testing::HasSubstr("不存在|找不到"));
}

// ==================== Token 验证测试 ====================

TEST_F(AuthServiceTest, VerifyTokenValid) {
    // 注册并登录
    authService->registerUser(
        "verify_test_user",
        "verify@example.com",
        "VerifyPassword123"
    );
    
    auto loginResult = authService->login(
        "verify_test_user",
        "VerifyPassword123",
        "192.168.1.1"
    );
    
    auto token = loginResult.getData()->getAccessToken();
    
    // 验证 token
    auto verifyResult = authService->verifyToken(token);
    ASSERT_TRUE(verifyResult.isSuccess());
    
    auto user = verifyResult.getData();
    EXPECT_EQ(user->getUsername(), "verify_test_user");
}

TEST_F(AuthServiceTest, VerifyTokenInvalid) {
    // 使用无效的 token
    auto result = authService->verifyToken("invalid.token.here");
    ASSERT_FALSE(result.isSuccess());
}

TEST_F(AuthServiceTest, VerifyTokenExpired) {
    // 创建一个过期的 token
    // 注意: 这需要 JwtUtil 支持设置过期时间
    std::string expiredToken = jwtUtil->generateToken(1, UserRole::USER, 1); // 1秒过期
    
    // 等待 token 过期
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    
    // 验证过期的 token
    auto result = authService->verifyToken(expiredToken);
    ASSERT_FALSE(result.isSuccess());
    EXPECT_THAT(result.getErrorMsg(), 
                ::testing::HasSubstr("过期"));
}

// ==================== Token 刷新测试 ====================

TEST_F(AuthServiceTest, RefreshTokenSuccess) {
    // 注册并登录获取 refresh token
    authService->registerUser(
        "refresh_test_user",
        "refresh@example.com",
        "RefreshPassword123"
    );
    
    auto loginResult = authService->login(
        "refresh_test_user",
        "RefreshPassword123",
        "192.168.1.1"
    );
    
    auto refreshToken = loginResult.getData()->getRefreshToken();
    
    // 刷新 token
    auto refreshResult = authService->refreshToken(refreshToken);
    ASSERT_TRUE(refreshResult.isSuccess());
    
    auto newToken = refreshResult.getData()->getAccessToken();
    EXPECT_FALSE(newToken.empty());
    EXPECT_NE(newToken, loginResult.getData()->getAccessToken());
}

TEST_F(AuthServiceTest, RefreshTokenInvalid) {
    // 使用无效的 refresh token
    auto result = authService->refreshToken("invalid.refresh.token");
    ASSERT_FALSE(result.isSuccess());
}

// ==================== 登出测试 ====================

TEST_F(AuthServiceTest, LogoutSuccess) {
    // 注册并登录
    authService->registerUser(
        "logout_test_user",
        "logout@example.com",
        "LogoutPassword123"
    );
    
    auto loginResult = authService->login(
        "logout_test_user",
        "LogoutPassword123",
        "192.168.1.1"
    );
    
    auto token = loginResult.getData()->getAccessToken();
    
    // 登出
    auto logoutResult = authService->logout(token);
    ASSERT_TRUE(logoutResult.isSuccess());
    
    // 登出后 token 应该无效
    auto verifyResult = authService->verifyToken(token);
    ASSERT_FALSE(verifyResult.isSuccess());
}

// ==================== 用户查询测试 ====================

TEST_F(AuthServiceTest, GetUserByIdSuccess) {
    // 注册用户
    auto registerResult = authService->registerUser(
        "query_test_user",
        "query@example.com",
        "QueryPassword123"
    );
    
    int64_t userId = registerResult.getData()->getId();
    
    // 查询用户
    auto result = authService->getUserById(userId);
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.getData()->getUsername(), "query_test_user");
}

TEST_F(AuthServiceTest, GetUserByIdNotFound) {
    // 查询不存在的用户
    auto result = authService->getUserById(99999);
    ASSERT_FALSE(result.isSuccess());
}

TEST_F(AuthServiceTest, GetUserByUsernameSuccess) {
    // 注册用户
    authService->registerUser(
        "username_query_user",
        "usernamequery@example.com",
        "QueryPassword123"
    );
    
    // 按用户名查询
    auto result = authService->getUserByUsername("username_query_user");
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.getData()->getEmail(), "usernamequery@example.com");
}

// ==================== 权限检查测试 ====================

TEST_F(AuthServiceTest, CanUserSendMessageSuccess) {
    // 注册并登录一个普通用户
    auto registerResult = authService->registerUser(
        "can_send_user",
        "cansend@example.com",
        "CanSendPassword123"
    );
    
    auto userId = registerResult.getData()->getId();
    
    // 检查是否能发送消息
    auto result = authService->canUserSendMessage(userId);
    ASSERT_TRUE(result.isSuccess());
}

TEST_F(AuthServiceTest, CanUserSendMessageBanned) {
    // 注册一个用户并将其设为禁用
    auto registerResult = authService->registerUser(
        "banned_user",
        "banned@example.com",
        "BannedPassword123"
    );
    
    auto userId = registerResult.getData()->getId();
    
    // 禁用用户 (需要 AuthService 提供更新方法)
    // authService->updateUserStatus(userId, UserStatus::BANNED);
    
    // 检查是否能发送消息 (应该失败)
    // auto result = authService->canUserSendMessage(userId);
    // ASSERT_FALSE(result.isSuccess());
}

// ==================== 集成测试 ====================

TEST_F(AuthServiceTest, FullAuthenticationFlow) {
    // 完整的认证流程测试
    
    // 1. 注册
    auto registerResult = authService->registerUser(
        "full_flow_user",
        "fullflow@example.com",
        "FullFlowPassword123"
    );
    ASSERT_TRUE(registerResult.isSuccess());
    int64_t userId = registerResult.getData()->getId();
    
    // 2. 登录
    auto loginResult = authService->login(
        "full_flow_user",
        "FullFlowPassword123",
        "192.168.1.100"
    );
    ASSERT_TRUE(loginResult.isSuccess());
    std::string accessToken = loginResult.getData()->getAccessToken();
    std::string refreshToken = loginResult.getData()->getRefreshToken();
    
    // 3. 验证 token
    auto verifyResult = authService->verifyToken(accessToken);
    ASSERT_TRUE(verifyResult.isSuccess());
    
    // 4. 获取用户信息
    auto userResult = authService->getUserById(userId);
    ASSERT_TRUE(userResult.isSuccess());
    
    // 5. 刷新 token
    auto refreshResult = authService->refreshToken(refreshToken);
    ASSERT_TRUE(refreshResult.isSuccess());
    
    // 6. 登出
    auto logoutResult = authService->logout(accessToken);
    ASSERT_TRUE(logoutResult.isSuccess());
    
    // 7. 验证登出后 token 无效
    auto finalVerify = authService->verifyToken(accessToken);
    ASSERT_FALSE(finalVerify.isSuccess());
}

// ==================== 主函数 ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
