#include "controllers/WebSocketController.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <chrono>
#include <uuid/uuid.h>
#include <iomanip>
#include <sstream>

namespace yachiyo::controllers {

// ==================== UUID 生成辅助函数 ====================

static std::string generateUUID() {
    uuid_t uuid;
    uuid_generate(uuid);
    
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

// ==================== 构造/析构 ====================

WebSocketController::WebSocketController(
    std::shared_ptr<services::AvatarResponseService> avatarService
) 
    : avatar_service_(avatarService),
      is_running_(false) {
}

WebSocketController::~WebSocketController() {
    stopHeartbeat();
}

// ==================== 初始化 ====================

bool WebSocketController::initialize() {
    LOG_INFO("初始化 WebSocket 控制器");
    
    if (!avatar_service_) {
        LOG_ERROR("Avatar 服务未配置");
        return false;
    }
    
    startHeartbeat();
    
    LOG_INFO("WebSocket 控制器初始化完成");
    return true;
}

// ==================== 连接管理 ====================

std::string WebSocketController::handleClientConnect(
    const std::string& clientId,
    const json& metadata
) {
    LOG_INFO("客户端连接: {}", clientId);
    
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    ClientSession session;
    session.clientId = clientId;
    session.sessionId = generateSessionId();
    session.connectedAt = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    session.lastActivity = session.connectedAt;
    session.isAlive = true;
    
    // 从元数据中提取信息
    if (metadata.contains("userId")) {
        session.userId = metadata["userId"].get<std::string>();
    }
    if (metadata.contains("language")) {
        session.language = metadata["language"].get<std::string>();
    } else {
        session.language = "zh-CN";
    }
    
    sessions_[clientId] = session;
    
    logClientEvent(clientId, "CONNECT", metadata);
    
    return session.sessionId;
}

void WebSocketController::handleClientDisconnect(const std::string& clientId) {
    LOG_INFO("客户端断开: {}", clientId);
    
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto it = sessions_.find(clientId);
    if (it != sessions_.end()) {
        json details = {
            {"connectedDuration", 
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now().time_since_epoch()).count() - it->second.connectedAt}
        };
        logClientEvent(clientId, "DISCONNECT", details);
        sessions_.erase(it);
    }
}

// ==================== 消息处理 ====================

Utils::Result<json> WebSocketController::handleMessage(
    const std::string& clientId,
    const std::string& messageData
) {
    LOG_DEBUG("处理客户端消息: {}", clientId);
    
    // 1. 解析消息
    auto parseResult = parseMessage(messageData);
    if (!parseResult.isSuccess()) {
        LOG_ERROR("消息解析失败: {}", parseResult.getError().message);
        return parseResult;
    }
    
    auto wsMessage = parseResult.getValue();
    
    // 2. 更新客户端活动时间
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        auto it = sessions_.find(clientId);
        if (it != sessions_.end()) {
            it->second.lastActivity = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
    }
    
    // 3. 根据消息类型处理
    json response;
    
    switch (wsMessage.type) {
        case WSMessageType::USER_MESSAGE: {
            std::string userId = wsMessage.userId;
            if (userId.empty()) {
                std::lock_guard<std::mutex> lock(session_mutex_);
                auto it = sessions_.find(clientId);
                if (it != sessions_.end()) {
                    userId = it->second.userId;
                }
            }
            
            if (wsMessage.payload.contains("text")) {
                std::string text = wsMessage.payload["text"].get<std::string>();
                auto targetLanguage = wsMessage.payload.value("language", "ja");
                
                auto result = processUserMessage(clientId, userId, text, targetLanguage);
                if (!result.isSuccess()) {
                    LOG_ERROR("处理用户消息失败: {}", result.getError().message);
                    return Utils::Result<json>::fail(5002, "处理用户消息失败");
                }
                response = result.getValue();
            }
            break;
        }
        
        case WSMessageType::HEARTBEAT: {
            response = createResponse(WSMessageType::HEARTBEAT, clientId, {
                {"status", "alive"},
                {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()}
            });
            break;
        }
        
        case WSMessageType::SYSTEM_INFO: {
            response = createResponse(WSMessageType::SYSTEM_INFO, clientId, 
                getSessionInfo(clientId));
            break;
        }
        
        default:
            LOG_WARN("未知消息类型: {}", static_cast<int>(wsMessage.type));
            return Utils::Result<json>::fail(4001, "未知消息类型");
    }
    
    return Utils::Result<json>::success(response);
}

Utils::Result<json> WebSocketController::processUserMessage(
    const std::string& clientId,
    const std::string& userId,
    const std::string& text,
    const std::string& targetLanguage
) {
    LOG_INFO("处理用户消息: clientId={}, userId={}", clientId, userId);
    
    if (!avatar_service_) {
        return Utils::Result<json>::fail(5001, "Avatar 服务不可用");
    }
    
    // 调用 Avatar 服务
    auto result = avatar_service_->processUserMessage(userId, text, targetLanguage);
    
    if (!result.isSuccess()) {
        LOG_ERROR("Avatar 处理失败: {}", result.getError().message);
        broadcastError(clientId, result.getError().code, result.getError().message);
        return Utils::Result<json>::fail(5002, "Avatar 处理失败");
    }
    
    auto avatarResponse = result.getValue();
    
    // 广播响应
    broadcastResponse(clientId, avatarResponse);
    
    // 返回成功的 JSON 响应（格式需与前端 useWebSocket.ts 的 avatar_response 处理匹配）
    json response = {
        {"type", "avatar_response"},
        {"data", {
            {"request_id", avatarResponse.requestId},
            {"text", avatarResponse.text},
            {"original_text", avatarResponse.originalText},
            {"audio_url", avatarResponse.audioUrl},
            {"audio_duration_ms", avatarResponse.audioDurationMs},
            {"emotions", json::array()},
            {"actions", json::array()},
            {"animation_commands", json::array()},
            {"processing_time_ms", avatarResponse.processingTimeMs},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()}
        }}
    };
    
