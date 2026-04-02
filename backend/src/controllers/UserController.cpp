#include "../../include/controllers/UserController.hpp"
#include "../../include/services/UserService.hpp"
#include "../../include/utils/JsonUtils.hpp"
#include "../../include/utils/LogUtils.hpp"
#include <crow.h>

namespace yachiyo::controllers {

UserController::UserController() {
    logger = LogUtils::getLogger("UserController");
    userService = std::make_shared<services::UserServiceImpl>();
}

void UserController::registerRoutes(crow::SimpleApp& app) {
    // 获取用户列表（管理员）
    CROW_ROUTE(app, "/api/v1/admin/users")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            return this->getUsers(req);
        });

    // 获取单个用户信息
    CROW_ROUTE(app, "/api/v1/admin/users/<string>")
        .methods("GET"_method)
        ([this](const crow::request& req, const std::string& userId) {
            return this->getUserById(req, userId);
        });

    // 创建用户（管理员）
    CROW_ROUTE(app, "/api/v1/admin/users")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            return this->createUser(req);
        });

    // 更新用户信息（管理员）
    CROW_ROUTE(app, "/api/v1/admin/users/<string>")
        .methods("PUT"_method)
        ([this](const crow::request& req, const std::string& userId) {
            return this->updateUser(req, userId);
        });

    // 删除用户（管理员）
    CROW_ROUTE(app, "/api/v1/admin/users/<string>")
        .methods("DELETE"_method)
        ([this](const crow::request& req, const std::string& userId) {
            return this->deleteUser(req, userId);
        });

    // 更新用户角色（管理员）
    CROW_ROUTE(app, "/api/v1/admin/users/<string>/role")
        .methods("PUT"_method)
        ([this](const crow::request& req, const std::string& userId) {
            return this->updateUserRole(req, userId);
        });

    // 禁用/启用用户（管理员）
    CROW_ROUTE(app, "/api/v1/admin/users/<string>/status")
        .methods("PUT"_method)
        ([this](const crow::request& req, const std::string& userId) {
            return this->updateUserStatus(req, userId);
        });

    // 获取用户统计信息（管理员）
    CROW_ROUTE(app, "/api/v1/admin/users/stats")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            return this->getUserStats(req);
        });

    logger->info("用户控制器路由已注册");
}

