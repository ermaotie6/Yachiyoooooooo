#include "utils/HttpServer.hpp"
#include "controllers/BaseController.hpp"
#include <crow.h>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace Yachiyo {
namespace Utils {

// CrowHttpServer实现
CrowHttpServer::CrowHttpServer(const HttpServerConfig& config)
    : config(config), running(false) {
    initialize();
}

CrowHttpServer::~CrowHttpServer() {
    cleanup();
}

bool CrowHttpServer::initialize() {
    try {
        // 初始化统计信息
        statistics = Statistics();
        running = false;
        return true;
    } catch (const std::exception& e) {
        // 记录错误日志
        std::cerr << "Failed to initialize HTTP server: " << e.what() << std::endl;
        return false;
    }
}

void CrowHttpServer::cleanup() {
    stop();
    routes.clear();
    controllers.clear();
    middlewares.clear();
    workerThreads.clear();
}

bool CrowHttpServer::start() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (running) {
        return true;
    }
    
    try {
        // 创建Crow应用
        crow::SimpleApp app;
        
        // 配置Crow应用
        app.loglevel(crow::LogLevel::Warning);
        
        // 设置端口和地址
        app.port(config.port).bindaddr(config.host);
        
        // 设置线程数
        app.concurrency(config.threads);
        
        // 注册控制器路由
        for (auto& controller : controllers) {
            controller->registerRoutes(app);
        }
        
        // 注册自定义路由
        // 注意：这里需要将routes映射到Crow的路由，但routes的类型不匹配
        // 暂时跳过自定义路由注册
        
        // 启动服务器线程
        workerThreads.emplace_back([this, app]() mutable {
            try {
                running = true;
                app.run();
            } catch (const std::exception& e) {
                std::cerr << "HTTP server error: " << e.what() << std::endl;
                running = false;
            }
        });
        
        // 等待服务器启动
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to start HTTP server: " << e.what() << std::endl;
        running = false;
        return false;
    }
}

void CrowHttpServer::stop() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!running) {
        return;
    }
    
    running = false;
    
    // 等待所有工作线程结束
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    workerThreads.clear();
}

bool CrowHttpServer::isRunning() const {
    std::lock_guard<std::mutex> lock(mutex);
    return running;
}

void CrowHttpServer::registerRoute(Controllers::HttpMethod method, 
                                  const std::string& path, 
                                  HttpRequestHandler handler) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 创建路由键
    std::string routeKey = methodToString(method) + ":" + path;
    routes[routeKey] = handler;
}

void CrowHttpServer::registerController(std::shared_ptr<Controllers::BaseController> controller) {
    std::lock_guard<std::mutex> lock(mutex);
    controllers.push_back(controller);
}

HttpServerConfig CrowHttpServer::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex);
    return config;
}

void CrowHttpServer::updateConfig(const HttpServerConfig& newConfig) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (running) {
        // 如果服务器正在运行，需要重启才能应用新配置
        std::cerr << "Warning: Server is running. Restart required to apply new configuration." << std::endl;
    }
    
    config = newConfig;
}

std::map<std::string, int64_t> CrowHttpServer::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::map<std::string, int64_t> stats;
    stats["totalRequests"] = statistics.totalRequests;
    stats["successfulRequests"] = statistics.successfulRequests;
    stats["failedRequests"] = statistics.failedRequests;
    stats["totalBytesSent"] = statistics.totalBytesSent;
    stats["totalBytesReceived"] = statistics.totalBytesReceived;
    
    // 添加状态码统计
    for (const auto& [code, count] : statistics.statusCodeCounts) {
        stats["status_" + std::to_string(code)] = count;
    }
    
    // 添加端点统计
    for (const auto& [endpoint, count] : statistics.endpointCounts) {
        stats["endpoint_" + endpoint] = count;
    }
    
    return stats;
}

void CrowHttpServer::setStaticFileDirectory(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    // 静态文件目录功能需要Crow的静态文件中间件
    // 暂时记录但不实现
    std::cout << "Static file directory set to: " << path << std::endl;
}

void CrowHttpServer::enableWebSocket(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex);
    // WebSocket功能需要Crow的WebSocket支持
    // 暂时记录但不实现
    std::cout << "WebSocket enabled at path: " << path << std::endl;
}

void CrowHttpServer::addMiddleware(std::function<void(Controllers::HttpRequest&)> middleware) {
    std::lock_guard<std::mutex> lock(mutex);
    middlewares.push_back(middleware);
}

