#include "controllers/BaseController.hpp"
#include "utils/Logger.hpp"
#include "utils/JwtUtil.hpp"
#include "utils/RedisUtil.hpp"
#include "config/ConfigManager.hpp"
#include <crow.h>

namespace yachiyo::controllers {

void BaseController::registerRoute(const std::string& path, crow::SimpleApp& app) {
    // 使用运行时路由注册，支持变量路径
    // GET
    app.route_dynamic(path)
        .methods("GET"_method)
        ([this](const crow::request& req, crow::response& res) {
            this->handleRequest(req, res);
        });

    // POST
    app.route_dynamic(path)
        .methods("POST"_method)
        ([this](const crow::request& req, crow::response& res) {
            this->handleRequest(req, res);
        });

    // PUT
    app.route_dynamic(path)
        .methods("PUT"_method)
        ([this](const crow::request& req, crow::response& res) {
            this->handleRequest(req, res);
        });

    // DELETE
    app.route_dynamic(path)
        .methods("DELETE"_method)
        ([this](const crow::request& req, crow::response& res) {
            this->handleRequest(req, res);
        });
}

void BaseController::handleRequest(const crow::request& req, crow::response& res) {
    // 设置默认响应头
    res.add_header("Content-Type", "application/json");
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    // 处理预检请求
    if (req.method == "OPTIONS"_method) {
        res.code = 200;
        res.end();
        return;
    }
    
    // 验证Token
    auto authResult = validateToken(req);
    if (!authResult.isSuccess()) {
        res.code = 401;
        res.write(authResult.toJson().dump());
        res.end();
        return;
    }
    
    // 检查权限
    auto permissionResult = checkPermission(req, authResult.getData().value());
    if (!permissionResult.isSuccess()) {
        res.code = 403;
        res.write(permissionResult.toJson().dump());
        res.end();
        return;
    }
    
    // 解析请求体
    nlohmann::json requestBody;
    if (req.method == "POST"_method || req.method == "PUT"_method) {
        try {
            requestBody = nlohmann::json::parse(req.body);
        } catch (const std::exception& e) {
            auto error = Yachiyo::Utils::Result<void>::errorResult("400", "无效的JSON格式");
            res.code = 400;
            res.write(error.toJson().dump());
            res.end();
            return;
        }
    }
    
    // 调用具体的控制器处理
    processRequest(req, res, requestBody, authResult.getData().value());
}

Yachiyo::Utils::Result<std::string> BaseController::validateToken(const crow::request& req) {
    // 从请求头获取Authorization
    auto authHeader = req.get_header_value("Authorization");
    
    if (authHeader.empty()) {
        return Yachiyo::Utils::Result<std::string>::errorResult("401", "缺少Authorization头");
    }
    
    // 检查Bearer token格式
    if (authHeader.find("Bearer ") != 0) {
        return Yachiyo::Utils::Result<std::string>::errorResult("401", "Token格式错误，应为Bearer token");
    }

    std::string token = authHeader.substr(7); // 移除"Bearer "
    
    // 检查令牌是否在黑名单中
    std::string blacklistKey = "token_blacklist:" + token;
    if (Yachiyo::Utils::RedisUtil::cacheExists(blacklistKey)) {
        return Yachiyo::Utils::Result<std::string>::errorResult("401", "令牌已被撤销");
    }

    // 从配置中获取JWT密钥
    auto configManager = config::ConfigManager::getInstance();
    std::string jwtSecret = configManager->getString("jwt.secret", "yachiyo-default-secret-change-in-production");
    int jwtExpiration = configManager->getInt("jwt.expiresIn", 24);

    // 创建JWT工具实例并验证token
    Yachiyo::Utils::JwtUtil jwtUtil(jwtSecret, jwtExpiration);
    auto [valid, message] = jwtUtil.verifyToken(token);
    
    if (!valid) {
        return Yachiyo::Utils::Result<std::string>::errorResult("401", "令牌验证失败: " + message);
    }

    // 从token中提取用户ID
    int64_t userId = jwtUtil.getUserIdFromToken(token);
    if (userId <= 0) {
        return Yachiyo::Utils::Result<std::string>::errorResult("401", "无法从令牌中获取用户信息");
    }

    // 返回用户ID（作为字符串）
    return Yachiyo::Utils::Result<std::string>::successResult(std::to_string(userId), "Token验证成功");
}

Yachiyo::Utils::Result<void> BaseController::checkPermission(const crow::request& req, const std::string& userId) {
    // 获取请求路径
    std::string path = req.url;
    // 使用 req.method（enum），避免依赖不存在的 method_name()
    auto method = req.method;
    
    // 获取请求头中的Authorization来提取角色信息
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
        return Yachiyo::Utils::Result<void>::errorResult("403", "无法获取用户角色信息");
    }
    
