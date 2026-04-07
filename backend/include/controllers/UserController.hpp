#pragma once

#include "BaseController.hpp"
#include "../services/UserService.hpp"
#include "../utils/JwtUtil.hpp"
#include <crow.h>
#include <memory>

namespace yachiyo::controllers {

/**
 * @brief 用户管理控制器
 * 
 * API端点 (管理员):
 * - GET    /api/v1/admin/users              - 获取用户列表
 * - GET    /api/v1/admin/users/<id>         - 获取用户详情
 * - POST   /api/v1/admin/users              - 创建用户
 * - PUT    /api/v1/admin/users/<id>         - 更新用户
 * - DELETE /api/v1/admin/users/<id>         - 删除用户
 * - PUT    /api/v1/admin/users/<id>/role    - 更新用户角色
 * - PUT    /api/v1/admin/users/<id>/status  - 启用/禁用用户
 * - GET    /api/v1/admin/users/stats        - 获取用户统计
 */
class UserController : public BaseController {
public:
    UserController();
    
    void registerRoutes(crow::SimpleApp& app);
    
private:
    std::shared_ptr<Yachiyo::services::UserServiceImpl> userService;
    
    // ==================== 辅助方法 ====================

    /** @brief 从请求头提取 Bearer Token 字符串 */
    std::string extractToken(const crow::request& req);
    /** @brief 从 token 中获取 userId（字符串），失败返回空 */
    std::string resolveUserId(const std::string& token);
    /** @brief 从 token 中获取角色，判断是否为 admin */
    bool isAdmin(const std::string& token);

    // ==================== 端点处理 ====================

    /** @brief 获取用户列表（管理员） */
    crow::response getUsers(const crow::request& req);
    /** @brief 获取单个用户信息 */
    crow::response getUserById(const crow::request& req, const std::string& userId);
    /** @brief 创建用户（管理员） */
    crow::response createUser(const crow::request& req);
    /** @brief 更新用户信息 */
    crow::response updateUser(const crow::request& req, const std::string& userId);
    /** @brief 删除用户（管理员） */
    crow::response deleteUser(const crow::request& req, const std::string& userId);
    /** @brief 更新用户角色（管理员） */
    crow::response updateUserRole(const crow::request& req, const std::string& userId);
    /** @brief 启用/禁用用户（管理员） */
    crow::response updateUserStatus(const crow::request& req, const std::string& userId);
    /** @brief 获取用户统计信息（管理员） */
    crow::response getUserStats(const crow::request& req);
};

} // namespace yachiyo::controllers