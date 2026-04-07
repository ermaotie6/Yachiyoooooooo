#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>

namespace Yachiyo {
namespace Utils {

/**
 * @brief 日志器类
 * 
 * 提供简单的日志记录接口，包装 spdlog
 */
class Logger {
public:
    /**
     * @brief 构造函数
     * @param name 日志器名称
     */
    explicit Logger(const std::string& name);
    
    /**
     * @brief 获取默认日志器
     * @return 默认日志器
     */
    static std::shared_ptr<Logger> getDefault();
    
    /**
     * @brief 获取指定名称的日志器
     * @param name 日志器名称
     * @return 日志器实例
     */
    static std::shared_ptr<Logger> getLogger(const std::string& name);
    
    /**
     * @brief 设置日志级别
     * @param level 日志级别字符串: trace, debug, info, warn, error, critical
     */
    void setLevel(const std::string& level);
    
    /**
     * @brief 记录跟踪级别日志
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void trace(const char* format, Args&&... args) {
        logger_->trace(format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 记录调试级别日志
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void debug(const char* format, Args&&... args) {
        logger_->debug(format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 记录信息级别日志
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void info(const char* format, Args&&... args) {
        logger_->info(format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 记录警告级别日志
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void warn(const char* format, Args&&... args) {
        logger_->warn(format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 记录错误级别日志
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void error(const char* format, Args&&... args) {
        logger_->error(format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 记录严重级别日志
     * @param format 格式字符串
     * @param args 参数
     */
    template<typename... Args>
    void critical(const char* format, Args&&... args) {
        logger_->critical(format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 刷新日志
     */
    void flush() {
        logger_->flush();
    }
    
    /**
     * @brief 获取底层 spdlog 日志器
     * @return spdlog 日志器
     */
    std::shared_ptr<spdlog::logger> getSpdLogger() const {
        return logger_;
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
    
    // 将日志级别字符串转换为 spdlog 级别
    static spdlog::level::level_enum stringToLevel(const std::string& level);
    
    // 初始化默认日志器
    static void initializeDefaultLogger();
    
    static std::shared_ptr<Logger> defaultLogger;
    static bool defaultInitialized;
};

} // namespace Utils
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::utils {
    using Logger = Yachiyo::Utils::Logger;
}

// ==================== 便捷日志宏 ====================
// 这些宏使用 spdlog 的默认日志器，支持 fmt 格式化
// 用法: LOG_INFO("消息: {}", variable);

#define LOG_TRACE(...)   SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...)   SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...)    SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)    SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...)   SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)