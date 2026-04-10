#include "utils/HashUtil.hpp"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <stdexcept>

namespace Yachiyo {
namespace Utils {

std::string HashUtil::sha256(const std::string& input) {
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

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
    for (unsigned int i = 0; i < md_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)md_value[i];
    }
    return ss.str();
}

std::pair<std::string, std::string> HashUtil::hashPassword(const std::string& password) {
    // 生成随机盐值并与密码拼接后SHA256
    std::string salt = generateSalt(16);
    std::string hash = sha256(salt + password);
    // 返回 <hash, salt> 分离存储
    return {hash, salt};
}

std::string HashUtil::hashPasswordCombined(const std::string& password) {
    auto [hash, salt] = hashPassword(password);
    // 存储格式: salt:hash
    return salt + ":" + hash;
}

bool HashUtil::verifyPassword(const std::string& password, const std::string& hashedPassword) {
    // 解析 salt:hash 格式
    auto colonPos = hashedPassword.find(':');
    if (colonPos == std::string::npos) {
        // 兼容旧格式（无盐值）
        return sha256(password) == hashedPassword;
    }
    std::string salt = hashedPassword.substr(0, colonPos);
    std::string hash = hashedPassword.substr(colonPos + 1);
    return sha256(salt + password) == hash;
}

bool HashUtil::verifyPassword(const std::string& password, const std::string& hash, const std::string& salt) {
    return sha256(salt + password) == hash;
}

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
    for (size_t i = 0; i < length; ++i) {
        salt += alphanum[dis(gen)];
    }
    return salt;
}

std::string HashUtil::generateApiKey() {
    return sha256(generateSalt(32) + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
}

std::string HashUtil::generateJwtSecret() {
    return sha256(generateSalt(64) + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
}

} // namespace Utils
} // namespace Yachiyo