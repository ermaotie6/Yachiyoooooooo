#pragma once

#include <string>
#include <functional>
#include <map>
#include <memory>
#include "utils/Result.hpp"

namespace Yachiyo {
namespace Controllers {

// HTTP 方法枚举
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    OPTIONS
};

// HTTP 请求结构
struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queryParams;
    std::string body;
    std::string clientIp;
    
    // 从 headers 中获取值
    std::string getHeader(const std::string& key) const {
        auto it = headers.find(key);
        return it != headers.end() ? it->second : "";
    }
    
    // 从 queryParams 中获取值
    std::string getQueryParam(const std::string& key) const {
        auto it = queryParams.find(key);
        return it != queryParams.end() ? it->second : "";
    }
};

// HTTP 响应结构
struct HttpResponse {
    int statusCode = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    
    // 设置 Content-Type
    void setContentType(const std::string& contentType) {
        headers["Content-Type"] = contentType;
    }
    
    // 设置 CORS 头
    void setCorsHeaders() {
        headers["Access-Control-Allow-Origin"] = "*";
        headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    }
};

// 路由处理器类型
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

/**
 * @brief 控制器基类
 */
class BaseController {
public:
    BaseController() = default;
    virtual ~BaseController() = default;

    /**
     * @brief 注册路由
     * @param method HTTP方法
     * @param path 路由路径
     * @param handler 路由处理器
     */
    void registerRoute(HttpMethod method, const std::string& path, RouteHandler handler);

    /**
     * @brief 处理HTTP请求
     * @param request HTTP请求
     * @return HTTP响应
     */
    virtual HttpResponse handleRequest(const HttpRequest& request);

    /**
     * @brief 获取控制器名称
     * @return 控制器名称
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 获取基础路径
     * @return 基础路径
     */
    virtual std::string getBasePath() const = 0;

protected:
    /**
     * @brief 创建成功响应
     * @tparam T 数据类型
     * @param data 数据
     * @param message 消息
     * @return HTTP响应
     */
    template<typename T>
    HttpResponse createSuccessResponse(const T& data, const std::string& message = "操作成功") {
        auto result = Utils::Result<T>::successResult(data, message);
        return createJsonResponse(result.toJson());
    }

    /**
     * @brief 创建成功响应（无数据）
     * @param message 消息
     * @return HTTP响应
     */
    HttpResponse createSuccessResponse(const std::string& message = "操作成功") {
        auto result = Utils::Result<void>::successResult(message);
        return createJsonResponse(result.toJson());
    }

    /**
     * @brief 创建错误响应
     * @param code 错误码
     * @param message 错误消息
     * @return HTTP响应
     */
    HttpResponse createErrorResponse(const std::string& code, const std::string& message) {
        auto result = Utils::Result<void>::errorResult(code, message);
        HttpResponse response;
        response.statusCode = (code == "401" || code == "403") ? std::stoi(code) : 400;
        response.setContentType("application/json");
        response.body = result.toString();
        return response;
    }

    /**
     * @brief 创建JSON响应
     * @param json JSON数据
     * @param statusCode 状态码
     * @return HTTP响应
     */
    HttpResponse createJsonResponse(const nlohmann::json& json, int statusCode = 200) {
        HttpResponse response;
        response.statusCode = statusCode;
        response.setContentType("application/json");
        response.setCorsHeaders();
        response.body = json.dump();
        return response;
    }

    /**
     * @brief 验证JWT令牌
     * @param request HTTP请求
     * @return 验证结果和用户ID
     */
    std::pair<bool, int64_t> validateToken(const HttpRequest& request);

    /**
     * @brief 检查用户权限
     * @param userId 用户ID
     * @param requiredRole 所需角色
     * @return 是否有权限
     */
    bool checkPermission(int64_t userId, const std::string& requiredRole);

private:
    std::map<std::string, RouteHandler> routes;
};

} // namespace Controllers
} // namespace Yachiyo