crow::response UserController::getUsers(const crow::request& req) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 验证管理员权限
        if (!userService->isAdmin(token)) {
            return utils::JsonUtils::createErrorResponse(403, "权限不足，需要管理员权限");
        }

        // 获取查询参数
        int page = 1;
        if (req.url_params.get("page")) {
            page = std::stoi(req.url_params.get("page"));
        }

        int limit = 20;
        if (req.url_params.get("limit")) {
            limit = std::stoi(req.url_params.get("limit"));
        }

        std::string search = req.url_params.get("search") ? req.url_params.get("search") : "";
        std::string role = req.url_params.get("role") ? req.url_params.get("role") : "";
        bool activeOnly = req.url_params.get("active_only") ? 
                         std::string(req.url_params.get("active_only")) == "true" : false;

        auto result = userService->getUsers(token, page, limit, search, role, activeOnly);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "获取用户列表成功";
        response["data"]["page"] = page;
        response["data"]["limit"] = limit;
        response["data"]["total"] = result.total;
        response["data"]["total_pages"] = result.totalPages;
        response["data"]["users"] = crow::json::wvalue::list();
        
        for (size_t i = 0; i < result.users.size(); ++i) {
            crow::json::wvalue user;
            user["id"] = result.users[i].id;
            user["username"] = result.users[i].username;
            user["email"] = result.users[i].email;
            user["role"] = result.users[i].role;
            user["avatar"] = result.users[i].avatar;
            user["bio"] = result.users[i].bio;
            user["is_active"] = result.users[i].isActive;
            user["is_verified"] = result.users[i].isVerified;
            user["created_at"] = result.users[i].createdAt;
            user["updated_at"] = result.users[i].updatedAt;
            user["last_login"] = result.users[i].lastLogin;
            
            // 添加统计信息
            user["stats"]["total_chats"] = result.users[i].stats.totalChats;
            user["stats"]["total_messages"] = result.users[i].stats.totalMessages;
            user["stats"]["total_tokens"] = result.users[i].stats.totalTokens;
            user["stats"]["last_active"] = result.users[i].stats.lastActive;
            
            response["data"]["users"][i] = user;
        }
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("获取用户列表失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response UserController::getUserById(const crow::request& req, const std::string& userId) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 检查权限：管理员可以查看任何用户，普通用户只能查看自己
        bool isAdmin = userService->isAdmin(token);
        std::string currentUserId = userService->getUserIdFromToken(token);
        
        if (!isAdmin && currentUserId != userId) {
            return utils::JsonUtils::createErrorResponse(403, "权限不足，只能查看自己的信息");
        }

        auto result = userService->getUserById(token, userId);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(404, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "获取用户信息成功";
        response["data"]["id"] = result.user.id;
        response["data"]["username"] = result.user.username;
        response["data"]["email"] = result.user.email;
        response["data"]["role"] = result.user.role;
        response["data"]["avatar"] = result.user.avatar;
        response["data"]["bio"] = result.user.bio;
        response["data"]["is_active"] = result.user.isActive;
        response["data"]["is_verified"] = result.user.isVerified;
        response["data"]["created_at"] = result.user.createdAt;
        response["data"]["updated_at"] = result.user.updatedAt;
        response["data"]["last_login"] = result.user.lastLogin;
        
        // 如果是管理员或查看自己，显示更多信息
        if (isAdmin || currentUserId == userId) {
            response["data"]["stats"]["total_chats"] = result.user.stats.totalChats;
            response["data"]["stats"]["total_messages"] = result.user.stats.totalMessages;
            response["data"]["stats"]["total_tokens"] = result.user.stats.totalTokens;
            response["data"]["stats"]["last_active"] = result.user.stats.lastActive;
        }
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("获取用户信息失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response UserController::createUser(const crow::request& req) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 验证管理员权限
        if (!userService->isAdmin(token)) {
            return utils::JsonUtils::createErrorResponse(403, "权限不足，需要管理员权限");
        }

        auto json = crow::json::load(req.body);
        if (!json) {
            return utils::JsonUtils::createErrorResponse(400, "无效的JSON数据");
        }

        std::string username = json["username"].s();
        std::string email = json["email"].s();
        std::string password = json["password"].s();
        std::string role = json.has("role") ? json["role"].s() : "user";
        std::string avatar = json.has("avatar") ? json["avatar"].s() : "";
        std::string bio = json.has("bio") ? json["bio"].s() : "";
        
        if (username.empty() || email.empty() || password.empty()) {
            return utils::JsonUtils::createErrorResponse(400, "用户名、邮箱和密码不能为空");
        }

        if (password.length() < 8) {
            return utils::JsonUtils::createErrorResponse(400, "密码长度至少为8个字符");
        }

        // 验证角色
        if (role != "user" && role != "admin" && role != "moderator") {
            return utils::JsonUtils::createErrorResponse(400, "无效的用户角色");
        }

        auto result = userService->createUser(token, username, email, password, role, avatar, bio);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "创建用户成功";
        response["data"]["id"] = result.userId;
        response["data"]["username"] = username;
        response["data"]["email"] = email;
        response["data"]["role"] = role;
        response["data"]["avatar"] = avatar;
        response["data"]["bio"] = bio;
        response["data"]["created_at"] = result.createdAt;
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("创建用户失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response UserController::updateUser(const crow::request& req, const std::string& userId) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 检查权限：管理员可以更新任何用户，普通用户只能更新自己
        bool isAdmin = userService->isAdmin(token);
        std::string currentUserId = userService->getUserIdFromToken(token);
        
        if (!isAdmin && currentUserId != userId) {
            return utils::JsonUtils::createErrorResponse(403, "权限不足，只能更新自己的信息");
        }

        auto json = crow::json::load(req.body);
        if (!json) {
            return utils::JsonUtils::createErrorResponse(400, "无效的JSON数据");
        }

        std::string email = json.has("email") ? json["email"].s() : "";
        std::string avatar = json.has("avatar") ? json["avatar"].s() : "";
        std::string bio = json.has("bio") ? json["bio"].s() : "";
        
        // 普通用户不能通过这个接口更新角色
        std::string role = "";
        if (isAdmin && json.has("role")) {
            role = json["role"].s();
        }

        auto result = userService->updateUser(token, userId, email, avatar, bio, role);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "更新用户信息成功";
        response["data"]["id"] = userId;
        response["data"]["email"] = result.email;
        response["data"]["avatar"] = result.avatar;
        response["data"]["bio"] = result.bio;
        if (!role.empty()) {
            response["data"]["role"] = result.role;
        }
        response["data"]["updated_at"] = result.updatedAt;
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("更新用户信息失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response UserController::deleteUser(const crow::request& req, const std::string& userId) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 验证管理员权限
        if (!userService->isAdmin(token)) {
            return utils::JsonUtils::createErrorResponse(403, "权限不足，需要管理员权限");
        }

        // 不能删除自己
        std::string currentUserId = userService->getUserIdFromToken(token);
        if (currentUserId == userId) {
            return utils::JsonUtils::createErrorResponse(400, "不能删除自己的账户");
        }

        auto result = userService->deleteUser(token, userId);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "删除用户成功";
        response["data"]["deleted_user_id"] = userId;
        response["data"]["deleted_at"] = result.deletedAt;
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("删除用户失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response UserController::updateUserRole(const crow::request& req, const std::string& userId) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 验证管理员权限
        if (!userService->isAdmin(token)) {
            return utils::JsonUtils::createErrorResponse(403, "权限不足，需要管理员权限");
        }

        auto json = crow::json::load(req.body);
        if (!json) {
            return utils::JsonUtils::createErrorResponse(400, "无效的JSON数据");
        }

        std::string role = json["role"].s();
        
        if (role.empty()) {
            return utils::JsonUtils::createErrorResponse(400, "角色不能为空");
        }

        // 验证角色
        if (role != "user" && role != "admin" && role != "moderator") {
            return utils::JsonUtils::createErrorResponse(400, "无效的用户角色");
        }

        auto result = userService->updateUserRole(token, userId, role);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "更新用户角色成功";
        response["data"]["user_id"] = userId;
        response["data"]["new_role"] = role;
        response["data"]["updated_at"] = result.updatedAt;
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("更新用户角色失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response UserController::updateUserStatus(const crow::request& req, const std::string& userId) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 验证管理员权限
        if (!userService->isAdmin(token)) {
            return utils::JsonUtils::createErrorResponse(403, "权限不足，需要管理员权限");
        }

        auto json = crow::json::load(req.body);
        if (!json) {
            return utils::JsonUtils::createErrorResponse(400, "无效的JSON数据");
        }

        bool isActive = json["is_active"].b();
        
        auto result = userService->updateUserStatus(token, userId, isActive);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = isActive ? "启用用户成功" : "禁用用户成功";
        response["data"]["user_id"] = userId;
        response["data"]["is_active"] = isActive;
        response["data"]["updated_at"] = result.updatedAt;
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("更新用户状态失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response UserController::getUserStats(const crow::request& req) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 验证管理员权限
        if (!userService->isAdmin(token)) {
            return utils::JsonUtils::createErrorResponse(403, "权限不足，需要管理员权限");
        }

        // 获取时间范围参数
        std::string period = req.url_params.get("period") ? req.url_params.get("period") : "day";
        int days = 7;
        if (req.url_params.get("days")) {
            days = std::stoi(req.url_params.get("days"));
        }

        auto result = userService->getUserStats(token, period, days);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "获取用户统计信息成功";
        response["data"]["total_users"] = result.totalUsers;
        response["data"]["active_users"] = result.activeUsers;
        response["data"]["new_users_today"] = result.newUsersToday;
        response["data"]["new_users_week"] = result.newUsersWeek;
        response["data"]["new_users_month"] = result.newUsersMonth;
        response["data"]["user_growth_rate"] = result.userGrowthRate;
        response["data"]["avg_messages_per_user"] = result.avgMessagesPerUser;
        response["data"]["avg_tokens_per_user"] = result.avgTokensPerUser;
        
        // 添加用户分布
        response["data"]["distribution"]["by_role"] = crow::json::wvalue::object();
        for (const auto& [role, count] : result.roleDistribution) {
            response["data"]["distribution"]["by_role"][role] = count;
        }
        
        response["data"]["distribution"]["by_status"] = crow::json::wvalue::object();
        response["data"]["distribution"]["by_status"]["active"] = result.activeUsers;
        response["data"]["distribution"]["by_status"]["inactive"] = result.totalUsers - result.activeUsers;
        
        // 添加时间序列数据
        response["data"]["time_series"] = crow::json::wvalue::list();
        for (size_t i = 0; i < result.timeSeries.size(); ++i) {
            crow::json::wvalue point;
            point["date"] = result.timeSeries[i].date;
            point["new_users"] = result.timeSeries[i].newUsers;
            point["active_users"] = result.timeSeries[i].activeUsers;
            point["total_messages"] = result.timeSeries[i].totalMessages;
            point["total_tokens"] = result.timeSeries[i].totalTokens;
            response["data"]["time_series"][i] = point;
        }
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("获取用户统计信息失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

} // namespace yachiyo::controllers