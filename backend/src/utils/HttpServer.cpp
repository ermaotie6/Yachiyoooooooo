#include "utils/HttpServer.hpp"
#include "controllers/BaseController.hpp"
#include <crow.h>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace yachiyo {
namespace utils {

CrowHttpServer::CrowHttpServer() = default;

CrowHttpServer::~CrowHttpServer() {
    stop();
}

void CrowHttpServer::registerController(const std::string& basePath,
                                        std::shared_ptr<controllers::BaseController> controller) {
    std::lock_guard<std::mutex> lock(mutex_);
    controllers_.emplace_back(basePath, controller);
}

bool CrowHttpServer::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_) {
        return true;
    }

    try {
        // 配置 Crow 应用
        app_.loglevel(crow::LogLevel::Warning);
        app_.port(port_).bindaddr(host_);
        app_.concurrency(workers_);

        // 注册所有控制器路由
        for (auto& [basePath, controller] : controllers_) {
            controller->registerRoutes(app_);
        }

        // 在后台线程中启动 Crow
        serverThread_ = std::thread([this]() {
            try {
                app_.run();
            } catch (const std::exception& e) {
                std::cerr << "HTTP server error: " << e.what() << std::endl;
            }
        });

        // 等待服务器启动
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        running_ = true;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to start HTTP server: " << e.what() << std::endl;
        running_ = false;
        return false;
    }
}

void CrowHttpServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    app_.stop();

    if (serverThread_.joinable()) {
        serverThread_.join();
    }
}

void CrowHttpServer::wait() {
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
}

} // namespace utils
} // namespace yachiyo