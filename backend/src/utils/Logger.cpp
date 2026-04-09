#include "utils/Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Yachiyo {
namespace Utils {

// 静态成员初始化
std::shared_ptr<Logger> Logger::defaultLogger = nullptr;
bool Logger::defaultInitialized = false;

Logger::Logger(const std::string& name) {
    // 先查找已有的 spdlog logger
    logger_ = spdlog::get(name);
    if (!logger_) {
        // 创建一个新的彩色控制台 logger
        logger_ = spdlog::stdout_color_mt(name);
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
    }
}

std::shared_ptr<Logger> Logger::getDefault() {
    if (!defaultInitialized) {
        initializeDefaultLogger();
    }
    return defaultLogger;
}

std::shared_ptr<Logger> Logger::getLogger(const std::string& name) {
    return std::make_shared<Logger>(name);
}

void Logger::setLevel(const std::string& level) {
    logger_->set_level(stringToLevel(level));
}

spdlog::level::level_enum Logger::stringToLevel(const std::string& level) {
    if (level == "trace")    return spdlog::level::trace;
    if (level == "debug")    return spdlog::level::debug;
    if (level == "info")     return spdlog::level::info;
    if (level == "warn")     return spdlog::level::warn;
    if (level == "warning")  return spdlog::level::warn;
    if (level == "error")    return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    if (level == "off")      return spdlog::level::off;
    return spdlog::level::info;
}

void Logger::initializeDefaultLogger() {
    defaultLogger = std::make_shared<Logger>("default");
    defaultInitialized = true;
}

} // namespace Utils
} // namespace Yachiyo
