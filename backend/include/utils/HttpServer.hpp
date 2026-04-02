#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include "controllers/BaseController.hpp"

namespace Yachiyo {
namespace Utils {

// HTTP 服务器配置
struct HttpServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    int threads = 4;
    int maxConnections = 1000;
    int timeoutSeconds = 30;
    bool enableSSL = false;
    std::string sslCertPath;
    std::string sslKeyPath;
    bool enableCORS = true;
    std::string corsOrigins = "*";
    int maxRequestSizeMB = 10;
    bool enableCompression = true;
    bool enableLogging = true;
    std::string logPath = "logs/http_server.log";
};

// HTTP 请求处理器
using HttpRequestHandler = std::function<Controllers::HttpResponse(const Controllers::HttpRequest&)>;

/**
 * @brief HTTP服务器接口
 */
class HttpServer {
public:
    virtual ~HttpServer() = default;

    /**
     * @brief 启动服务器
     * @return 是否启动成功
     */
    virtual bool start() = 0;

    /**
     * @brief 停止服务器
     */
    virtual void stop() = 0;

    /**
     * @brief 服务器是否运行中
     * @return 运行状态
     */
    virtual bool isRunning() const = 0;

    /**
     * @brief 注册路由
     * @param method HTTP方法
     * @param path 路由路径
     * @param handler 请求处理器
     */
    virtual void registerRoute(Controllers::HttpMethod method, 
                              const std::string& path, 
                              HttpRequestHandler handler) = 0;

    /**
     * @brief 注册控制器
     * @param controller 控制器
     */
    virtual void registerController(std::shared_ptr<Controllers::BaseController> controller) = 0;

    /**
     * @brief 获取服务器配置
     * @return 服务器配置
     */
    virtual HttpServerConfig getConfig() const = 0;

    /**
     * @brief 更新服务器配置
     * @param config 新配置
     */
    virtual void updateConfig(const HttpServerConfig& config) = 0;

    /**
     * @brief 获取服务器统计信息
     * @return 统计信息
     */
    virtual std::map<std::string, int64_t> getStatistics() const = 0;
};

/**
 * @brief 基于Crow的HTTP服务器实现
 */
class CrowHttpServer : public HttpServer {
public:
    CrowHttpServer(const HttpServerConfig& config = HttpServerConfig());
    ~CrowHttpServer();

    // 实现接口方法
    bool start() override;
    void stop() override;
    bool isRunning() const override;
    void registerRoute(Controllers::HttpMethod method, 
                      const std::string& path, 
                      HttpRequestHandler handler) override;
    void registerController(std::shared_ptr<Controllers::BaseController> controller) override;
    HttpServerConfig getConfig() const override;
    void updateConfig(const HttpServerConfig& config) override;
    std::map<std::string, int64_t> getStatistics() const override;

    /**
     * @brief 设置静态文件目录
     * @param path 静态文件目录路径
     */
    void setStaticFileDirectory(const std::string& path);

    /**
     * @brief 启用WebSocket支持
     * @param path WebSocket路径
     */
    void enableWebSocket(const std::string& path = "/ws");

    /**
     * @brief 添加中间件
     * @param middleware 中间件函数
     */
    void addMiddleware(std::function<void(Controllers::HttpRequest&)> middleware);

private:
    /**
     * @brief 初始化服务器
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 处理HTTP请求
     * @param request 原始请求
     * @return HTTP响应
     */
    Controllers::HttpResponse handleRequest(const Controllers::HttpRequest& request);

    /**
     * @brief 记录访问日志
     * @param request 请求信息
     * @param response 响应信息
     * @param durationMs 处理时长（毫秒）
     */
    void logAccess(const Controllers::HttpRequest& request, 
                  const Controllers::HttpResponse& response, 
                  int64_t durationMs);

    /**
     * @brief 验证请求大小
     * @param request 请求信息
     * @return 是否有效
     */
    bool validateRequestSize(const Controllers::HttpRequest& request) const;

    /**
     * @brief 应用CORS头
     * @param response 响应信息
     */
    void applyCORSHeaders(Controllers::HttpResponse& response) const;

    /**
     * @brief 转换HTTP方法
     * @param method HTTP方法枚举
     * @return 方法字符串
     */
    std::string methodToString(Controllers::HttpMethod method) const;

    /**
     * @brief 解析HTTP方法
     * @param method 方法字符串
     * @return HTTP方法枚举
     */
    Controllers::HttpMethod parseMethod(const std::string& method) const;

    // 服务器配置
    HttpServerConfig config;
    
    // 服务器状态
    bool running;
    
    // 路由表
    std::map<std::string, HttpRequestHandler> routes;
    
    // 控制器列表
    std::vector<std::shared_ptr<Controllers::BaseController>> controllers;
    
    // 中间件列表
    std::vector<std::function<void(Controllers::HttpRequest&)>> middlewares;
    
    // 线程池
    std::vector<std::thread> workerThreads;
    
    // 统计信息
    struct Statistics {
        int64_t totalRequests = 0;
        int64_t successfulRequests = 0;
        int64_t failedRequests = 0;
        int64_t totalBytesSent = 0;
        int64_t totalBytesReceived = 0;
        std::map<int, int64_t> statusCodeCounts;
        std::map<std::string, int64_t> endpointCounts;
    } statistics;
    
    // 线程安全
    mutable std::mutex mutex;
};

/**
 * @brief HTTP服务器管理器
 */
class HttpServerManager {
public:
    static HttpServerManager& getInstance();

    /**
     * @brief 创建HTTP服务器
     * @param config 服务器配置
     * @return HTTP服务器实例
     */
    std::shared_ptr<HttpServer> createServer(const HttpServerConfig& config = HttpServerConfig());

    /**
     * @brief 获取默认HTTP服务器
     * @return HTTP服务器实例
     */
    std::shared_ptr<HttpServer> getDefaultServer();

    /**
     * @brief 设置默认HTTP服务器
     * @param server HTTP服务器实例
     */
    void setDefaultServer(std::shared_ptr<HttpServer> server);

    /**
     * @brief 停止所有服务器
     */
    void stopAllServers();

    /**
     * @brief 获取所有服务器
     * @return 服务器列表
     */
    std::vector<std::shared_ptr<HttpServer>> getAllServers() const;

private:
    HttpServerManager() = default;
    ~HttpServerManager();

    // 服务器列表
    std::vector<std::shared_ptr<HttpServer>> servers;
    
    // 默认服务器
    std::shared_ptr<HttpServer> defaultServer;
    
    // 线程安全
    mutable std::mutex mutex;
};

} // namespace Utils
} // namespace Yachiyo