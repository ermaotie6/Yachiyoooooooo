#pragma once

#include <string>
#include <memory>
#include <vector>

namespace Yachiyo {

/**
 * @brief 应用程序类 - 管理整个应用的生命周期
 */
class Application {
public:
    Application();
    ~Application();

    /**
     * @brief 初始化应用程序
     * @param argc 命令行参数数量
     * @param argv 命令行参数数组
     * @return 初始化是否成功
     */
    bool initialize(int argc, char* argv[]);

    /**
     * @brief 运行应用程序
     * @return 应用程序退出码
     */
    int run();

    /**
     * @brief 停止应用程序
     */
    void stop();

    /**
     * @brief 获取应用程序版本
     * @return 版本字符串
     */
    static std::string getVersion();

private:
    /**
     * @brief 加载配置文件
     * @return 加载是否成功
     */
    bool loadConfig();

    /**
     * @brief 初始化数据库连接
     * @return 初始化是否成功
     */
    bool initDatabase();

    /**
     * @brief 初始化HTTP服务器
     * @return 初始化是否成功
     */
    bool initHttpServer();

    /**
     * @brief 初始化AI服务
     * @return 初始化是否成功
     */
    bool initAIService();

    /**
     * @brief 初始化缓存服务
     * @return 初始化是否成功
     */
    bool initCacheService();

    /**
     * @brief 清理资源
     */
    void cleanup();

    // 私有成员变量
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace Yachiyo