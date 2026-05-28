#pragma once

#include <string>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace Yachiyo {
namespace Utils {

class HashUtil {
public:
    // 使用 SHA256 生成哈希（通用用途，非密码哈希）
    static std::string sha256(const std::string& input);
    
    /**
     * @brief 使用 bcrypt 生成密码哈希
     * @param password 明文密码
     * @return pair<hash, salt> — hash 为 bcrypt 结果（含内嵌盐值），salt 留空（向后兼容）
     *
     * bcrypt 哈希格式: $2b$12$<22-char-salt><31-char-hash> = 60 字符
     * cost factor=12, 盐值内嵌于哈希中
     */
    static std::pair<std::string, std::string> hashPassword(const std::string& password);
    
    /**
     * @brief 使用 bcrypt 生成密码哈希（单字符串）
     * @return bcrypt 哈希字符串（含内嵌盐值和 cost factor）
     */
    static std::string hashPasswordCombined(const std::string& password);
    
    /**
     * @brief 验证密码（支持 bcrypt + 旧版 SHA-256 兼容）
     *
     * bcrypt 哈希以 "$2b$" 或 "$2a$" 开头
     * SHA-256 旧格式为 "salt:hash"
     */
    static bool verifyPassword(const std::string& password, const std::string& storedHash);
    static bool verifyPassword(const std::string& password, const std::string& hash, const std::string& salt);
    
    // 生成随机盐
    static std::string generateSalt(size_t length = 16);
    
    // 生成 API 密钥
    static std::string generateApiKey();
    
    // 生成 JWT 密钥
    static std::string generateJwtSecret();

private:
    static std::string bcryptHash(const std::string& password);
    static bool bcryptVerify(const std::string& password, const std::string& hash);
    static std::string bcryptGenerateSalt();
};

} // namespace Utils
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::utils {
    using HashUtil = Yachiyo::Utils::HashUtil;
}
