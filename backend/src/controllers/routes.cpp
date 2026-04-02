#include "controllers/BaseController.hpp"
#include "utils/Logger.hpp"
#include "utils/JwtUtil.hpp"
#include "utils/RedisUtil.hpp"
#include <crow.h>

namespace Yachiyo {
namespace controllers {

void BaseController::registerRoute(const std::string& path, crow::SimpleApp& app) {
    // 注册GET请求处理器
    CROW_ROUTE(app, path)
    .methods("GET"_method)
    ([this](const crow::request& req, crow::response& res) {
        this->handleRequest(req, res);
    });
    
    // 注册POST请求处理器
    CROW_ROUTE(app, path)
    .methods("POST"_method)
    ([this](const crow::request& req, crow::response& res) {
        this->handleRequest(req, res);
    });
    
    // 注册PUT请求处理器
    CROW_ROUTE(app, path)
    .methods("PUT"_method)
    ([this](const crow::request& req, crow::response& res) {
        this->handleRequest(req, res);
    });
    
    // 注册DELETE请求处理器
    CROW_ROUTE(app, path)
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
    auto permissionResult = checkPermission(req, authResult.getData<std::string>());
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
            auto error = Utils::Result<void>::errorResult("400", "无效的JSON格式");
            res.code = 400;
            res.write(error.toJson().dump());
            res.end();
            return;
        }
    }
    
    // 调用具体的控制器处理
    // 子类应该重写handleRequest方法
    // 这里只是基类实现，实际应调用子类的processRequest
    processRequest(req, res, requestBody, authResult.getData<std::string>());
}

Utils::Result<std::string> BaseController::validateToken(const crow::request& req) {
    // 从请求头获取Authorization
    auto authHeader = req.get_header_value("Authorization");
    
    if (authHeader.empty()) {
        return Utils::Result<std::string>::errorResult("401", "缺少Authorization头");
    }
    
    // 检查Bearer token格式
    if (authHeader.find("Bearer ") != 0) {
        return Utils::Result<std::string>::errorResult("401", "Token格式错误，应为Bearer token");
    }

    std::string token = authHeader.substr(7); // 移除"Bearer "
    
    // 检查令牌是否在黑名单中
    std::string blacklistKey = "token_blacklist:" + token;
    if (Utils::RedisUtil::cacheExists(blacklistKey)) {
        return Utils::Result<std::string>::errorResult("401", "令牌已被撤销");
    }

    // 创建JWT工具实例并验证token
    Utils::JwtUtil jwtUtil("your-secret-key-change-this", 24);
    auto [valid, message] = jwtUtil.verifyToken(token);
    
    if (!valid) {
        return Utils::Result<std::string>::errorResult("401", "令牌验证失败: " + message);
    }

    // 从token中提取用户ID
    int64_t userId = jwtUtil.getUserIdFromToken(token);
    if (userId <= 0) {
        return Utils::Result<std::string>::errorResult("401", "无法从令牌中获取用户信息");
    }

    // 返回用户ID（作为字符串）
    return Utils::Result<std::string>::successResult(std::to_string(userId), "Token验证成功");
}

Utils::Result<void> BaseController::checkPermission(const crow::request& req, const std::string& userId) {
    // 实现权限检查逻辑
    
    // 获取请求路径
    std::string path = req.url;
    std::string method = std::string(req.method_name());
    
    // 获取请求头中的Authorization来提取角色信息
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
        return Utils::Result<void>::errorResult("403", "无法获取用户角色信息");
    }
    
    std::string token = authHeader.substr(7);
    Utils::JwtUtil jwtUtil("your-secret-key-change-this", 24);
    std::string userRole = jwtUtil.getRoleFromToken(token);
    
    // 定义路由权限规则 (基于RBAC模式)
    // 管理员路由 (需要ADMIN角色)
    if (path.find("/api/v1/admin/") == 0) {
        if (userRole != "ADMIN") {
            return Utils::Result<void>::errorResult("403", "需要管理员权限");
        }
    }
    
    // AI相关路由 (需要AUTH或ADMIN角色)
    if (path.find("/api/v2/ai/") == 0) {
        if (userRole != "USER" && userRole != "ADMIN") {
            return Utils::Result<void>::errorResult("403", "需要用户权限");
        }
    }
    
    // 帖子相关路由 (需要AUTH或ADMIN角色)
    if (path.find("/api/v1/posts") == 0) {
        // POST、PUT、DELETE操作需要验证所有权限
        if ((method == "POST" || method == "PUT" || method == "DELETE")) {
            if (userRole != "USER" && userRole != "ADMIN") {
                return Utils::Result<void>::errorResult("403", "权限不足");
            }
        }
    }
    
    // 用户相关路由 (某些操作仅限自己或管理员)
    if (path.find("/api/v1/users/") == 0 && method != "GET") {
        // 非GET操作需要特殊权限检查
        if (userRole != "ADMIN") {
            return Utils::Result<void>::errorResult("403", "权限不足");
        }
    }
    
    // 默认允许读操作 (GET), 写操作需要检查角色
    if (method == "GET") {
        return Utils::Result<void>::successResult("权限检查通过");
    }
    
    // 其他写操作 (POST, PUT, DELETE) 需要至少USER角色
    if (userRole.empty() || (userRole != "USER" && userRole != "ADMIN")) {
        return Utils::Result<void>::errorResult("403", "没有权限执行此操作");
    }
    
    return Utils::Result<void>::successResult("权限检查通过");
}

void BaseController::processRequest(const crow::request& req, crow::response& res, 
                                   const nlohmann::json& body, const std::string& userId) {
    // 基类实现为空，子类应该重写
    auto error = Utils::Result<void>::errorResult("404", "请求路径不存在");
    res.code = 404;
    res.write(error.toJson().dump());
    res.end();
}

} // namespace controllers
} // namespace Yachiyo