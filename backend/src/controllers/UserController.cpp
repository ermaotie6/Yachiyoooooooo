#include "controllers/UserController.hpp"
#include "services/UserService.hpp"
#include "utils/JsonUtils.hpp"
#include "utils/LogUtils.hpp"
#include "utils/JwtUtil.hpp"
#include "config/ConfigManager.hpp"
#include <crow.h>

namespace yachiyo::controllers {

using yachiyo::utils::LogUtils;
namespace dto = Yachiyo::dto;

// ==================== 构造 ====================

UserController::UserController() {
    logger = LogUtils::getLogger("UserController");
    userService = std::make_shared<Yachiyo::services::UserServiceImpl>();
}

// ==================== 辅助方法 ====================

std::string UserController::extractToken(const crow::request& req) {
    std::string authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0) return "";
    return authHeader.substr(7);
}

std::string UserController::resolveUserId(const std::string& token) {
    try {
        auto cfg = config::ConfigManager::getInstance();
        std::string secret = cfg->getString("jwt.secret", "yachiyo-default-secret-change-in-production");
        int exp = cfg->getInt("jwt.expiresIn", 24);
        Yachiyo::Utils::JwtUtil jwt(secret, exp);
        auto [ok, msg] = jwt.verifyToken(token);
        if (!ok) return "";
        int64_t uid = jwt.getUserIdFromToken(token);
        return uid > 0 ? std::to_string(uid) : "";
    } catch (...) { return ""; }
}

bool UserController::isAdmin(const std::string& token) {
    try {
        auto cfg = config::ConfigManager::getInstance();
        std::string secret = cfg->getString("jwt.secret", "yachiyo-default-secret-change-in-production");
        int exp = cfg->getInt("jwt.expiresIn", 24);
        Yachiyo::Utils::JwtUtil jwt(secret, exp);
        std::string role = jwt.getRoleFromToken(token);
        return role == "admin" || role == "ADMIN";
    } catch (...) { return false; }
}

// ==================== 路由注册 ====================

void UserController::registerRoutes(crow::SimpleApp& app) {
    // 获取用户列表（管理员）
    CROW_ROUTE(app, "/api/v1/admin/users")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            return this->getUsers(req);
        });

    // 用户统计信息（管理员）— 必须在 /users/<string> 之前注册
    CROW_ROUTE(app, "/api/v1/admin/users/stats")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            return this->getUserStats(req);
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

    logger->info("用户控制器路由已注册");
}

// ==================== 获取用户列表 ====================

crow::response UserController::getUsers(const crow::request& req) {
    try {
        std::string token = extractToken(req);
        if (token.empty()) return unauthorizedResponse("未提供有效的认证令牌");
        if (!isAdmin(token))  return forbiddenResponse("权限不足，需要管理员权限");

        // 查询参数
        std::string keyword;
        int page = 1, limit = 20;
        if (req.url_params.get("search"))  keyword = req.url_params.get("search");
        if (req.url_params.get("page"))    page  = std::stoi(req.url_params.get("page"));
        if (req.url_params.get("limit"))   limit = std::stoi(req.url_params.get("limit"));

        auto users = userService->searchUsers(keyword, page, limit);

        nlohmann::json usersArr = nlohmann::json::array();
        for (const auto& u : users) {
            usersArr.push_back({
                {"id",         u.id},
                {"username",   u.username},
                {"email",      u.email},
                {"role",       u.role},
                {"avatar",     u.avatarUrl},
                {"bio",        u.bio},
                {"is_active",  u.isActive},
                {"is_verified",u.isVerified}
            });
        }

        return successResponse("获取用户列表成功", {
            {"page", page}, {"limit", limit},
            {"users", usersArr}
        });

    } catch (const std::exception& e) {
        logger->error("获取用户列表失败: {}", e.what());
        return internalServerErrorResponse("服务器内部错误");
    }
}

// ==================== 获取单个用户 ====================

