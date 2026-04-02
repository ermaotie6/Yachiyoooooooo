#include "controllers/HealthController.hpp"
#include "Application.hpp"
#include "utils/JsonUtils.hpp"
#include <chrono>
#include <ctime>

namespace Yachiyo {
namespace controllers {

HealthController::HealthController() {
    // 初始化健康检查控制器
}

HealthController::~HealthController() {
    // 清理资源
}

void HealthController::registerRoutes(crow::SimpleApp& app) {
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
        // 收集和返回应用指标
        // 实际项目中应该收集更详细的指标
        
        auto app = Application::getInstance();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - app->getStartTime()
        ).count();
        
        crow::json::wvalue metrics;
        
        // 系统指标
        crow::json::wvalue system;
        system["uptime_seconds"] = uptime;
        system["start_time"] = std::chrono::system_clock::to_time_t(app->getStartTime());
        metrics["system"] = std::move(system);
        
        // HTTP指标（模拟）
        crow::json::wvalue http;
        http["total_requests"] = 0; // 实际项目中应该统计请求数
        http["active_connections"] = 0;
        http["requests_per_second"] = 0.0;
        metrics["http"] = std::move(http);
        
        // 内存指标（模拟）
        crow::json::wvalue memory;
        memory["used_mb"] = 0; // 实际项目中应该获取真实内存使用
        memory["total_mb"] = 0;
        memory["free_mb"] = 0;
        metrics["memory"] = std::move(memory);
        
        return crow::response(200, metrics);
    } catch (const std::exception& e) {
        crow::json::wvalue error;
        error["error"] = e.what();
        error["timestamp"] = std::time(nullptr);
        
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
    return "/api/health";
}

std::string HealthController::getName() const {
    return "HealthController";
}

} // namespace controllers
} // namespace Yachiyo