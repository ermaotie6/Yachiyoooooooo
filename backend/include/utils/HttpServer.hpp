#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <crow.h>
#include "../controllers/BaseController.hpp"

namespace yachiyo::utils {

/**
 * @brief 基于Crow的HTTP服务器实现
 * 
 * 提供 Application.cpp 使用的接口:
 * - setHost/setPort/setWorkers
 * - enableSSL/enableCORS/enableCompression/enableRateLimit
 * - registerController(path, controller)
 * - start/stop/wait
 */
class CrowHttpServer {
public:
    CrowHttpServer();
    ~CrowHttpServer();

    // ===== 配置接口 =====
    void setHost(const std::string& host) { host_ = host; }
    void setPort(int port) { port_ = port; }
    void setWorkers(int workers) { workers_ = workers; }
    
    std::string getHost() const { return host_; }
    int getPort() const { return port_; }
    
    // SSL 配置
    void enableSSL(const std::string& certPath, const std::string& keyPath) {
        sslEnabled_ = true;
        sslCertPath_ = certPath;
        sslKeyPath_ = keyPath;
    }
    
    // CORS 配置
    void enableCORS(const std::string& origin, const std::string& methods,
                    const std::string& headers, bool credentials) {
        corsEnabled_ = true;
        corsOrigin_ = origin;
        corsMethods_ = methods;
        corsHeaders_ = headers;
        corsCredentials_ = credentials;
    }
    
    // 压缩配置
    void enableCompression(int level) {
        compressionEnabled_ = true;
        compressionLevel_ = level;
    }
    
    // 速率限制配置
    void enableRateLimit(int maxRequests, int windowSeconds) {
        rateLimitEnabled_ = true;
        rateLimitMaxRequests_ = maxRequests;
        rateLimitWindowSeconds_ = windowSeconds;
    }

    // ===== 路由注册 =====
    
    /**
     * @brief 注册控制器到指定路径
     * @param basePath 基础路径 (如 "/api/v1/auth")
     * @param controller 控制器实例
     */
    void registerController(const std::string& basePath, 
                           std::shared_ptr<controllers::BaseController> controller);

    // ===== 服务器控制 =====
    bool start();
    void stop();
    void wait();
    bool isRunning() const { return running_; }

private:
    crow::SimpleApp app_;
    
    std::string host_ = "0.0.0.0";
    int port_ = 8080;
    int workers_ = 4;
    bool running_ = false;
    
    // SSL
    bool sslEnabled_ = false;
    std::string sslCertPath_;
    std::string sslKeyPath_;
    
    // CORS
    bool corsEnabled_ = true;
    std::string corsOrigin_ = "*";
    std::string corsMethods_ = "GET,POST,PUT,DELETE,OPTIONS";
    std::string corsHeaders_ = "Content-Type,Authorization";
    bool corsCredentials_ = true;
    
    // 压缩
    bool compressionEnabled_ = true;
    int compressionLevel_ = 6;
    
    // 速率限制
    bool rateLimitEnabled_ = false;
    int rateLimitMaxRequests_ = 100;
    int rateLimitWindowSeconds_ = 60;
    
    // 控制器列表
    std::vector<std::pair<std::string, std::shared_ptr<controllers::BaseController>>> controllers_;
    
    // 服务器线程
    std::thread serverThread_;
    mutable std::mutex mutex_;
};

} // namespace yachiyo::utils

// 向后兼容别名
namespace Yachiyo::Utils {
    using CrowHttpServer = yachiyo::utils::CrowHttpServer;
}