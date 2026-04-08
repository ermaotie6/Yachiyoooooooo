#include "Application.hpp"
#include "utils/LogUtils.hpp"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <atomic>

using namespace yachiyo;
using LogUtils = yachiyo::utils::LogUtils;

// 全局原子变量，用于信号处理器 (async-signal-safe)
static volatile std::sig_atomic_t g_signalReceived = 0;

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
        
        // 等待应用程序运行（同时检查信号标志）
        while (!g_signalReceived) {
            // 短暂休眠，避免忙等待
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!app->isRunning()) break;
        }
        
        // 主线程中安全地执行清理（可以调用非 async-signal-safe 函数）
        if (g_signalReceived) {
            auto logger = LogUtils::getLogger("Main");
            logger->info("收到信号 {}，正在优雅关闭...", static_cast<int>(g_signalReceived));
            app->stop();
        }
        
        return EXIT_SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "应用程序异常: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

/**
 * @brief 信号处理器 (async-signal-safe)
 * @note 仅设置原子标志，由主线程执行实际清理
 */
void signalHandler(int signal) {
    // 对于致命信号，重置为默认处理器后重新触发
    if (signal == SIGSEGV || signal == SIGABRT) {
        std::signal(signal, SIG_DFL);
        std::raise(signal);
        return;
    }
    // 对于 SIGINT/SIGTERM，设置标志让主线程处理
    g_signalReceived = signal;
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