#pragma once

#include "../models/User.hpp"
#include "../utils/Result.hpp"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace yachiyo::services {

using Models::User, Models::UserRole, Models::UserStatus;
using Utils::Result;

/**
 * @brief 认证服务接口 (v2.0 - 虚拟主播直播平台)
 * 
 * 功能:
 * 1. 用户注册 - 创建新用户账号
 * 2. 用户登录 - 验证用户凭证并生成JWT令牌
 * 3. 验证令牌 - 验证JWT令牌的有效性
 * 4. 刷新令牌 - 使用刷新令牌获取新的访问令牌
 * 5. 用户注销 - 使失效用户的刷新令牌
 */
class IAuthService {
public:
    virtual ~IAuthService() = default;
    
    /**
     * @brief 用户注册
     * @param username 用户名 (唯一，3-50字符)
     * @param email 邮箱 (唯一)
     * @param password 密码 (最少8字符)
     * @return Result<std::shared_ptr<User>> 创建的用户对象或错误信息
     */
    virtual Result<std::shared_ptr<User>> registerUser(
        const std::string& username,
        const std::string& email,
        const std::string& password
    ) = 0;
    
    /**
     * @brief 用户登录
     * @param username 用户名或邮箱
     * @param password 密码
     * @param userIp 用户IP (用于安全记录)
     * @return Result<json> 包含access_token, refresh_token 和用户信息的JSON
     */
    virtual Result<json> login(
        const std::string& username,
        const std::string& password,
        const std::string& userIp = ""
    ) = 0;
    
    /**
     * @brief 验证JWT令牌
     * @param token JWT令牌
     * @return Result<std::shared_ptr<User>> 验证成功时返回关联的用户
     */
    virtual Result<std::shared_ptr<User>> verifyToken(
        const std::string& token
    ) = 0;
    
    /**
     * @brief 从令牌中提取用户ID
     * @param token JWT令牌
     * @return 用户ID，如果无效返回0
     */
    virtual int64_t getUserIdFromToken(const std::string& token) = 0;
    
    /**
     * @brief 从令牌中提取用户角色
     * @param token JWT令牌
     * @return 用户角色，如果无效返回USER
     */
    virtual UserRole getRoleFromToken(const std::string& token) = 0;
    
    /**
     * @brief 刷新令牌
     * @param refreshToken 刷新令牌
     * @return Result<json> 新的访问令牌和刷新令牌
     */
    virtual Result<json> refreshToken(
        const std::string& refreshToken
    ) = 0;
    
    /**
     * @brief 用户注销 (失效刷新令牌)
     * @param userId 用户ID
     * @param refreshToken 刷新令牌
     * @return Result<bool> 操作是否成功
     */
    virtual Result<bool> logout(
        int64_t userId,
        const std::string& refreshToken
    ) = 0;
    
    /**
     * @brief 根据ID获取用户
     * @param userId 用户ID
     * @return Result<std::shared_ptr<User>> 用户对象或错误信息
     */
    virtual Result<std::shared_ptr<User>> getUserById(int64_t userId) = 0;
    
    /**
     * @brief 根据用户名获取用户
     * @param username 用户名
     * @return Result<std::shared_ptr<User>> 用户对象或错误信息
     */
    virtual Result<std::shared_ptr<User>> getUserByUsername(
        const std::string& username
    ) = 0;
    
    /**
     * @brief 验证用户是否可以发送消息
     * @param userId 用户ID
     * @return Result<bool> 是否可以发送
     */
    virtual Result<bool> canUserSendMessage(int64_t userId) = 0;
};

} // namespace yachiyo::services
