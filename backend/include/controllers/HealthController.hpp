#pragma once

#include "BaseController.hpp"
#include "../dto/CommonDTO.hpp"
#include "../utils/LogUtils.hpp"
#include <chrono>
#include <sys/utsname.h>
#include <sys/resource.h>

namespace yachiyo::controllers {

/**
 * @brief 健康检查控制器
 */
class HealthController : public BaseController {
private:
    std::shared_ptr<Logger> logger;
    
public:
    HealthController() {
        logger = LogUtils::getLogger("HealthController");
    }
    
    virtual ~HealthController() = default;
    
    /**
     * @brief 注册路由
     */
    void registerRoutes() override {
        // 健康检查端点
        registerRoute("GET", "/", [this](const HttpRequest& req) -> HttpResponse {
            return checkHealth(req);
        });
        
        // 详细健康信息
        registerRoute("GET", "/detailed", [this](const HttpRequest& req) -> HttpResponse {
            return detailedHealth(req);
        });
        
        // 系统信息
        registerRoute("GET", "/system", [this](const HttpRequest& req) -> HttpResponse {
            return systemInfo(req);
        });
        
        // 数据库健康检查
        registerRoute("GET", "/database", [this](const HttpRequest& req) -> HttpResponse {
            return databaseHealth(req);
        });
        
        // Redis健康检查
        registerRoute("GET", "/redis", [this](const HttpRequest& req) -> HttpResponse {
            return redisHealth(req);
        });
        
        logger->info("健康检查控制器路由已注册");
    }
    
private:
    /**
     * @brief 基础健康检查
     */
    HttpResponse checkHealth(const HttpRequest& req) {
        try {
            dto::ResponseDTO response;
            response.data = {
                {"status", "UP"},
                {"timestamp", getCurrentTimestamp()},
                {"service", "Yachiyo C++ Server"},
                {"version", "1.0.0"}
            };
            
            return createJsonResponse(200, response.toJson());
            
        } catch (const std::exception& e) {
            logger->error("健康检查失败: {}", e.what());
            
            dto::ResponseDTO response(500, "Health check failed", {
                {"status", "DOWN"},
                {"error", e.what()}
            });
            
            return createJsonResponse(500, response.toJson());
        }
    }
    
    /**
     * @brief 详细健康信息
     */
    HttpResponse detailedHealth(const HttpRequest& req) {
        try {
            // 获取系统信息
            utsname sysInfo;
            if (uname(&sysInfo) != 0) {
                throw std::runtime_error("无法获取系统信息");
            }
            
            // 获取内存使用情况
            struct rusage usage;
            if (getrusage(RUSAGE_SELF, &usage) != 0) {
                throw std::runtime_error("无法获取资源使用情况");
            }
            
            // 获取当前时间
            auto now = std::chrono::system_clock::now();
            auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
                now - Application::getInstance()->getStartTime()
            ).count();
            
            dto::ResponseDTO response;
            response.data = {
                {"status", "UP"},
                {"timestamp", getCurrentTimestamp()},
                {"uptime", uptime},
                {"system", {
                    {"os", sysInfo.sysname},
                    {"release", sysInfo.release},
                    {"version", sysInfo.version},
                    {"machine", sysInfo.machine}
                }},
                {"resources", {
                    {"userTime", usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0},
                    {"systemTime", usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0},
                    {"maxRSS", usage.ru_maxrss},
                    {"pageFaults", usage.ru_majflt}
                }},
                {"services", {
                    {"http", "UP"},
                    {"database", checkDatabaseStatus()},
                    {"redis", checkRedisStatus()},
                    {"ai", checkAIStatus()}
                }}
            };
            
            return createJsonResponse(200, response.toJson());
            
        } catch (const std::exception& e) {
            logger->error("详细健康检查失败: {}", e.what());
            
            dto::ResponseDTO response(500, "Detailed health check failed", {
                {"status", "DEGRADED"},
                {"error", e.what()}
            });
            
            return createJsonResponse(500, response.toJson());
        }
    }
    
