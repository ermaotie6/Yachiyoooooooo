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
    EXPECT_EQ(hash.length(), 64u);
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

// ==================== 密码哈希测试 (bcrypt, $2b$ 格式) ====================

TEST_F(HashUtilTest, HashPassword_NotEmpty) {
    auto [hash, salt] = hashUtil.hashPassword("myPassword123");
    EXPECT_FALSE(hash.empty());
    // salt 可能为空 — bcrypt 盐值已内嵌在 hash 中
}

TEST_F(HashUtilTest, HashPassword_DifferentFromPlainText) {
    std::string password = "myPassword123";
    auto [hash, salt] = hashUtil.hashPassword(password);
    EXPECT_NE(password, hash);
}

TEST_F(HashUtilTest, HashPasswordCombined_Format) {
    std::string combined = hashUtil.hashPasswordCombined("securePassword!@#");
    EXPECT_FALSE(combined.empty());
    EXPECT_EQ(combined.substr(0, 4), "$2b$");
}

TEST_F(HashUtilTest, VerifyPassword_CorrectPassword) {
    std::string password = "securePassword!@#";
    std::string combined = hashUtil.hashPasswordCombined(password);
    EXPECT_TRUE(hashUtil.verifyPassword(password, combined));
}

TEST_F(HashUtilTest, VerifyPassword_WrongPassword) {
    std::string password = "securePassword!@#";
    std::string combined = hashUtil.hashPasswordCombined(password);
    EXPECT_FALSE(hashUtil.verifyPassword("wrongPassword", combined));
}

TEST_F(HashUtilTest, VerifyPassword_EmptyPassword) {
    std::string combined = hashUtil.hashPasswordCombined("");
    EXPECT_TRUE(hashUtil.verifyPassword("", combined));
}

// ==================== Unicode 测试 ====================

TEST_F(HashUtilTest, SHA256_UnicodeInput) {
    std::string hash = hashUtil.sha256("你好世界");
    EXPECT_EQ(hash.length(), 64u);
    EXPECT_FALSE(hash.empty());
}

TEST_F(HashUtilTest, HashPassword_UnicodePassword) {
    std::string password = "密码测试123";
    std::string combined = hashUtil.hashPasswordCombined(password);
    EXPECT_TRUE(hashUtil.verifyPassword(password, combined));
}
