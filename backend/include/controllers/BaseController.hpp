#pragma once

#include <string>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>
#include <regex>
#include <memory>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "../utils/Result.hpp"

// 前置声明 JwtUtil，BaseController.cpp 会用到
namespace Yachiyo::Utils { class JwtUtil; }

namespace yachiyo::controllers {

using json = nlohmann::json;

// 路由处理器类型 (Crow 风格)
using CrowRouteHandler = std::function<void(const crow::request&, crow::response&)>;

// HttpRequest/HttpResponse 类型别名，用于 HealthController 等内部路由系统
struct HttpRequest {
    std::string path;
    std::string method;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queryParams;
    std::string body;
    std::string clientIp;
    
    std::string getHeader(const std::string& key) const {
        auto it = headers.find(key);
        return it != headers.end() ? it->second : "";
    }
    
    std::string getQueryParam(const std::string& key) const {
        auto it = queryParams.find(key);
        return it != queryParams.end() ? it->second : "";
    }
};

struct HttpResponse {
    int statusCode = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    
    void setContentType(const std::string& contentType) {
        headers["Content-Type"] = contentType;
    }
    
    void setCorsHeaders() {
        headers["Access-Control-Allow-Origin"] = "*";
        headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    }
};

/**
 * @brief 控制器基类 (Crow 兼容版)
 * 
 * 所有控制器的基类，提供通用功能：
 * - 路由注册
 * - 请求处理
 * - JWT令牌验证
 * - 权限检查
 * - 统一响应格式
 * - 输入验证
 * - 分页支持
 */
class BaseController {
public:
    BaseController();
    virtual ~BaseController();

    // ==================== Crow 路由注册 ====================

    /**
     * @brief 注册路由到 Crow 应用 (子类必须重写)
     * @param app Crow 应用引用
     */
    virtual void registerRoutes(crow::SimpleApp& app) {
        // 默认空实现，子类应重写以注册自己的路由
    }

    /**
     * @brief 注册路由到 Crow 应用 (旧接口，通过 basePath)
     * @param path 基础路径
     * @param app Crow 应用引用
     */
    virtual void registerRoute(const std::string& path, crow::SimpleApp& app);

    /**
     * @brief 处理 HTTP 请求 (Crow 风格)
     * @param req Crow 请求
     * @param res Crow 响应
     */
    virtual void handleRequest(const crow::request& req, crow::response& res);

    // ==================== 内部路由系统 ====================

    /**
     * @brief 注册路由 (内部路由系统，供 HealthController 等使用)
     */
    void registerRoute(const std::string& method, const std::string& path,
                       std::function<HttpResponse(const HttpRequest&)> handler) {
        std::string key = method + ":" + path;
        internalRoutes[key] = handler;
    }

    /**
     * @brief 注册路由 (子类重写，用于内部路由系统)
     */
    virtual void registerRoutes() {}

    /**
     * @brief 获取控制器名称
     */
    virtual std::string getName() const { return "BaseController"; }

    /**
     * @brief 获取基础路径
     */
    virtual std::string getBasePath() const { return "/"; }

protected:
    // 日志器 (所有子控制器可用)
    std::shared_ptr<spdlog::logger> logger;

    // ==================== Crow 响应构建 ====================

    /** @brief 成功响应 (200) */
    crow::response successResponse(const std::string& message,
                                   const nlohmann::json& data = nlohmann::json::object());
    /** @brief 创建响应 (201) */
    crow::response createdResponse(const std::string& message,
                                   const nlohmann::json& data = nlohmann::json::object());
    /** @brief 通用错误响应 */
    crow::response errorResponse(int statusCode, const std::string& message,
                                 const std::string& errorCode = "ERROR");
    /** @brief 400 */
    crow::response badRequestResponse(const std::string& message);
    /** @brief 401 */
    crow::response unauthorizedResponse(const std::string& message = "未授权");
    /** @brief 403 */
    crow::response forbiddenResponse(const std::string& message = "权限不足");
    /** @brief 404 */
    crow::response notFoundResponse(const std::string& message = "资源未找到");
    /** @brief 500 */
    crow::response internalServerErrorResponse(const std::string& message = "服务器内部错误");
    /** @brief 422 验证错误 */
    crow::response validationErrorResponse(const std::vector<std::string>& errors);
    /** @brief 分页响应 */
    crow::response paginatedResponse(const nlohmann::json& data,
                                     int page, int pageSize,
                                     int totalItems, int totalPages);
    /** @brief 异常处理 */
    crow::response handleException(const std::exception& e, const std::string& context);