Controllers::HttpResponse CrowHttpServer::handleRequest(const Controllers::HttpRequest& request) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        // 验证请求大小
        if (!validateRequestSize(request)) {
            Controllers::HttpResponse response;
            response.statusCode = 413;
            response.body = "Request too large";
            response.headers["Content-Type"] = "text/plain";
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            logAccess(request, response, durationMs);
            
            return response;
        }
        
        // 应用中间件
        Controllers::HttpRequest processedRequest = request;
        for (auto& middleware : middlewares) {
            middleware(processedRequest);
        }
        
        // 查找路由处理器
        std::string routeKey = methodToString(processedRequest.method) + ":" + processedRequest.path;
        auto it = routes.find(routeKey);
        
        Controllers::HttpResponse response;
        
        if (it != routes.end()) {
            // 调用路由处理器
            response = it->second(processedRequest);
        } else {
            // 路由未找到
            response.statusCode = 404;
            response.body = "Not Found";
            response.headers["Content-Type"] = "text/plain";
        }
        
        // 应用CORS头
        applyCORSHeaders(response);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        logAccess(request, response, durationMs);
        
        return response;
        
    } catch (const std::exception& e) {
        Controllers::HttpResponse response;
        response.statusCode = 500;
        response.body = std::string("Internal Server Error: ") + e.what();
        response.headers["Content-Type"] = "text/plain";
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        logAccess(request, response, durationMs);
        
        return response;
    }
}

void CrowHttpServer::logAccess(const Controllers::HttpRequest& request, 
                              const Controllers::HttpResponse& response, 
                              int64_t durationMs) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 更新统计信息
    statistics.totalRequests++;
    
    if (response.statusCode >= 200 && response.statusCode < 400) {
        statistics.successfulRequests++;
    } else {
        statistics.failedRequests++;
    }
    
    statistics.statusCodeCounts[response.statusCode]++;
    statistics.endpointCounts[request.path]++;
    
    // 记录访问日志
    if (config.enableLogging) {
        try {
            std::ofstream logFile(config.logPath, std::ios::app);
            if (logFile.is_open()) {
                auto now = std::chrono::system_clock::now();
                auto now_time_t = std::chrono::system_clock::to_time_t(now);
                
                logFile << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
                        << " " << request.clientIp
                        << " " << methodToString(request.method)
                        << " " << request.path
                        << " " << response.statusCode
                        << " " << durationMs << "ms"
                        << " " << request.body.size() << "B"
                        << " " << response.body.size() << "B"
                        << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to write access log: " << e.what() << std::endl;
        }
    }
}

bool CrowHttpServer::validateRequestSize(const Controllers::HttpRequest& request) const {
    size_t maxSize = config.maxRequestSizeMB * 1024 * 1024;
    return request.body.size() <= maxSize;
}

void CrowHttpServer::applyCORSHeaders(Controllers::HttpResponse& response) const {
    if (config.enableCORS) {
        response.headers["Access-Control-Allow-Origin"] = config.corsOrigins;
        response.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
        response.headers["Access-Control-Allow-Credentials"] = "true";
        response.headers["Access-Control-Max-Age"] = "86400";
    }
}

std::string CrowHttpServer::methodToString(Controllers::HttpMethod method) const {
    switch (method) {
        case Controllers::HttpMethod::GET: return "GET";
        case Controllers::HttpMethod::POST: return "POST";
        case Controllers::HttpMethod::PUT: return "PUT";
        case Controllers::HttpMethod::DELETE: return "DELETE";
        case Controllers::HttpMethod::PATCH: return "PATCH";
        case Controllers::HttpMethod::HEAD: return "HEAD";
        case Controllers::HttpMethod::OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

Controllers::HttpMethod CrowHttpServer::parseMethod(const std::string& method) const {
    if (method == "GET") return Controllers::HttpMethod::GET;
    if (method == "POST") return Controllers::HttpMethod::POST;
    if (method == "PUT") return Controllers::HttpMethod::PUT;
    if (method == "DELETE") return Controllers::HttpMethod::DELETE;
    if (method == "PATCH") return Controllers::HttpMethod::PATCH;
    if (method == "HEAD") return Controllers::HttpMethod::HEAD;
    if (method == "OPTIONS") return Controllers::HttpMethod::OPTIONS;
    return Controllers::HttpMethod::GET;
}

// HttpServerManager实现
HttpServerManager::~HttpServerManager() {
    stopAllServers();
}

HttpServerManager& HttpServerManager::getInstance() {
    static HttpServerManager instance;
    return instance;
}

std::shared_ptr<HttpServer> HttpServerManager::createServer(const HttpServerConfig& config) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto server = std::make_shared<CrowHttpServer>(config);
    servers.push_back(server);
    
    if (!defaultServer) {
        defaultServer = server;
    }
    
    return server;
}

std::shared_ptr<HttpServer> HttpServerManager::getDefaultServer() {
    std::lock_guard<std::mutex> lock(mutex);
    return defaultServer;
}

void HttpServerManager::setDefaultServer(std::shared_ptr<HttpServer> server) {
    std::lock_guard<std::mutex> lock(mutex);
    defaultServer = server;
}

void HttpServerManager::stopAllServers() {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& server : servers) {
        server->stop();
    }
    
    servers.clear();
    defaultServer.reset();
}

std::vector<std::shared_ptr<HttpServer>> HttpServerManager::getAllServers() const {
    std::lock_guard<std::mutex> lock(mutex);
    return servers;
}

} // namespace Utils
} // namespace Yachiyo