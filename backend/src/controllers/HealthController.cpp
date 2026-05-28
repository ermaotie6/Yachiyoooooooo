#include "controllers/HealthController.hpp"
#include "Application.hpp"
#include "services/WebSocketService.hpp"
#include "utils/JsonUtils.hpp"
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>

extern std::shared_ptr<Yachiyo::Services::WebSocketService> g_webSocketService;

namespace yachiyo {
namespace controllers {

HealthController::HealthController() {
    // 初始化健康检查控制器
}

HealthController::~HealthController() {
    // 清理资源
}

void HealthController::registerRoutes(crow::SimpleApp& app) {
    // v1 路由（与 Application 启动日志一致）
    CROW_ROUTE(app, "/api/v1/health")([this]() {
        return this->healthCheck();
    });
    CROW_ROUTE(app, "/api/v1/health/detailed")([this]() {
        return this->detailedHealthCheck();
    });
    CROW_ROUTE(app, "/api/v1/health/ready")([this]() {
        return this->readinessCheck();
    });
    CROW_ROUTE(app, "/api/v1/health/live")([this]() {
        return this->livenessCheck();
    });
    CROW_ROUTE(app, "/api/v1/health/metrics")([this]() {
        return this->metrics();
    });
    CROW_ROUTE(app, "/api/v1/health/version")([this]() {
        return this->versionInfo();
    });

    // 兼容旧路由
    CROW_ROUTE(app, "/api/health")([this]() {
        return this->healthCheck();
    });
    
    CROW_ROUTE(app, "/api/health/detailed")([this]() {
        return this->detailedHealthCheck();
    });
    
    CROW_ROUTE(app, "/api/health/ready")([this]() {
        return this->readinessCheck();
    });
    
    CROW_ROUTE(app, "/api/health/live")([this]() {
        return this->livenessCheck();
    });
    
    CROW_ROUTE(app, "/api/health/metrics")([this]() {
        return this->metrics();
    });
    
    CROW_ROUTE(app, "/api/health/version")([this]() {
        return this->versionInfo();
    });
}

crow::response HealthController::healthCheck() {
    try {
        auto app = Application::getInstance();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - app->getStartTime()
        ).count();
        
        crow::json::wvalue response;
        response["status"] = "healthy";
        response["timestamp"] = std::time(nullptr);
        response["uptime_seconds"] = uptime;
        response["service"] = "YachiyoCPP";
        response["version"] = "1.0.0";
        
        return crow::response(200, response);
    } catch (const std::exception& e) {
        crow::json::wvalue error;
        error["status"] = "unhealthy";
        error["error"] = e.what();
        error["timestamp"] = std::time(nullptr);
        
        return crow::response(503, error);
    }
}

crow::response HealthController::detailedHealthCheck() {
    try {
        auto app = Application::getInstance();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - app->getStartTime()
        ).count();
        
        // 模拟检查各个组件
        bool databaseHealthy = true; // 实际项目中应该检查数据库连接
        bool redisHealthy = true;    // 实际项目中应该检查Redis连接
        bool aiServiceHealthy = true; // 实际项目中应该检查AI服务
        
        crow::json::wvalue response;
        response["status"] = "healthy";
        response["timestamp"] = std::time(nullptr);
        response["uptime_seconds"] = uptime;
        
        // 组件健康状态
        crow::json::wvalue components;
        components["database"] = databaseHealthy ? "healthy" : "unhealthy";
        components["redis"] = redisHealthy ? "healthy" : "unhealthy";
        components["ai_service"] = aiServiceHealthy ? "healthy" : "unhealthy";
        components["http_server"] = "healthy";
        response["components"] = std::move(components);
        
        // 系统信息
        crow::json::wvalue system;
        system["service"] = "YachiyoCPP";
        system["version"] = "1.0.0";
        system["environment"] = "development"; // 实际项目中应该从配置读取
        system["hostname"] = "localhost"; // 实际项目中应该获取真实主机名
        response["system"] = std::move(system);
        
        return crow::response(200, response);
    } catch (const std::exception& e) {
        crow::json::wvalue error;
        error["status"] = "unhealthy";
        error["error"] = e.what();
        error["timestamp"] = std::time(nullptr);
        
        return crow::response(503, error);
    }
}

