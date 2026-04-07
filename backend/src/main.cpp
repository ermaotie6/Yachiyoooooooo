#include "Application.hpp"
#include "utils/LogUtils.hpp"
#include <iostream>
#include <csignal>
#include <cstdlib>

using namespace yachiyo;
using LogUtils = yachiyo::utils::LogUtils;

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    try {
        // 获取应用程序实例
        auto app = Application::getInstance();
        
        // 初始化应用程序
        if (!app->initialize(argc, argv)) {
            std::cerr << "应用程序初始化失败" << std::endl;
            return EXIT_FAILURE;
        }
        
        // 启动应用程序
        if (!app->start()) {
            std::cerr << "应用程序启动失败" << std::endl;
            return EXIT_FAILURE;
        }
        
        // 等待应用程序运行
        app->wait();
        
        return EXIT_SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "应用程序异常: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

/**
 * @brief 信号处理器
 */
void signalHandler(int signal) {
    auto logger = LogUtils::getLogger("Main");
    
    switch (signal) {
        case SIGINT:
            logger->info("收到SIGINT信号，正在优雅关闭...");
            break;
        case SIGTERM:
            logger->info("收到SIGTERM信号，正在优雅关闭...");
            break;
        case SIGSEGV:
            logger->error("收到SIGSEGV信号，内存访问违规!");
            break;
        case SIGABRT:
            logger->error("收到SIGABRT信号，程序异常终止!");
            break;
        default:
            logger->warn("收到未知信号: {}", signal);
            break;
    }
    
    // 获取应用程序实例并停止
    auto app = Application::getInstance();
    app->stop();
    
    exit(signal);
}

/**
 * @brief 设置全局异常处理器
 */
void setupGlobalExceptionHandler() {
    std::set_terminate([]() {
        auto logger = LogUtils::getLogger("Main");
        logger->error("未捕获的异常，程序终止");
        
        try {
            // 重新抛出以获取异常信息
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& e) {
            logger->error("异常信息: {}", e.what());
        } catch (...) {
            logger->error("未知异常类型");
        }
        
        std::abort();
    });
}

/**
 * @brief 初始化信号处理器
 */
void setupSignalHandlers() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
    
    // 忽略 SIGPIPE (仅 POSIX 系统)
#ifdef SIGPIPE
    std::signal(SIGPIPE, SIG_IGN);
#endif
}

// 跨平台全局初始化 (使用静态对象的构造/析构代替 GCC __attribute__)
struct GlobalInitializer {
    GlobalInitializer() {
        // 设置全局异常处理器
        setupGlobalExceptionHandler();
        
        // 设置信号处理器
        setupSignalHandlers();
        
        // 初始化随机数种子
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }
    ~GlobalInitializer() {
        auto logger = LogUtils::getLogger("Main");
        logger->info("应用程序正在退出...");
    }
};
static GlobalInitializer g_initializer;