    // 添加表情和动作
    for (const auto& emotion : avatarResponse.emotions) {
        response["data"]["emotions"].push_back(emotion);
    }
    
    for (const auto& action : avatarResponse.actions) {
        response["data"]["actions"].push_back(action);
    }

    // 添加动画命令
    for (const auto& cmd : avatarResponse.animationCommands) {
        response["data"]["animation_commands"].push_back(cmd.toJson());
    }
    
    return Utils::Result<json>::success(response);
}

void WebSocketController::broadcastResponse(
    const std::string& clientId,
    const services::AvatarResponseService::AvatarResponse& response
) {
    // 注意: 此方法仅记录日志，不执行实际的 WebSocket 广播。
    // 实际的消息推送（发送给发送者 + 广播给其他观众）由 Application.cpp 中的
    // onMessageReceived 回调通过 g_webSocketService->sendToClient() 和
    // 遍历 getClients() 完成。此方法作为 Controller 层的事件追踪点。
    LOG_DEBUG("Avatar 响应已生成 (clientId={}), 等待 Application 层推送", clientId);
    
    json payload = {
        {"requestId", response.requestId},
        {"text", response.text},
        {"audioUrl", response.audioUrl},
        {"emotions", response.emotions},
        {"actions", response.actions},
        {"processingTimeMs", response.processingTimeMs}
    };
    
    logClientEvent(clientId, "RESPONSE_GENERATED", payload);
}

// ==================== 心跳检测 ====================

void WebSocketController::startHeartbeat() {
    if (is_running_) {
        LOG_WARN("心跳已在运行");
        return;
    }
    
    is_running_ = true;
    heartbeat_thread_ = std::thread(&WebSocketController::heartbeatRoutine, this);
    
    LOG_INFO("心跳检测已启动");
}

void WebSocketController::stopHeartbeat() {
    if (!is_running_) {
        return;
    }
    
    is_running_ = false;
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    
    LOG_INFO("心跳检测已停止");
}

bool WebSocketController::isClientAlive(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    auto it = sessions_.find(clientId);
    return it != sessions_.end() && it->second.isAlive;
}

void WebSocketController::heartbeatRoutine() {
    while (is_running_) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        
        checkClientHealth();
        cleanupExpiredSessions(300);  // 5分钟超时
    }
}

