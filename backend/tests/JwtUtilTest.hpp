#pragma once

#include <gtest/gtest.h>
#include <string>
#include <memory>
#include "../include/utils/JwtUtil.hpp"
#include "../include/utils/RedisUtil.hpp"
#include "../include/services/AuthService.hpp"

namespace Yachiyo {
namespace tests {

/**
 * @brief JWT 工具单元测试
 */
class JwtUtilTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        // 在每个测试前执行
        secretKey = "test_secret_key_for_jwt_testing_12345";
    }

    virtual void TearDown() {
        // 在每个测试后执行
    }

    std::string secretKey;
};

/**
 * @brief JWT 令牌生成测试
 */
TEST_F(JwtUtilTest, GenerateTokenSuccess) {
    std::string userId = "12345";
    std::string username = "testuser";
    std::string role = "user";
    int expiresIn = 86400; // 24 小时

    auto token = Utils::JwtUtil::generateToken(userId, username, role, expiresIn);

    EXPECT_FALSE(token.empty());
    EXPECT_GT(token.length(), 0);
    // JWT 格式: header.payload.signature (包含两个点)
    EXPECT_EQ(std::count(token.begin(), token.end(), '.'), 2);
}

/**
 * @brief JWT 令牌验证测试 - 有效令牌
 */
TEST_F(JwtUtilTest, VerifyTokenValid) {
    std::string userId = "12345";
    std::string username = "testuser";
    std::string role = "user";

    auto token = Utils::JwtUtil::generateToken(userId, username, role, 86400);
    auto claims = Utils::JwtUtil::verifyToken(token);

    EXPECT_TRUE(claims.has_value());
    EXPECT_EQ(claims->at("userId"), userId);
    EXPECT_EQ(claims->at("username"), username);
    EXPECT_EQ(claims->at("role"), role);
}

/**
 * @brief JWT 令牌验证测试 - 无效令牌
 */
TEST_F(JwtUtilTest, VerifyTokenInvalid) {
    std::string invalidToken = "invalid.token.here";
    auto claims = Utils::JwtUtil::verifyToken(invalidToken);

    EXPECT_FALSE(claims.has_value());
}

/**
 * @brief JWT 提取用户ID测试
 */
TEST_F(JwtUtilTest, GetUserIdFromToken) {
    std::string userId = "99999";
    auto token = Utils::JwtUtil::generateToken(userId, "user", "user", 86400);
    auto extractedId = Utils::JwtUtil::getUserIdFromToken(token);

    EXPECT_TRUE(extractedId.has_value());
    EXPECT_EQ(extractedId.value(), userId);
}

/**
 * @brief JWT 提取用户名测试
 */
TEST_F(JwtUtilTest, GetUsernameFromToken) {
    std::string username = "john_doe";
    auto token = Utils::JwtUtil::generateToken("123", username, "user", 86400);
    auto extractedName = Utils::JwtUtil::getUsernameFromToken(token);

    EXPECT_TRUE(extractedName.has_value());
    EXPECT_EQ(extractedName.value(), username);
}

/**
 * @brief JWT 刷新令牌测试
 */
TEST_F(JwtUtilTest, RefreshToken) {
    auto oldToken = Utils::JwtUtil::generateToken("123", "user", "user", 3600);
    auto newToken = Utils::JwtUtil::refreshToken(oldToken);

    EXPECT_FALSE(newToken.empty());
    EXPECT_NE(oldToken, newToken);
    
    // 新令牌应该也是有效的
    auto claims = Utils::JwtUtil::verifyToken(newToken);
    EXPECT_TRUE(claims.has_value());
}

} // namespace tests
} // namespace Yachiyo
