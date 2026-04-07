#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <nlohmann/json.hpp>
#include "utils/Result.hpp"
#include "dto/CommonDTO.hpp"
#include "services/AvatarResponseService.hpp"

namespace yachiyo::controllers {

using json = nlohmann::json;
using Yachiyo::Utils::Result;

// ==================== WebSocket 消息类型 ====================

enum class WSMessageType {
    CONNECT,          // 客户端连接
    DISCONNECT,       // 客户端断开
    USER_MESSAGE,     // 用户消息
    SYSTEM_INFO,      // 系统信息
    HEARTBEAT,        // 心跳
    ERROR_RESPONSE,   // 错误响应
    AVATAR_RESPONSE   // Avatar 响应
};

struct WSMessage {
    WSMessageType type;
    std::string clientId;
    std::string sessionId;
    int64_t timestamp;
    json payload;
};

// ==================== WebSocket 控制器 ====================

class WebSocketController {
public:
    explicit WebSocketController(
        std::shared_ptr<services::AvatarResponseService> avatarService
    );
    ~WebSocketController();
    
    // ===== 连接管理 =====
    bool initialize();
    
    // 处理客户端连接
    std::string handleClientConnect(const std::string& clientId, const json& metadata);
    
    // 处理客户端断开
    void handleClientDisconnect(const std::string& clientId);
    
    // ===== 消息处理 =====
    Utils::Result<json> handleMessage(
        const std::string& clientId,
        const std::string& messageData
    );
    
    // 处理用户消息
    Utils::Result<json> processUserMessage(
        const std::string& clientId,
        const std::string& userId,
        const std::string& text,
        const std::string& targetLanguage = "zh-CN"
    );
    
    // 发送响应给客户端
    void broadcastResponse(
        const std::string& clientId,
        const services::AvatarResponseService::AvatarResponse& response
    );
    
    // ===== 心跳检测 =====
    void startHeartbeat();
    void stopHeartbeat();
    bool isClientAlive(const std::string& clientId);
    
    // ===== 连接信息 =====
    std::vector<std::string> getConnectedClients();
    int getConnectedClientCount() const;
    json getSessionInfo(const std::string& clientId);
    
    // ===== 错误处理 =====
    void broadcastError(
        const std::string& clientId,
        const std::string& errorCode,
        const std::string& errorMessage
    );
    
private:
    // 内部结构体
    struct ClientSession {
        std::string clientId;
        std::string sessionId;
        std::string userId;
        int64_t connectedAt;
        int64_t lastActivity;
        std::string language;
        bool isAlive;
    };
    
    // 成员变量
    std::shared_ptr<services::AvatarResponseService> avatar_service_;
    std::map<std::string, ClientSession> sessions_;
    std::mutex session_mutex_;
    
    bool is_running_;
    std::thread heartbeat_thread_;
    
    // ===== 私有方法 =====
    
    // 解析客户端消息
    Utils::Result<WSMessage> parseMessage(const std::string& messageData);
    
    // 生成响应 JSON
    json createResponse(
        WSMessageType type,
        const std::string& clientId,
        const json& payload
    );
    
    // 生成会话 ID
    std::string generateSessionId();
    
    // 心跳处理
    void heartbeatRoutine();
    void checkClientHealth();
    
    // 清理过期会话
    void cleanupExpiredSessions(int timeoutSeconds = 300);
    
    // 日志记录
    void logClientEvent(
        const std::string& clientId,
        const std::string& event,
        const json& details
    );
};

} // namespace yachiyo::controllers
