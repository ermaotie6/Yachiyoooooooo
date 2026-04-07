#pragma once

#include <string>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace Yachiyo {
namespace Utils {

class HashUtil {
public:
    // 使用SHA256生成哈希
    static std::string sha256(const std::string& input);
    
    // 生成带盐的密码哈希
    static std::string hashPassword(const std::string& password);
    
    // 验证密码
    static bool verifyPassword(const std::string& password, const std::string& hashedPassword);
    
    // 生成随机盐
    static std::string generateSalt(size_t length = 16);
    
    // 生成API密钥
    static std::string generateApiKey();
    
    // 生成JWT密钥
    static std::string generateJwtSecret();
};

} // namespace Utils
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::utils {
    using HashUtil = Yachiyo::Utils::HashUtil;
}