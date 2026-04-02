#include "../../include/utils/LogUtils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <map>
#include <mutex>

namespace Yachiyo {
namespace Utils {

// 全局日志记录器映射
static std::map<std::string, std::shared_ptr<Logger>> g_loggers;
static std::mutex g_logger_mutex;

// 日志级别字符串
static const char* LOG_LEVEL_NAMES[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

// 颜色代码（用于控制台输出）
static const char* LOG_COLORS[] = {
    "\033[36m",  // DEBUG - 青色
    "\033[32m",  // INFO - 绿色
    "\033[33m",  // WARN - 黄色
    "\033[31m",  // ERROR - 红色
    "\033[35m"   // FATAL - 品红色
};
static const char* RESET_COLOR = "\033[0m";

/**
 * @brief Logger 实现类
 */
class LoggerImpl : public Logger {
private:
    std::string name;
    LogLevel minLevel;
    std::ofstream logFile;
    std::mutex logMutex;
    bool enableConsole;
    bool enableFile;

public:
    LoggerImpl(const std::string& name, LogLevel level = LogLevel::INFO)
        : name(name), minLevel(level), enableConsole(true), enableFile(true) {
    }

    ~LoggerImpl() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void setLogFile(const std::string& filePath) override {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            logFile.close();
        }
        logFile.open(filePath, std::ios::app);
    }

    void setMinLevel(LogLevel level) override {
        minLevel = level;
    }

    void setConsoleOutput(bool enable) override {
        enableConsole = enable;
    }

    void setFileOutput(bool enable) override {
        enableFile = enable;
    }

    void debug(const std::string& message) override {
        log(LogLevel::DEBUG, message);
    }

    void info(const std::string& message) override {
        log(LogLevel::INFO, message);
    }

    void warn(const std::string& message) override {
        log(LogLevel::WARN, message);
    }

    void error(const std::string& message) override {
        log(LogLevel::ERROR, message);
    }

    void fatal(const std::string& message) override {
        log(LogLevel::FATAL, message);
    }

private:
    void log(LogLevel level, const std::string& message) {
        if (level < minLevel) {
            return;
        }

        std::lock_guard<std::mutex> lock(logMutex);

        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream timeStream;
        timeStream << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
                   << "." << std::setfill('0') << std::setw(3) << ms.count();

        std::string logEntry = "[" + timeStream.str() + "] [" + name + "] [" +
                              LOG_LEVEL_NAMES[static_cast<int>(level)] + "] " +
                              message;

        // 输出到控制台
        if (enableConsole) {
            std::cout << LOG_COLORS[static_cast<int>(level)]
                     << logEntry
                     << RESET_COLOR
                     << std::endl;
        }

        // 输出到文件
        if (enableFile && logFile.is_open()) {
            logFile << logEntry << std::endl;
            logFile.flush();
        }
    }
};

/**
 * @brief 获取或创建日志记录器
 */
std::shared_ptr<Logger> LogUtils::getLogger(const std::string& name) {
    std::lock_guard<std::mutex> lock(g_logger_mutex);

    auto it = g_loggers.find(name);
    if (it != g_loggers.end()) {
        return it->second;
    }

    // 创建新的日志记录器
    auto logger = std::make_shared<LoggerImpl>(name, LogLevel::INFO);
    g_loggers[name] = logger;

    return logger;
}

/**
 * @brief 配置全局日志文件
 */
void LogUtils::configureGlobalLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(g_logger_mutex);

    for (auto& pair : g_loggers) {
        pair.second->setLogFile(filePath);
    }
}

/**
 * @brief 设置全局日志级别
 */
void LogUtils::setGlobalLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(g_logger_mutex);

    for (auto& pair : g_loggers) {
        pair.second->setMinLevel(level);
    }
}

/**
 * @brief 启用或禁用全局控制台输出
 */
void LogUtils::enableGlobalConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(g_logger_mutex);

    for (auto& pair : g_loggers) {
        pair.second->setConsoleOutput(enable);
    }
}

/**
 * @brief 启用或禁用全局文件输出
 */
void LogUtils::enableGlobalFileOutput(bool enable) {
    std::lock_guard<std::mutex> lock(g_logger_mutex);

    for (auto& pair : g_loggers) {
        pair.second->setFileOutput(enable);
    }
}

/**
 * @brief 清理所有日志记录器
 */
void LogUtils::clearAllLoggers() {
    std::lock_guard<std::mutex> lock(g_logger_mutex);
    g_loggers.clear();
}

} // namespace Utils
} // namespace Yachiyo
