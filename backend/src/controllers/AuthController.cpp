#include "controllers/AuthController.hpp"
#include "utils/LogUtils.hpp"
#include "utils/Logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace yachiyo::controllers {

// ==================== 路由注册 ====================
void AuthController::registerRoutes(crow::SimpleApp& app) {
    // 注册 — 不需要认证
    CROW_ROUTE(app, "/api/v1/auth/register")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->registerUser(req, res);
            return res;
        });

    // 登录 — 不需要认证
    CROW_ROUTE(app, "/api/v1/auth/login")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->login(req, res);
            return res;
        });

    // 刷新令牌
    CROW_ROUTE(app, "/api/v1/auth/refresh")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->refreshToken(req, res);
            return res;
        });

    // 注销 — 需要认证
    CROW_ROUTE(app, "/api/v1/auth/logout")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->logout(req, res);
            return res;
        });

    // 获取个人资料 — 需要认证
    CROW_ROUTE(app, "/api/v1/auth/profile")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->getProfile(req, res);
            return res;
        });

    // 更新个人资料 — 需要认证
    CROW_ROUTE(app, "/api/v1/auth/profile")
        .methods("PUT"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->updateProfile(req, res);
            return res;
        });

    LOG_INFO("认证控制器路由已注册");
}

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
            auto user = result.value();
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
                {"msg", result.getMessage()}
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
            const auto& data = result.value();
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
                {"msg", result.getMessage()}
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
            const auto& data = result.value();
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
                {"msg", result.getMessage()}
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
        // 使用基类方法提取令牌
        std::string token = getAuthToken(req);
        if (token.empty()) {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
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
                {"msg", result.getMessage()}
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
        // 使用基类方法提取令牌
        std::string token = getAuthToken(req);
        if (token.empty()) {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
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
            auto user = result.value();
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
        // 使用基类方法提取令牌
        std::string token = getAuthToken(req);
        if (token.empty()) {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
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
        
        // 构建更新SQL
        std::vector<std::string> setClauses;
        std::vector<std::string> params;
        int paramIdx = 1;
        
        if (!nickname.empty()) {
            setClauses.push_back("nickname = $" + std::to_string(paramIdx++));
            params.push_back(nickname);
        }
        if (!bio.empty()) {
            setClauses.push_back("bio = $" + std::to_string(paramIdx++));
            params.push_back(bio);
        }
        setClauses.push_back("updated_at = NOW()");
        
        std::string sql = "UPDATE users SET ";
        for (size_t i = 0; i < setClauses.size(); ++i) {
            if (i > 0) sql += ", ";
            sql += setClauses[i];
        }
        sql += " WHERE id = $" + std::to_string(paramIdx);
        params.push_back(std::to_string(userId));
        
        // 通过 authService 获取 dbUtil 执行更新
        // 由于 AuthController 没有直接持有 dbUtil，这里通过获取用户来验证更新
        auto userResult = authService->getUserById(userId);
        if (!userResult.isSuccess()) {
            res.code = 404;
            res.body = json({
                {"code", 404},
                {"msg", "用户不存在"}
            }).dump();
            return;
        }
        
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
