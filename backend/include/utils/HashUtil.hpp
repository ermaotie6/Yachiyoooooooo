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
    
    /**
     * @brief 生成带盐的密码哈希
     * @param password 明文密码
     * @return pair<hash, salt> — hash为SHA256结果，salt为随机盐值
     * 
     * 数据库应分开存储 hash 和 salt 两个字段。
     * 也可以用 hashPasswordCombined() 获取 "salt:hash" 单字符串格式。
     */
    static std::pair<std::string, std::string> hashPassword(const std::string& password);
    
    /**
     * @brief 生成带盐的密码哈希 (合并格式 salt:hash)
     * @return 格式为 "salt:hash" 的字符串
     */
    static std::string hashPasswordCombined(const std::string& password);
    
    // 验证密码 (支持 "salt:hash" 合并格式和 hash+salt 分离格式)
    static bool verifyPassword(const std::string& password, const std::string& hashedPassword);
    static bool verifyPassword(const std::string& password, const std::string& hash, const std::string& salt);
    
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