crow::response HealthController::readinessCheck() {
    try {
        // 检查应用是否准备好接收流量
        // 这里可以添加更复杂的就绪检查逻辑
        
        crow::json::wvalue response;
        response["status"] = "ready";
        response["timestamp"] = std::time(nullptr);
        response["checks_passed"] = true;
        
        return crow::response(200, response);
    } catch (const std::exception& e) {
        crow::json::wvalue error;
        error["status"] = "not_ready";
        error["error"] = e.what();
        error["timestamp"] = std::time(nullptr);
        
        return crow::response(503, error);
    }
}

crow::response HealthController::livenessCheck() {
    try {
        // 检查应用是否存活
        // 这里可以添加更复杂的存活检查逻辑
        
        crow::json::wvalue response;
        response["status"] = "alive";
        response["timestamp"] = std::time(nullptr);
        
        return crow::response(200, response);
    } catch (const std::exception& e) {
        crow::json::wvalue error;
        error["status"] = "dead";
        error["error"] = e.what();
        error["timestamp"] = std::time(nullptr);
        
        return crow::response(503, error);
    }
}

crow::response HealthController::metrics() {
    try {
        auto app = Application::getInstance();
        auto now = std::chrono::system_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
            now - app->getStartTime()
        ).count();

        crow::json::wvalue metrics;

        // 系统指标
        crow::json::wvalue system;
        system["uptime_seconds"] = static_cast<int64_t>(uptime);
        system["start_time"] = static_cast<int64_t>(
            std::chrono::system_clock::to_time_t(app->getStartTime()));
        metrics["system"] = std::move(system);

        // WebSocket 连接数 (真实数据)
        crow::json::wvalue ws;
        ws["active_connections"] = static_cast<int>(
            ::g_webSocketService ? ::g_webSocketService->getClientCount() : 0);
        metrics["websocket"] = std::move(ws);

        // 内存指标 — 从 /proc/self/status 读取真实值
        crow::json::wvalue memory;
        long vmRSS_kb = 0, vmSize_kb = 0;
        std::ifstream statusFile("/proc/self/status");
        std::string line;
        while (std::getline(statusFile, line)) {
            if (line.rfind("VmRSS:", 0) == 0) {
                std::istringstream iss(line.substr(6));
                iss >> vmRSS_kb;
            } else if (line.rfind("VmSize:", 0) == 0) {
                std::istringstream iss(line.substr(7));
                iss >> vmSize_kb;
            }
            if (vmRSS_kb > 0 && vmSize_kb > 0) break;
        }
        memory["used_mb"] = static_cast<double>(vmRSS_kb) / 1024.0;
        memory["virtual_mb"] = static_cast<double>(vmSize_kb) / 1024.0;
        metrics["memory"] = std::move(memory);

        return crow::response(200, metrics);
    } catch (const std::exception& e) {
        crow::json::wvalue error;
        error["error"] = e.what();
        error["timestamp"] = static_cast<int64_t>(std::time(nullptr));
        return crow::response(500, error);
    }
}

crow::response HealthController::versionInfo() {
    try {
        crow::json::wvalue version;
        
        // 应用版本信息
        version["name"] = "YachiyoCPP";
        version["version"] = "1.0.0";
        version["build_date"] = __DATE__;
        version["build_time"] = __TIME__;
        
        // 依赖版本（模拟）
        crow::json::wvalue dependencies;
        dependencies["crow"] = "1.0+5";
        dependencies["spdlog"] = "1.11.0";
        dependencies["nlohmann_json"] = "3.10.5";
        version["dependencies"] = std::move(dependencies);
        
        // 编译信息
        crow::json::wvalue build;
        build["compiler"] = "g++";
        build["cxx_standard"] = "20";
        build["build_type"] = "Debug"; // 实际项目中应该从配置读取
        version["build"] = std::move(build);
        
        return crow::response(200, version);
    } catch (const std::exception& e) {
        crow::json::wvalue error;
        error["error"] = e.what();
        error["timestamp"] = std::time(nullptr);
        
        return crow::response(500, error);
    }
}

std::string HealthController::getBasePath() const {
    return "/api/v1/health";
}

std::string HealthController::getName() const {
    return "HealthController";
}

} // namespace controllers
} // namespace yachiyo
