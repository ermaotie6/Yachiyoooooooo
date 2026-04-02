#pragma once

#include "controllers/BaseController.hpp"
#include "services/UserService.hpp"
#include "dto/UserDTO.hpp"
#include <crow.h>
#include <memory>

namespace Yachiyo {
namespace controllers {

class UserController : public BaseController {
public:
    explicit UserController(std::shared_ptr<services::UserService> userService = nullptr);
    
    void registerRoutes(crow::SimpleApp& app) override;
    
private:
    std::shared_ptr<services::UserService> userService;
    
    // 获取用户信息
    crow::response getUserInfo(const crow::request& req);
    
    // 更新用户信息
    crow::response updateUserInfo(const crow::request& req);
    
    // 获取用户列表（仅管理员）
    crow::response getUserList(const crow::request& req);
    
    // 更新用户角色（仅管理员）
    crow::response updateUserRole(const crow::request& req);
    
    // 删除用户（仅管理员）
    crow::response deleteUser(const crow::request& req);
};

} // namespace controllers
} // namespace Yachiyo