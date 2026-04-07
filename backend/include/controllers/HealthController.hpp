#pragma once

#include "BaseController.hpp"
#include <crow.h>
#include <memory>
#include <chrono>

namespace yachiyo {
namespace controllers {

/**
 * @brief 健康检查控制器
 * 
 * API端点:
 * - GET /api/health           - 基础健康检查
 * - GET /api/health/detailed  - 详细健康信息
 * - GET /api/health/ready     - 就绪检查
 * - GET /api/health/live      - 存活检查
 * - GET /api/health/metrics   - 指标信息
 * - GET /api/health/version   - 版本信息
 */
class HealthController : public BaseController {
public:
    HealthController();
    ~HealthController() override;
    
    void registerRoutes(crow::SimpleApp& app);
    
    std::string getBasePath() const override;
    std::string getName() const override;

private:
    crow::response healthCheck();
    crow::response detailedHealthCheck();
    crow::response readinessCheck();
    crow::response livenessCheck();
    crow::response metrics();
    crow::response versionInfo();
};

} // namespace controllers
} // namespace yachiyo