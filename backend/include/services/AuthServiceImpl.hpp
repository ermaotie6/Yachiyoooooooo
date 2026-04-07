#pragma once

#include "AuthService.hpp"
#include "../utils/HashUtil.hpp"
#include "../utils/JwtUtil.hpp"
#include "../utils/DatabaseUtil.hpp"
#include <memory>

namespace yachiyo::services {

/**
 * @brief 认证服务实现
 */
class AuthServiceImpl : public IAuthService {
private:
    std::shared_ptr<Yachiyo::Utils::DatabaseUtil> dbUtil;
    std::shared_ptr<Yachiyo::Utils::JwtUtil> jwtUtil;
    std::shared_ptr<Yachiyo::Utils::HashUtil> hashUtil;
    
public:
    explicit AuthServiceImpl(
        std::shared_ptr<Yachiyo::Utils::DatabaseUtil> db,
        std::shared_ptr<Yachiyo::Utils::JwtUtil> jwt,
        std::shared_ptr<Yachiyo::Utils::HashUtil> hash
    ) : dbUtil(db), jwtUtil(jwt), hashUtil(hash) {}
    
    // ==================== 实现接口 ====================
    
    Result<std::shared_ptr<User>> registerUser(
        const std::string& username,
        const std::string& email,
        const std::string& password
    ) override;
    
    Result<json> login(
        const std::string& username,
        const std::string& password,
        const std::string& userIp = ""
    ) override;
    
    Result<std::shared_ptr<User>> verifyToken(
        const std::string& token
    ) override;
    
    int64_t getUserIdFromToken(const std::string& token) override;
    
    UserRole getRoleFromToken(const std::string& token) override;
    
    Result<json> refreshToken(
        const std::string& refreshToken
    ) override;
    
    Result<bool> logout(
        int64_t userId,
        const std::string& refreshToken
    ) override;
    
    Result<std::shared_ptr<User>> getUserById(int64_t userId) override;
    
    Result<std::shared_ptr<User>> getUserByUsername(
        const std::string& username
    ) override;
    
    Result<bool> canUserSendMessage(int64_t userId) override;

private:
    // 辅助方法
    bool isValidUsername(const std::string& username) const;
    bool isValidEmail(const std::string& email) const;
    bool isValidPassword(const std::string& password) const;
    bool isUserBlacklisted(const std::string& identifier) const;
};

} // namespace yachiyo::services
