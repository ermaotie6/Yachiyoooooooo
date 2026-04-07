#pragma once

#include <string>
#include <map>
#include <chrono>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

namespace Yachiyo {
namespace Utils {

/**
 * @brief JWT工具类
 */
class JwtUtil {
public:
    /**
     * @brief 构造函数
     * @param secret JWT密钥
     * @param expirationHours 过期时间（小时）
     */
    JwtUtil(const std::string& secret, int expirationHours = 24);
    ~JwtUtil();

    /**
     * @brief 生成JWT令牌 (原始接口)
     * @param userId 用户ID
     * @param username 用户名
     * @param role 用户角色
     * @return JWT令牌
     */
    std::string generateToken(int64_t userId, const std::string& username, const std::string& role);

    /**
     * @brief 生成JWT令牌 (JSON payload + TTL秒数)
     * @param payload JSON载荷，应包含 user_id, username, role
     * @param ttlSeconds 令牌有效期（秒）
     * @return JWT令牌
     */
    std::string generateToken(const nlohmann::json& payload, int ttlSeconds);

    /**
     * @brief 验证JWT令牌 (返回 pair<bool, string>)
     * @param token JWT令牌
     * @return 验证结果和错误信息
     */
    std::pair<bool, std::string> verifyToken(const std::string& token);

    /**
     * @brief 验证JWT令牌并返回载荷 (返回 optional<json>)
     * @param token JWT令牌
     * @return 载荷JSON，验证失败返回 nullopt
     */
    std::optional<nlohmann::json> verifyTokenPayload(const std::string& token);

    /**
     * @brief 从令牌中获取用户ID
     * @param token JWT令牌
     * @return 用户ID，验证失败返回0
     */
    int64_t getUserIdFromToken(const std::string& token);

    /**
     * @brief 从令牌中获取用户名
     * @param token JWT令牌
     * @return 用户名，验证失败返回空字符串
     */
    std::string getUsernameFromToken(const std::string& token);

    /**
     * @brief 从令牌中获取用户角色
     * @param token JWT令牌
     * @return 用户角色，验证失败返回空字符串
     */
    std::string getRoleFromToken(const std::string& token);

    /**
     * @brief 从令牌中获取所有声明
     * @param token JWT令牌
     * @return 声明映射，验证失败返回空映射
     */
    std::map<std::string, std::string> getClaimsFromToken(const std::string& token);

    /**
     * @brief 刷新令牌
     * @param token 旧令牌
     * @return 新令牌，刷新失败返回空字符串
     */
    std::string refreshToken(const std::string& token);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace Utils
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::utils {
    using JwtUtil = Yachiyo::Utils::JwtUtil;
}