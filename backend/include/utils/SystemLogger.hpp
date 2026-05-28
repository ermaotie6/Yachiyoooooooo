#pragma once

/**
 * SystemLogger — 轻量数据库运行日志工具
 *
 * 使用 g_databaseUtil (全局 DatabaseUtil 实例) 向 system_logs 和
 * websocket_logs 表写入运行时事件。所有写入使用参数化查询防止 SQL 注入。
 *
 * 使用方式:
 *   SystemLogger::info("OpenClawGateway", "AI response generated", {{"latency_ms", "350"}});
 *   SystemLogger::wsConnect("client_123", "web", "192.168.1.1", "Chrome/120");
 */

#include "utils/DatabaseUtil.hpp"
#include <string>
#include <vector>
#include <map>

// 全局 DatabaseUtil 实例 (在 Application.cpp 的全局命名空间中定义)
extern std::shared_ptr<Yachiyo::Utils::DatabaseUtil> g_databaseUtil;

namespace yachiyo::utils {

class SystemLogger {
public:
    static void log(const std::string& level,
                    const std::string& component,
                    const std::string& message,
                    const std::map<std::string, std::string>& details = {})
    {
        if (!::g_databaseUtil) return;
        try {
            std::string jsonDetails = "{";
            bool first = true;
            for (const auto& [k, v] : details) {
                if (!first) jsonDetails += ",";
                jsonDetails += "\"" + k + "\":\"" + v + "\"";
                first = false;
            }
            jsonDetails += "}";

            ::g_databaseUtil->execute(
                "INSERT INTO system_logs (level, component, message, details) "
                "VALUES ($1, $2, $3, $4)",
                {level, component, message, jsonDetails}
            );
        } catch (...) { /* 静默失败：日志记录不应影响主业务 */ }
    }

    static void info(const std::string& component, const std::string& message,
                     const std::map<std::string, std::string>& details = {})
    {
        log("INFO", component, message, details);
    }

    static void warn(const std::string& component, const std::string& message,
                     const std::map<std::string, std::string>& details = {})
    {
        log("WARNING", component, message, details);
    }

    static void error(const std::string& component, const std::string& message,
                      const std::map<std::string, std::string>& details = {})
    {
        log("ERROR", component, message, details);
    }

    static void wsEvent(const std::string& clientId,
                        const std::string& connectionType,
                        const std::string& ipAddress,
                        const std::string& status,
                        const std::string& errorMsg = "",
                        int64_t durationMs = 0)
    {
        if (!::g_databaseUtil) return;
        try {
            ::g_databaseUtil->execute(
                "INSERT INTO websocket_logs (client_id, connection_type, ip_address, "
                "status, error_message, duration_ms) "
                "VALUES ($1, $2, $3, $4, $5, $6)",
                {clientId, connectionType, ipAddress, status,
                 errorMsg, std::to_string(durationMs)}
            );
        } catch (...) { /* 静默 */ }
    }
};

} // namespace yachiyo::utils