crow::response UserController::getUserById(const crow::request& req, const std::string& userId) {
    try {
        std::string token = extractToken(req);
        if (token.empty()) return unauthorizedResponse("未提供有效的认证令牌");

        std::string currentUserId = resolveUserId(token);
        bool admin = isAdmin(token);
        if (!admin && currentUserId != userId)
            return forbiddenResponse("权限不足，只能查看自己的信息");

        auto user = userService->getUserById(userId);

        nlohmann::json data = {
            {"id",          user.id},
            {"username",    user.username},
            {"email",       user.email},
            {"display_name",user.displayName},
            {"role",        user.role},
            {"avatar",      user.avatarUrl},
            {"bio",         user.bio},
            {"is_active",   user.isActive},
            {"is_verified", user.isVerified}
        };

        if (admin || currentUserId == userId) {
            auto stats = userService->getUserStatistics(userId);
            data["stats"] = {
                {"total_posts",     stats.totalPosts},
                {"total_likes",     stats.totalLikes},
                {"total_comments",  stats.totalComments},
                {"total_views",     stats.totalViews},
                {"total_followers", stats.totalFollowers},
                {"total_following", stats.totalFollowing}
            };
        }

        return successResponse("获取用户信息成功", data);

    } catch (const std::exception& e) {
        logger->error("获取用户信息失败: {}", e.what());
        return notFoundResponse("用户不存在");
    }
}

// ==================== 创建用户 ====================

crow::response UserController::createUser(const crow::request& req) {
    try {
        std::string token = extractToken(req);
        if (token.empty()) return unauthorizedResponse("未提供有效的认证令牌");
        if (!isAdmin(token))  return forbiddenResponse("权限不足，需要管理员权限");

        nlohmann::json body;
        if (!validateJson(req, body)) return badRequestResponse("无效的JSON数据");

        std::string username = body.value("username", "");
        std::string email    = body.value("email", "");
        std::string password = body.value("password", "");
        std::string role     = body.value("role", "user");
        std::string avatar   = body.value("avatar", "");
        std::string bio      = body.value("bio", "");

        if (username.empty() || email.empty() || password.empty())
            return badRequestResponse("用户名、邮箱和密码不能为空");
        if (password.length() < 8)
            return badRequestResponse("密码长度至少为8个字符");
        if (role != "user" && role != "admin" && role != "moderator")
            return badRequestResponse("无效的用户角色");

        // 目前 UserServiceImpl 没有 createUser，走低层 DAO
        // TODO: 在 UserServiceImpl 中封装 createUser 方法
        return badRequestResponse("创建用户功能尚未实现 (待 DAO 扩展)");

    } catch (const std::exception& e) {
        logger->error("创建用户失败: {}", e.what());
        return internalServerErrorResponse("服务器内部错误");
    }
}

// ==================== 更新用户 ====================

crow::response UserController::updateUser(const crow::request& req, const std::string& userId) {
    try {
        std::string token = extractToken(req);
        if (token.empty()) return unauthorizedResponse("未提供有效的认证令牌");

        std::string currentUserId = resolveUserId(token);
        bool admin = isAdmin(token);
        if (!admin && currentUserId != userId)
            return forbiddenResponse("权限不足，只能更新自己的信息");

        nlohmann::json body;
        if (!validateJson(req, body)) return badRequestResponse("无效的JSON数据");

        dto::UserDTO updateDTO;
        updateDTO.email       = body.value("email", "");
        updateDTO.avatarUrl   = body.value("avatar", "");
        updateDTO.bio         = body.value("bio", "");
        updateDTO.displayName = body.value("display_name", "");

        auto updated = userService->updateUser(userId, updateDTO);

        return successResponse("更新用户信息成功", {
            {"id",           updated.id},
            {"display_name", updated.displayName},
            {"email",        updated.email},
            {"avatar",       updated.avatarUrl},
            {"bio",          updated.bio},
            {"role",         updated.role}
        });

    } catch (const std::exception& e) {
        logger->error("更新用户信息失败: {}", e.what());
        return internalServerErrorResponse("服务器内部错误");
    }
}

// ==================== 删除用户 ====================