    /**
     * @brief 系统信息
     */
    HttpResponse systemInfo(const HttpRequest& req) {
        try {
            // 获取系统信息
            utsname sysInfo;
            if (uname(&sysInfo) != 0) {
                throw std::runtime_error("无法获取系统信息");
            }
            
            // 获取内存信息
            std::ifstream meminfo("/proc/meminfo");
            std::string line;
            long totalMem = 0, freeMem = 0, availableMem = 0;
            
            while (std::getline(meminfo, line)) {
                std::istringstream iss(line);
                std::string key;
                long value;
                std::string unit;
                
                iss >> key >> value >> unit;
                
                if (key == "MemTotal:") totalMem = value;
                else if (key == "MemFree:") freeMem = value;
                else if (key == "MemAvailable:") availableMem = value;
            }
            
            // 获取CPU信息
            std::ifstream cpuinfo("/proc/cpuinfo");
            int cpuCount = 0;
            std::string cpuModel;
            
            while (std::getline(cpuinfo, line)) {
                if (line.find("model name") != std::string::npos) {
                    cpuModel = line.substr(line.find(":") + 2);
                    cpuCount++;
                } else if (line.find("processor") != std::string::npos) {
                    cpuCount = std::max(cpuCount, std::stoi(line.substr(line.find(":") + 2)) + 1);
                }
            }
            
            // 获取负载平均值
            double loadavg[3];
            if (getloadavg(loadavg, 3) != 3) {
                loadavg[0] = loadavg[1] = loadavg[2] = 0.0;
            }
            
            dto::ResponseDTO response;
            response.data = {
                {"system", {
                    {"os", sysInfo.sysname},
                    {"release", sysInfo.release},
                    {"version", sysInfo.version},
                    {"machine", sysInfo.machine},
                    {"nodename", sysInfo.nodename}
                }},
                {"cpu", {
                    {"count", cpuCount},
                    {"model", cpuModel},
                    {"load1", loadavg[0]},
                    {"load5", loadavg[1]},
                    {"load15", loadavg[2]}
                }},
                {"memory", {
                    {"total", totalMem},
                    {"free", freeMem},
                    {"available", availableMem},
                    {"used", totalMem - availableMem},
                    {"usage", static_cast<double>(totalMem - availableMem) / totalMem * 100}
                }},
                {"disk", getDiskInfo()},
                {"network", getNetworkInfo()}
            };
            
            return createJsonResponse(200, response.toJson());
            
        } catch (const std::exception& e) {
            logger->error("系统信息获取失败: {}", e.what());
            
            dto::ResponseDTO response(500, "System info failed", {
                {"error", e.what()}
            });
            
            return createJsonResponse(500, response.toJson());
        }
    }
    
    /**
     * @brief 数据库健康检查
     */
    HttpResponse databaseHealth(const HttpRequest& req) {
        try {
            std::string status = checkDatabaseStatus();
            
            dto::ResponseDTO response;
            response.data = {
                {"status", status},
                {"timestamp", getCurrentTimestamp()},
                {"details", getDatabaseDetails()}
            };
            
            return createJsonResponse(200, response.toJson());
            
        } catch (const std::exception& e) {
            logger->error("数据库健康检查失败: {}", e.what());
            
            dto::ResponseDTO response(500, "Database health check failed", {
                {"status", "DOWN"},
                {"error", e.what()}
            });
            
            return createJsonResponse(500, response.toJson());
        }
    }
    
    /**
     * @brief Redis健康检查
     */
    HttpResponse redisHealth(const HttpRequest& req) {
        try {
            std::string status = checkRedisStatus();
            
            dto::ResponseDTO response;
            response.data = {
                {"status", status},
                {"timestamp", getCurrentTimestamp()},
                {"details", getRedisDetails()}
            };
            
            return createJsonResponse(200, response.toJson());
            
        } catch (const std::exception& e) {
            logger->error("Redis健康检查失败: {}", e.what());
            
            dto::ResponseDTO response(500, "Redis health check failed", {
                {"status", "DOWN"},
                {"error", e.what()}
            });
            
            return createJsonResponse(500, response.toJson());
        }
    }
    
private:
    /**
     * @brief 获取当前时间戳
     */
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    /**
     * @brief 检查数据库状态
     */
    std::string checkDatabaseStatus() {
        // 这里实现数据库连接检查
        // 暂时返回模拟状态
        return "UP";
    }
    
    /**
     * @brief 检查Redis状态
     */
    std::string checkRedisStatus() {
        // 这里实现Redis连接检查
        // 暂时返回模拟状态
        return "UP";
    }
    
    /**
     * @brief 检查AI服务状态
     */
    std::string checkAIStatus() {
        // 这里实现AI服务检查
        // 暂时返回模拟状态
        return "UP";
    }
    
    /**
     * @brief 获取数据库详细信息
     */
    json getDatabaseDetails() {
        // 这里实现获取数据库详细信息
        return {
            {"type", "MySQL"},
            {"version", "8.0.0"},
            {"connections", 5},
            {"maxConnections", 100}
        };
    }
    
    /**
     * @brief 获取Redis详细信息
     */
    json getRedisDetails() {
        // 这里实现获取Redis详细信息
        return {
            {"version", "7.0.0"},
            {"usedMemory", "10MB"},
            {"connectedClients", 2},
            {"keys", 50}
        };
    }
    
    /**
     * @brief 获取磁盘信息
     */
    json getDiskInfo() {
        // 这里实现获取磁盘信息
        return {
            {"total", "100GB"},
            {"used", "30GB"},
            {"free", "70GB"},
            {"usage", 30}
        };
    }
    
    /**
     * @brief 获取网络信息
     */
    json getNetworkInfo() {
        // 这里实现获取网络信息
        return {
            {"hostname", "yachiyo-server"},
            {"ip", "127.0.0.1"},
            {"interfaces", json::array()}
        };
    }
};

} // namespace yachiyo::controllers