    std::string token = authHeader.substr(7);
    
    // 从配置中获取JWT密钥
    auto configManager = config::ConfigManager::getInstance();
    std::string jwtSecret = configManager->getString("jwt.secret", "yachiyo-default-secret-change-in-production");
    int jwtExpiration = configManager->getInt("jwt.expiresIn", 24);
    
    Yachiyo::Utils::JwtUtil jwtUtil(jwtSecret, jwtExpiration);
    std::string rawRole = jwtUtil.getRoleFromToken(token);
    
    // 归一化角色: JWT 中可能是 SMALLINT "1"/"99" 或字符串 "admin"/"user"
    std::string userRole;
    if (rawRole == "99" || rawRole == "admin" || rawRole == "ADMIN") {
        userRole = "ADMIN";
    } else if (rawRole == "1" || rawRole == "user" || rawRole == "USER" || !rawRole.empty()) {
        userRole = "USER";
    }
    
    // 定义路由权限规则 (基于RBAC模式)
    // 管理员路由 (需要ADMIN角色)
    if (path.find("/api/v1/admin/") == 0) {
        if (userRole != "ADMIN") {
            return Yachiyo::Utils::Result<void>::errorResult("403", "需要管理员权限");
        }
    }
    
    // AI相关路由 (需要AUTH或ADMIN角色)
    if (path.find("/api/v2/ai/") == 0) {
        if (userRole != "USER" && userRole != "ADMIN") {
            return Yachiyo::Utils::Result<void>::errorResult("403", "需要用户权限");
        }
    }
    
    // 帖子相关路由 (需要AUTH或ADMIN角色)
    if (path.find("/api/v1/posts") == 0) {
        if (method == "POST"_method || method == "PUT"_method || method == "DELETE"_method) {
            if (userRole != "USER" && userRole != "ADMIN") {
                return Yachiyo::Utils::Result<void>::errorResult("403", "权限不足");
            }
        }
    }
    
    // 用户相关路由 (某些操作仅限自己或管理员)
    if (path.find("/api/v1/users/") == 0 && method != "GET") {
        if (userRole != "ADMIN") {
            return Yachiyo::Utils::Result<void>::errorResult("403", "权限不足");
        }
    }
    
    // 默认允许读操作 (GET), 写操作需要检查角色
    if (method == "GET"_method) {
        return Yachiyo::Utils::Result<void>::successResult("权限检查通过");
    }
    
    // 其他写操作 (POST, PUT, DELETE) 需要至少USER角色
    if (userRole.empty() || (userRole != "USER" && userRole != "ADMIN")) {
        return Yachiyo::Utils::Result<void>::errorResult("403", "没有权限执行此操作");
    }
    
    return Yachiyo::Utils::Result<void>::successResult("权限检查通过");
}

void BaseController::processRequest(const crow::request& req, crow::response& res, 
                                   const nlohmann::json& body, const std::string& userId) {
    // 基类实现为空，子类应该重写
    auto error = Yachiyo::Utils::Result<void>::errorResult("404", "请求路径不存在");
    res.code = 404;
    res.write(error.toJson().dump());
    res.end();
}

} // namespace yachiyo::controllers