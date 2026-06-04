#include <gtest/gtest.h>
#include "utils/JwtUtil.hpp"

using Yachiyo::Utils::JwtUtil;

class JwtUtilTest : public ::testing::Test {
protected:
    void SetUp() override {
        jwtUtil = std::make_unique<JwtUtil>("test-secret-key-for-unit-tests", 24);
    }

    std::unique_ptr<JwtUtil> jwtUtil;
};

// ==================== Token 生成测试 ====================

TEST_F(JwtUtilTest, GenerateToken_NotEmpty) {
    std::string token = jwtUtil->generateToken(1, "testuser", "user");
    EXPECT_FALSE(token.empty());
}

TEST_F(JwtUtilTest, GenerateToken_HasThreeParts) {
    std::string token = jwtUtil->generateToken(1, "testuser", "user");
    // JWT 格式: header.payload.signature
    int dotCount = 0;
    for (char c : token) {
        if (c == '.') dotCount++;
    }
    EXPECT_EQ(dotCount, 2);
}

TEST_F(JwtUtilTest, GenerateToken_DifferentUsers) {
    std::string token1 = jwtUtil->generateToken(1, "user1", "user");
    std::string token2 = jwtUtil->generateToken(2, "user2", "admin");
    EXPECT_NE(token1, token2);
}

// ==================== Token 验证测试 ====================

TEST_F(JwtUtilTest, VerifyToken_ValidToken) {
    std::string token = jwtUtil->generateToken(42, "alice", "admin");
    auto [valid, error] = jwtUtil->verifyToken(token);
    EXPECT_TRUE(valid) << "Error: " << error;
    // verifyToken 成功时返回 success message，非空串
}

TEST_F(JwtUtilTest, VerifyToken_InvalidToken) {
    auto [valid, error] = jwtUtil->verifyToken("invalid.token.here");
    EXPECT_FALSE(valid);
    EXPECT_FALSE(error.empty());
}

TEST_F(JwtUtilTest, VerifyToken_EmptyToken) {
    auto [valid, error] = jwtUtil->verifyToken("");
    EXPECT_FALSE(valid);
}

TEST_F(JwtUtilTest, VerifyToken_WrongSecret) {
    std::string token = jwtUtil->generateToken(1, "user", "role");
    
    JwtUtil otherJwt("different-secret-key", 24);
    auto [valid, error] = otherJwt.verifyToken(token);
    EXPECT_FALSE(valid);
}

// ==================== Claims 提取测试 ====================

TEST_F(JwtUtilTest, GetUserIdFromToken) {
    std::string token = jwtUtil->generateToken(42, "alice", "admin");
    int64_t userId = jwtUtil->getUserIdFromToken(token);
    EXPECT_EQ(userId, 42);
}

TEST_F(JwtUtilTest, GetUsernameFromToken) {
    std::string token = jwtUtil->generateToken(1, "bob", "user");
    std::string username = jwtUtil->getUsernameFromToken(token);
    EXPECT_EQ(username, "bob");
}

TEST_F(JwtUtilTest, GetRoleFromToken) {
    std::string token = jwtUtil->generateToken(1, "charlie", "moderator");
    std::string role = jwtUtil->getRoleFromToken(token);
    EXPECT_EQ(role, "moderator");
}

TEST_F(JwtUtilTest, GetUserIdFromToken_InvalidToken) {
    int64_t userId = jwtUtil->getUserIdFromToken("garbage");
    EXPECT_EQ(userId, 0);
}

TEST_F(JwtUtilTest, GetClaimsFromToken) {
    std::string token = jwtUtil->generateToken(99, "dave", "admin");
    auto claims = jwtUtil->getClaimsFromToken(token);
    EXPECT_FALSE(claims.empty());
}

// ==================== Token 刷新测试 ====================

TEST_F(JwtUtilTest, RefreshToken_ValidToken) {
    std::string token = jwtUtil->generateToken(1, "user", "role");
    std::string newToken = jwtUtil->refreshToken(token);
    EXPECT_FALSE(newToken.empty());
    EXPECT_NE(token, newToken);  // 新 token 应与旧的不同（因为时间戳不同）
}

TEST_F(JwtUtilTest, RefreshToken_InvalidToken) {
    std::string newToken = jwtUtil->refreshToken("invalid");
    EXPECT_TRUE(newToken.empty());
}

TEST_F(JwtUtilTest, RefreshToken_PreservesUserId) {
    std::string token = jwtUtil->generateToken(42, "alice", "admin");
    std::string newToken = jwtUtil->refreshToken(token);
    ASSERT_FALSE(newToken.empty());
    
    int64_t userId = jwtUtil->getUserIdFromToken(newToken);
    EXPECT_EQ(userId, 42);
}
