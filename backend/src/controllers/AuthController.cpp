#include "../include/controllers/AuthController.hpp"
#include "../include/utils/Logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace yachiyo::controllers {

// ==================== 用户注册 ====================
void AuthController::registerUser(const crow::request& req, crow::response& res) {
    try {
        auto body = json::parse(req.body);
        
        std::string username = body.value("username", "");
        std::string email = body.value("email", "");
        std::string password = body.value("password", "");
        
        // 验证输入
        if (username.empty() || email.empty() || password.empty()) {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "缺少必需字段: username, email, password"}
            }).dump();
            return;
        }
        
        // 调用认证服务
        auto result = authService->registerUser(username, email, password);
        
        if (result.isSuccess()) {
            auto user = result.getData();
            res.code = 201;
            res.body = json({
                {"code", 201},
                {"msg", "注册成功"},
                {"data", {
                    {"user_id", user->getId()},
                    {"username", user->getUsername()},
                    {"email", user->getEmail()}
                }}
            }).dump();
            LOG_INFO("用户注册成功: " + username);
        } else {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("注册异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 用户登录 ====================
void AuthController::login(const crow::request& req, crow::response& res) {
    try {
        auto body = json::parse(req.body);
        
        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        
        if (username.empty() || password.empty()) {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "缺少用户名或密码"}
            }).dump();
            return;
        }
        
        // 获取用户IP
        std::string userIp = req.remote_ip_address;
        
        auto result = authService->login(username, password, userIp);
        
        if (result.isSuccess()) {
            auto data = result.getData();
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "登录成功"},
                {"data", data}
            }).dump();
            LOG_INFO("用户登录成功: " + username);
        } else {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("登录异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 刷新令牌 ====================
void AuthController::refreshToken(const crow::request& req, crow::response& res) {
    try {
        auto body = json::parse(req.body);
        
        std::string refreshToken = body.value("refresh_token", "");
        
        if (refreshToken.empty()) {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "缺少 refresh_token"}
            }).dump();
            return;
        }
        
        auto result = authService->refreshToken(refreshToken);
        
        if (result.isSuccess()) {
            auto data = result.getData();
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "令牌刷新成功"},
                {"data", data}
            }).dump();
        } else {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("刷新令牌异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 用户注销 ====================
void AuthController::logout(const crow::request& req, crow::response& res) {
    try {
        // 从Authorization头获取令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
        std::string token = authHeader.substr(7);
        int64_t userId = authService->getUserIdFromToken(token);
        
        if (userId <= 0) {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "令牌无效"}
            }).dump();
            return;
        }
        
        auto body = json::parse(req.body);
        std::string refreshToken = body.value("refresh_token", "");
        
        auto result = authService->logout(userId, refreshToken);
        
        if (result.isSuccess()) {
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "注销成功"}
            }).dump();
            LOG_INFO("用户注销: " + std::to_string(userId));
        } else {
            res.code = 500;
            res.body = json({
                {"code", 500},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("注销异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 获取用户资料 ====================
void AuthController::getProfile(const crow::request& req, crow::response& res) {
    try {
        // 从Authorization头获取令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
        std::string token = authHeader.substr(7);
        int64_t userId = authService->getUserIdFromToken(token);
        
        if (userId <= 0) {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "令牌无效或已过期"}
            }).dump();
            return;
        }
        
        auto result = authService->getUserById(userId);
        
        if (result.isSuccess()) {
            auto user = result.getData();
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "获取成功"},
                {"data", user->toJson()}
            }).dump();
        } else {
            res.code = 404;
            res.body = json({
                {"code", 404},
                {"msg", "用户不存在"}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("获取资料异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 更新用户资料 ====================
void AuthController::updateProfile(const crow::request& req, crow::response& res) {
    try {
        // 从Authorization头获取令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
        std::string token = authHeader.substr(7);
        int64_t userId = authService->getUserIdFromToken(token);
        
        if (userId <= 0) {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "令牌无效或已过期"}
            }).dump();
            return;
        }
        
        auto body = json::parse(req.body);
        std::string nickname = body.value("nickname", "");
        std::string bio = body.value("bio", "");
        
        if (nickname.empty() && bio.empty()) {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "至少需要提供 nickname 或 bio"}
            }).dump();
            return;
        }
        
        // TODO: 实现数据库更新逻辑
        // 这里简化处理，实际应调用认证服务更新用户信息
        
        res.code = 200;
        res.body = json({
            {"code", 200},
            {"msg", "更新成功"},
            {"data", {
                {"user_id", userId},
                {"nickname", nickname},
                {"bio", bio}
            }}
        }).dump();
        LOG_INFO("用户资料更新: " + std::to_string(userId));
        
    } catch (const std::exception& e) {
        LOG_ERROR("更新资料异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

} // namespace yachiyo::controllers
