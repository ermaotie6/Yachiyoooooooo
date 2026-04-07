#include <gtest/gtest.h>
#include "utils/HashUtil.hpp"

using Yachiyo::Utils::HashUtil;

class HashUtilTest : public ::testing::Test {
protected:
    HashUtil hashUtil;
};

// ==================== SHA256 测试 ====================

TEST_F(HashUtilTest, SHA256_BasicString) {
    std::string hash = hashUtil.sha256("hello");
    // SHA256("hello") 是已知值
    EXPECT_EQ(hash.length(), 64u);  // SHA256 hex 输出长度
    EXPECT_FALSE(hash.empty());
}

TEST_F(HashUtilTest, SHA256_EmptyString) {
    std::string hash = hashUtil.sha256("");
    EXPECT_EQ(hash.length(), 64u);
}

TEST_F(HashUtilTest, SHA256_Deterministic) {
    std::string hash1 = hashUtil.sha256("test_password");
    std::string hash2 = hashUtil.sha256("test_password");
    EXPECT_EQ(hash1, hash2);
}

TEST_F(HashUtilTest, SHA256_DifferentInputs) {
    std::string hash1 = hashUtil.sha256("password1");
    std::string hash2 = hashUtil.sha256("password2");
    EXPECT_NE(hash1, hash2);
}

// ==================== 密码哈希测试 ====================

TEST_F(HashUtilTest, HashPassword_NotEmpty) {
    std::string hashed = hashUtil.hashPassword("myPassword123");
    EXPECT_FALSE(hashed.empty());
}

TEST_F(HashUtilTest, HashPassword_DifferentFromPlainText) {
    std::string password = "myPassword123";
    std::string hashed = hashUtil.hashPassword(password);
    EXPECT_NE(password, hashed);
}

TEST_F(HashUtilTest, VerifyPassword_CorrectPassword) {
    std::string password = "securePassword!@#";
    std::string hashed = hashUtil.hashPassword(password);
    EXPECT_TRUE(hashUtil.verifyPassword(password, hashed));
}

TEST_F(HashUtilTest, VerifyPassword_WrongPassword) {
    std::string password = "securePassword!@#";
    std::string hashed = hashUtil.hashPassword(password);
    EXPECT_FALSE(hashUtil.verifyPassword("wrongPassword", hashed));
}

TEST_F(HashUtilTest, VerifyPassword_EmptyPassword) {
    std::string hashed = hashUtil.hashPassword("");
    EXPECT_TRUE(hashUtil.verifyPassword("", hashed));
}

// ==================== Unicode 测试 ====================

TEST_F(HashUtilTest, SHA256_UnicodeInput) {
    std::string hash = hashUtil.sha256("你好世界");
    EXPECT_EQ(hash.length(), 64u);
    EXPECT_FALSE(hash.empty());
}

TEST_F(HashUtilTest, HashPassword_UnicodePassword) {
    std::string password = "密码测试123";
    std::string hashed = hashUtil.hashPassword(password);
    EXPECT_TRUE(hashUtil.verifyPassword(password, hashed));
}
