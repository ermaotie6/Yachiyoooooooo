#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace yachiyo::utils {

/**
 * @brief 日志工具类
 * 
 * 提供统一的日志记录功能，支持控制台和文件输出
 */
class LogUtils {
public:
    /**
     * @brief 初始化日志系统
     * @param logLevel 日志级别: trace, debug, info, warn, error, critical
     * @param logFile 日志文件路径（可选）
     * @return 是否初始化成功
     */
    static bool initialize(const std::string& logLevel = "info", 
                          const std::string& logFile = "");
    
    /**
     * @brief 获取指定名称的日志器
     * @param name 日志器名称
     * @return 共享指针指向的日志器
     */
    static std::shared_ptr<spdlog::logger> getLogger(const std::string& name);
    
    /**
     * @brief 设置全局日志级别
     * @param level 日志级别
     */
    static void setGlobalLevel(const std::string& level);
    
    /**
     * @brief 刷新所有日志
     */
    static void flushAll();
    
    /**
     * @brief 关闭日志系统
     */
    static void shutdown();

private:
    static bool initialized;
    static std::string currentLogFile;
    
    // 将日志级别字符串转换为 spdlog 级别
    static spdlog::level::level_enum stringToLevel(const std::string& level);
    
    // 创建控制台 sink
    static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> createConsoleSink();
    
    // 创建文件 sink
    static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> createFileSink(const std::string& filePath);
};

} // namespace yachiyo::utils