#include "utils/HashUtil.hpp"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <crypt.h>
#include <stdexcept>
#include <cstring>

namespace Yachiyo {
namespace Utils {

std::string HashUtil::sha256(const std::string& input) {
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) throw std::runtime_error("EVP_MD_CTX_new failed");

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }
    if (EVP_DigestUpdate(mdctx, input.data(), input.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("EVP_DigestUpdate failed");
    }
    if (EVP_DigestFinal_ex(mdctx, md_value, &md_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }
    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < md_len; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)md_value[i];
    return ss.str();
}

// ==================== bcrypt 实现 (基于 POSIX crypt) ====================

std::string HashUtil::bcryptGenerateSalt() {
    static const char bcryptChars[] =
        "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 63);

    std::string salt;
    salt.reserve(22);
    for (int i = 0; i < 22; ++i)
        salt += bcryptChars[dis(gen)];
    return salt;
}

std::string HashUtil::bcryptHash(const std::string& password) {
    std::string setting = "$2b$12$" + bcryptGenerateSalt();
    char* hash = crypt(password.c_str(), setting.c_str());
    if (!hash) throw std::runtime_error("bcrypt: crypt() failed");
    return std::string(hash);
}

bool HashUtil::bcryptVerify(const std::string& password, const std::string& hash) {
    char* result = crypt(password.c_str(), hash.c_str());
    return result && std::strcmp(result, hash.c_str()) == 0;
}

// ==================== 密码哈希 API ====================

std::pair<std::string, std::string> HashUtil::hashPassword(const std::string& password) {
    std::string hash = bcryptHash(password);
    return {hash, ""};  // bcrypt 盐值内嵌于 hash 中，salt 字段留空以向后兼容
}

std::string HashUtil::hashPasswordCombined(const std::string& password) {
    return bcryptHash(password);
}

bool HashUtil::verifyPassword(const std::string& password, const std::string& storedHash) {
    // bcrypt 格式检测: $2b$ 或 $2a$ 开头
    if (storedHash.rfind("$2b$", 0) == 0 || storedHash.rfind("$2a$", 0) == 0) {
        return bcryptVerify(password, storedHash);
    }

    // 旧格式兼容: salt:hash (SHA-256)
    auto colonPos = storedHash.find(':');
    if (colonPos == std::string::npos) {
        // 无盐值旧格式
        return sha256(password) == storedHash;
    }
    std::string salt = storedHash.substr(0, colonPos);
    std::string hash = storedHash.substr(colonPos + 1);
    return sha256(salt + password) == hash;
}

bool HashUtil::verifyPassword(const std::string& password, const std::string& hash, const std::string& salt) {
    // bcrypt 格式
    if (hash.rfind("$2b$", 0) == 0 || hash.rfind("$2a$", 0) == 0) {
        return bcryptVerify(password, hash);
    }
    // SHA-256 旧格式兼容
    if (!salt.empty()) {
        return sha256(salt + password) == hash;
    }
    return sha256(password) == hash;
}

// ==================== 通用工具方法 (不变) ====================

std::string HashUtil::generateSalt(size_t length) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
    std::string salt;
    salt.reserve(length);
    for (size_t i = 0; i < length; ++i)
        salt += alphanum[dis(gen)];
    return salt;
}

std::string HashUtil::generateApiKey() {
    return sha256(generateSalt(32) + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()));
}

std::string HashUtil::generateJwtSecret() {
    return sha256(generateSalt(64) + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()));
}

} // namespace Utils
} // namespace Yachiyo
