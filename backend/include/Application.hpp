#pragma once

#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <iostream>
#include <nlohmann/json.hpp>

// 前向声明
namespace yachiyo::config { class ConfigManager; }
namespace yachiyo::utils { class CrowHttpServer; class LogUtils; }
namespace spdlog { class logger; }

namespace yachiyo {

using json = nlohmann::json;

/**
 * @brief 应用程序类 - 管理整个应用的生命周期 (单例模式)
 */
class Application : public std::enable_shared_from_this<Application> {
public:
    Application();
    ~Application();

    /**
     * @brief 获取单例实例
     * @return 应用程序共享指针
     */
    static std::shared_ptr<Application> getInstance();

    /**
     * @brief 初始化应用程序
     * @param argc 命令行参数数量
     * @param argv 命令行参数数组
     * @return 初始化是否成功
     */
    bool initialize(int argc, char* argv[]);

    /**
     * @brief 启动应用程序
     * @return 启动是否成功
     */
    bool start();

    /**
     * @brief 等待应用程序运行结束
     */
    void wait();

    /**
     * @brief 停止应用程序
     */
    void stop();

    /**
     * @brief 获取应用程序版本
     * @return 版本字符串
     */
    static std::string getVersion() { return "1.0.0"; }

    /**
     * @brief 获取启动时间
     */
    std::chrono::system_clock::time_point getStartTime() const { return startTime; }

private:
    // 命令行参数结构
    struct Arguments {
        std::string configDir = "config";
        std::string environment = "dev";
        int port = 8080;
        std::string host = "0.0.0.0";
        int workers = 4;
    };

    /**
     * @brief 解析命令行参数
     */
    void parseArguments(int argc, char* argv[]);

    /**
     * @brief 显示帮助信息
     */
    void showHelp();

    /**
     * @brief 初始化HTTP服务器
     */
    void initializeHttpServer(const json& config);

    /**
     * @brief 初始化数据库连接
     */
    void initializeDatabase();

    /**
     * @brief 初始化AI服务
     */
    void initializeAIServices();

    /**
     * @brief 初始化其他服务 (WebSocket, Redis等)
     */
    void initializeServices();

    /**
     * @brief 初始化控制器
     */
    void initializeControllers();

    /**
     * @brief 注册信号处理器
     */
    void registerSignalHandlers();

    // 单例实例
    static std::shared_ptr<Application> instance;

    // 成员变量
    std::shared_ptr<config::ConfigManager> configManager;
    std::shared_ptr<utils::CrowHttpServer> httpServer;
    std::shared_ptr<spdlog::logger> logger;
    std::atomic<bool> running;
    Arguments arguments;
    std::chrono::system_clock::time_point startTime;
};

} // namespace yachiyo