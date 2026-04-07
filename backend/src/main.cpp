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
 * @note 必须处理应用尚未初始化的情况，避免空指针解引用导致递归崩溃
 */
void signalHandler(int signal) {
    // 对于致命信号 (SIGSEGV/SIGABRT)，先重置为默认处理器，
    // 防止 signalHandler 自身再次触发同一信号时无限递归
    if (signal == SIGSEGV || signal == SIGABRT) {
        std::signal(signal, SIG_DFL);
    }
    
    // 尝试记录日志（容忍日志系统不可用的情况）
    try {
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
    } catch (...) {
        // 日志系统不可用，写 stderr 作为最后手段
        std::cerr << "收到信号: " << signal << std::endl;
    }
    
    // 获取应用程序实例并停止（防御空指针）
    try {
        auto app = Application::getInstance();
        if (app) {
            app->stop();
        }
    } catch (...) {
        // 应用实例不可用，忽略
    }
    
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