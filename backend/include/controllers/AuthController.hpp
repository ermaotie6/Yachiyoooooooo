#pragma once

#include "BaseController.hpp"
#include "../services/AuthService.hpp"
#include "../utils/JwtUtil.hpp"
#include <memory>

namespace yachiyo::controllers {

using Services::IAuthService;
using Utils::JwtUtil, Utils::Result;

/**
 * @brief 认证控制器 (v2.0)
 * 
 * API端点:
 * - POST /api/auth/register - 用户注册
 * - POST /api/auth/login - 用户登录
 * - POST /api/auth/refresh - 刷新令牌
 * - POST /api/auth/logout - 用户注销
 * - GET /api/auth/profile - 获取用户资料 (需要认证)
 */
class AuthController : public BaseController {
private:
    std::shared_ptr<IAuthService> authService;
    std::shared_ptr<JwtUtil> jwtUtil;
    
public:
    AuthController(
        std::shared_ptr<IAuthService> auth,
        std::shared_ptr<JwtUtil> jwt
    ) : authService(auth), jwtUtil(jwt) {}
    
    // ==================== 认证端点 ====================
    
    /**
     * @brief 用户注册
     */
    void registerUser(const crow::request& req, crow::response& res);
    
    /**
     * @brief 用户登录
     */
    void login(const crow::request& req, crow::response& res);
    
    /**
     * @brief 刷新令牌
     */
    void refreshToken(const crow::request& req, crow::response& res);
    
    /**
     * @brief 用户注销
     */
    void logout(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取用户资料 (需要JWT认证)
     */
    void getProfile(const crow::request& req, crow::response& res);
    
    /**
     * @brief 更新用户资料
     */
    void updateProfile(const crow::request& req, crow::response& res);
};

} // namespace yachiyo::controllers
