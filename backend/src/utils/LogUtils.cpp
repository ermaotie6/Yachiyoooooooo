#include "utils/LogUtils.hpp"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace yachiyo::utils {

// 静态成员初始化
bool LogUtils::initialized = false;
std::string LogUtils::currentLogFile;

bool LogUtils::initialize(const std::string& logLevel, const std::string& logFile) {
    if (initialized) {
        return true;
    }
    
    try {
        auto level = stringToLevel(logLevel);
        
        // 设置全局日志级别
        spdlog::set_level(level);
        
        // 设置日志格式
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
        
        // 如果指定了日志文件，创建文件 sink
        if (!logFile.empty()) {
            currentLogFile = logFile;
            
            // 创建一个同时输出到控制台和文件的默认 logger
            auto consoleSink = createConsoleSink();
            auto fileSink = createFileSink(logFile);
            
            auto defaultLogger = std::make_shared<spdlog::logger>(
                "default", 
                spdlog::sinks_init_list{consoleSink, fileSink}
            );
            defaultLogger->set_level(level);
            spdlog::set_default_logger(defaultLogger);
        }
        
        initialized = true;
        spdlog::info("日志系统初始化完成 (级别: {}, 文件: {})", 
                    logLevel, logFile.empty() ? "无" : logFile);
        return true;
        
    } catch (const spdlog::spdlog_ex& e) {
        std::cerr << "日志系统初始化失败: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<spdlog::logger> LogUtils::getLogger(const std::string& name) {
    // 先查找已有的 logger
    auto logger = spdlog::get(name);
    if (logger) {
        return logger;
    }
    
    // 创建新的 logger
    try {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(createConsoleSink());
        
        if (!currentLogFile.empty()) {
            sinks.push_back(createFileSink(currentLogFile));
        }
        
        logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        logger->set_level(spdlog::get_level());
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
        
        // 注册 logger 以便后续查找
        spdlog::register_logger(logger);
        
        return logger;
        
    } catch (const spdlog::spdlog_ex& e) {
        std::cerr << "创建 logger '" << name << "' 失败: " << e.what() << std::endl;
        // 返回默认 logger 作为 fallback
        return spdlog::default_logger();
    }
}

void LogUtils::setGlobalLevel(const std::string& level) {
    spdlog::set_level(stringToLevel(level));
}

void LogUtils::flushAll() {
    spdlog::apply_all([](std::shared_ptr<spdlog::logger> l) {
        l->flush();
    });
}

void LogUtils::shutdown() {
    spdlog::shutdown();
    initialized = false;
}

spdlog::level::level_enum LogUtils::stringToLevel(const std::string& level) {
    if (level == "trace")    return spdlog::level::trace;
    if (level == "debug")    return spdlog::level::debug;
    if (level == "info")     return spdlog::level::info;
    if (level == "warn")     return spdlog::level::warn;
    if (level == "warning")  return spdlog::level::warn;
    if (level == "error")    return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    if (level == "off")      return spdlog::level::off;
    
    // 默认 info
    return spdlog::level::info;
}

std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> LogUtils::createConsoleSink() {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_level(spdlog::level::trace); // sink 级别设为最低，由 logger 控制
    return sink;
}

std::shared_ptr<spdlog::sinks::basic_file_sink_mt> LogUtils::createFileSink(const std::string& filePath) {
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath, true);
    sink->set_level(spdlog::level::trace);
    return sink;
}

} // namespace yachiyo::utils