crow::response UserController::deleteUser(const crow::request& req, const std::string& userId) {
    try {
        std::string token = extractToken(req);
        if (token.empty()) return unauthorizedResponse("未提供有效的认证令牌");
        if (!isAdmin(token))  return forbiddenResponse("权限不足，需要管理员权限");

        std::string currentUserId = resolveUserId(token);
        if (currentUserId == userId)
            return badRequestResponse("不能删除自己的账户");

        bool ok = userService->deactivateUser(userId, currentUserId);
        if (!ok) return badRequestResponse("删除用户失败");

        return successResponse("删除用户成功", {{"deleted_user_id", userId}});

    } catch (const std::exception& e) {
        logger->error("删除用户失败: {}", e.what());
        return internalServerErrorResponse("服务器内部错误");
    }
}

// ==================== 更新用户角色 ====================

crow::response UserController::updateUserRole(const crow::request& req, const std::string& userId) {
    try {
        std::string token = extractToken(req);
        if (token.empty()) return unauthorizedResponse("未提供有效的认证令牌");
        if (!isAdmin(token))  return forbiddenResponse("权限不足，需要管理员权限");

        nlohmann::json body;
        if (!validateJson(req, body)) return badRequestResponse("无效的JSON数据");

        std::string role = body.value("role", "");
        if (role.empty()) return badRequestResponse("角色不能为空");
        if (role != "user" && role != "admin" && role != "moderator")
            return badRequestResponse("无效的用户角色");

        std::string adminId = resolveUserId(token);
        bool ok = userService->updateUserRole(userId, role, adminId);
        if (!ok) return badRequestResponse("更新用户角色失败");

        return successResponse("更新用户角色成功", {
            {"user_id", userId}, {"new_role", role}
        });

    } catch (const std::exception& e) {
        logger->error("更新用户角色失败: {}", e.what());
        return internalServerErrorResponse("服务器内部错误");
    }
}

// ==================== 启用/禁用用户 ====================

crow::response UserController::updateUserStatus(const crow::request& req, const std::string& userId) {
    try {
        std::string token = extractToken(req);
        if (token.empty()) return unauthorizedResponse("未提供有效的认证令牌");
        if (!isAdmin(token))  return forbiddenResponse("权限不足，需要管理员权限");

        nlohmann::json body;
        if (!validateJson(req, body)) return badRequestResponse("无效的JSON数据");

        bool activate = body.value("is_active", true);
        std::string adminId = resolveUserId(token);

        bool ok = activate
            ? userService->activateUser(userId, adminId)
            : userService->deactivateUser(userId, adminId);

        if (!ok) return badRequestResponse(activate ? "启用用户失败" : "禁用用户失败");

        return successResponse(activate ? "启用用户成功" : "禁用用户成功", {
            {"user_id", userId}, {"is_active", activate}
        });

    } catch (const std::exception& e) {
        logger->error("更新用户状态失败: {}", e.what());
        return internalServerErrorResponse("服务器内部错误");
    }
}

// ==================== 获取用户统计 ====================

crow::response UserController::getUserStats(const crow::request& req) {
    try {
        std::string token = extractToken(req);
        if (token.empty()) return unauthorizedResponse("未提供有效的认证令牌");
        if (!isAdmin(token))  return forbiddenResponse("权限不足，需要管理员权限");

        // 用当前管理员 ID 的统计信息作为演示
        std::string adminId = resolveUserId(token);
        auto stats = userService->getUserStatistics(adminId);

        return successResponse("获取统计信息成功", {
            {"total_posts",     stats.totalPosts},
            {"total_likes",     stats.totalLikes},
            {"total_comments",  stats.totalComments},
            {"total_views",     stats.totalViews},
            {"total_followers", stats.totalFollowers},
            {"total_following", stats.totalFollowing}
        });

    } catch (const std::exception& e) {
        logger->error("获取用户统计信息失败: {}", e.what());
        return internalServerErrorResponse("服务器内部错误");
    }
}

} // namespace yachiyo::controllers