    // ==================== 请求解析辅助 ====================

    /** @brief 从请求中提取 Bearer Token */
    std::string getAuthToken(const crow::request& req);
    /** @brief 从令牌中提取用户ID */
    std::string getUserIdFromToken(const crow::request& req);
    /** @brief 获取客户端 IP */
    std::string getClientIp(const crow::request& req);
    /** @brief 获取 User-Agent */
    std::string getUserAgent(const crow::request& req);

    // ==================== 输入验证 ====================

    /** @brief 解析请求体 JSON */
    bool validateJson(const crow::request& req, nlohmann::json& json);
    /** @brief 检查必需字段 */
    bool validateRequiredFields(const nlohmann::json& json,
                                const std::vector<std::string>& requiredFields,
                                std::vector<std::string>& missingFields);
    /** @brief 验证字符串长度 */
    bool validateStringLength(const std::string& str, int minLength, int maxLength = -1);
    /** @brief 验证邮箱格式 */
    bool validateEmailFormat(const std::string& email);
    /** @brief 验证 URL 格式 */
    bool validateUrlFormat(const std::string& url);
    /** @brief 分页参数校正 */
    bool validatePaginationParams(int& page, int& pageSize, int maxPageSize = 100);
    /** @brief HTML 实体转义 */
    std::string sanitizeInput(const std::string& input);

    // ==================== 日志辅助 ====================

    void logRequest(const crow::request& req, const std::string& endpoint);
    void logResponse(const crow::request& req, const crow::response& res,
                     const std::string& endpoint);

    // ==================== routes.cpp 中实现的安全方法 ====================

    /**
     * @brief 验证JWT令牌
     * @return Result 包含用户ID (字符串形式)
     */
    Yachiyo::Utils::Result<std::string> validateToken(const crow::request& req);

    /**
     * @brief 检查用户权限
     * @return Result 表示权限检查结果
     */
    Yachiyo::Utils::Result<void> checkPermission(const crow::request& req, const std::string& userId);

    /**
     * @brief 处理具体请求 (子类重写)
     */
    virtual void processRequest(const crow::request& req, crow::response& res,
                                const json& body, const std::string& userId);

    // ==================== 内部路由系统辅助 ====================

    /**
     * @brief 创建 JSON 响应 (用于内部路由系统)
     */
    HttpResponse createJsonResponse(int statusCode, const json& data) {
        HttpResponse response;
        response.statusCode = statusCode;
        response.setContentType("application/json");
        response.setCorsHeaders();
        response.body = data.dump();
        return response;
    }

    template<typename T>
    HttpResponse createSuccessResponse(const T& data, const std::string& message = "操作成功") {
        auto result = Yachiyo::Utils::Result<T>::successResult(data, message);
        return createJsonResponse(200, result.toJson());
    }

    HttpResponse createSuccessResponse(const std::string& message = "操作成功") {
        auto result = Yachiyo::Utils::Result<void>::successResult(message);
        return createJsonResponse(200, result.toJson());
    }

    HttpResponse createErrorResponse(const std::string& code, const std::string& message) {
        auto result = Yachiyo::Utils::Result<void>::errorResult(code, message);
        HttpResponse response;
        response.statusCode = (code == "401" || code == "403") ? std::stoi(code) : 400;
        response.setContentType("application/json");
        response.body = result.toString();
        return response;
    }

private:
    // 内部路由表 (用于 HealthController 等简单路由系统)
    std::map<std::string, std::function<HttpResponse(const HttpRequest&)>> internalRoutes;
};

} // namespace yachiyo::controllers

// 命名空间别名，兼容旧代码中的 Yachiyo::Controllers 引用
namespace Yachiyo { namespace Controllers = yachiyo::controllers; }