void WebSocketController::checkClientHealth() {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (auto& entry : sessions_) {
        // 心跳超时：5分钟没有活动
        int inactiveSeconds = static_cast<int>((now - entry.second.lastActivity) / 1000);
        if (inactiveSeconds > 300) {
            entry.second.isAlive = false;
            LOG_WARN("客户端心跳超时: {}", entry.first);
        }
    }
}

// ==================== 连接信息 ====================

std::vector<std::string> WebSocketController::getConnectedClients() {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    std::vector<std::string> clients;
    for (const auto& entry : sessions_) {
        if (entry.second.isAlive) {
            clients.push_back(entry.first);
        }
    }
    
    return clients;
}

int WebSocketController::getConnectedClientCount() const {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    int count = 0;
    for (const auto& entry : sessions_) {
        if (entry.second.isAlive) {
            count++;
        }
    }
    
    return count;
}

json WebSocketController::getSessionInfo(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto it = sessions_.find(clientId);
    if (it == sessions_.end()) {
        return json{};
    }
    
    const auto& session = it->second;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return json{
        {"clientId", session.clientId},
        {"sessionId", session.sessionId},
        {"userId", session.userId},
        {"language", session.language},
        {"connectedAt", session.connectedAt},
        {"lastActivity", session.lastActivity},
        {"connectedDuration", (now - session.connectedAt) / 1000},
        {"isAlive", session.isAlive}
    };
}

// ==================== 错误处理 ====================

void WebSocketController::broadcastError(
    const std::string& clientId,
    const std::string& errorCode,
    const std::string& errorMessage
) {
    LOG_ERROR("向客户端广播错误: {} - {}", errorCode, errorMessage);
    
    json errorResponse = createResponse(
        WSMessageType::ERROR_RESPONSE,
        clientId,
        {
            {"errorCode", errorCode},
            {"errorMessage", errorMessage}
        }
    );
    
    logClientEvent(clientId, "ERROR", errorResponse);
}

// ==================== 私有方法 ====================

Utils::Result<json> WebSocketController::parseMessage(const std::string& messageData) {
    try {
        auto msg = json::parse(messageData);
        
        // 验证必要字段 —— 兼容前端格式 (type + data) 和控制器格式 (type + payload)
        if (!msg.contains("type")) {
            return Utils::Result<json>::fail(4000, "消息格式不完整: 缺少 type");
        }
        
        // 统一: 如果前端发送的是 'data'，映射为 'payload'
        if (!msg.contains("payload") && msg.contains("data")) {
            msg["payload"] = msg["data"];
        }
        if (!msg.contains("payload")) {
            msg["payload"] = json::object();
        }
        
        // 统一: 如果 payload 中有 'content' 但没有 'text'，映射为 'text'
        if (msg["payload"].contains("content") && !msg["payload"].contains("text")) {
            msg["payload"]["text"] = msg["payload"]["content"];
        }
        
        return Utils::Result<json>::success(msg);
    } catch (const std::exception& e) {
        LOG_ERROR("JSON 解析异常: {}", e.what());
        return Utils::Result<json>::fail(4001, "消息解析失败");
    }
}

json WebSocketController::createResponse(
    WSMessageType type,
    const std::string& clientId,
    const json& payload
) {
    return json{
        {"type", static_cast<int>(type)},
        {"clientId", clientId},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()},
        {"payload", payload}
    };
}

std::string WebSocketController::generateSessionId() {
    return generateUUID();
}

void WebSocketController::cleanupExpiredSessions(int timeoutSeconds) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::vector<std::string> keysToRemove;
    for (const auto& entry : sessions_) {
        int inactiveSeconds = static_cast<int>((now - entry.second.lastActivity) / 1000);
        if (inactiveSeconds > timeoutSeconds && !entry.second.isAlive) {
            keysToRemove.push_back(entry.first);
        }
    }
    
    for (const auto& key : keysToRemove) {
        LOG_INFO("移除过期会话: {}", key);
        sessions_.erase(key);
    }
}

void WebSocketController::logClientEvent(
    const std::string& clientId,
    const std::string& event,
    const json& details
) {
    LOG_DEBUG("客户端事件: {} - {} - {}", clientId, event, details.dump());
}

} // namespace yachiyo::controllers
