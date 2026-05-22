#include "handlers/WebSocketMessageHandler.hpp"
#include "core/ServiceRegistry.hpp"
#include "services/WebSocketService.hpp"
#include "services/AuthService.hpp"
#include "services/MessageService.hpp"
#include "controllers/WebSocketController.hpp"
#include "spdlog/spdlog.h"

#include <chrono>

namespace yachiyo::handlers {

using json = nlohmann::json;
using Yachiyo::Services::WebSocketService;

WebSocketMessageHandler::WebSocketMessageHandler(
    std::shared_ptr<WebSocketService> wsService,
    std::shared_ptr<yachiyo::core::PipelineContext> pipeline,
    std::shared_ptr<spdlog::logger> logger)
    : ws_(std::move(wsService)),
      pipeline_(std::move(pipeline)),
      log_(std::move(logger)) {
}

// ==================== 注册回调 ====================

void WebSocketMessageHandler::registerCallbacks() {
    if (!ws_) {
        log_->error("WebSocketMessageHandler: wsService 为空，无法注册回调");
        return;
    }

    if (!pipeline_ || !pipeline_->wsController) {
        log_->error("WebSocketMessageHandler: pipeline/wsController 为空");
        return;
    }

    // 注册连接/断开回调
    ws_->onClientConnect(
        [ctrl = pipeline_->wsController](int64_t clientId, const Yachiyo::Services::ClientMetadata& metadata) {
            json meta;
            meta["device_type"] = metadata.device_type;
            if (!metadata.extra_data.is_null()) {
                meta.merge_patch(metadata.extra_data);
            }
            ctrl->handleClientConnect(std::to_string(clientId), meta);
        });

    ws_->onClientDisconnect(
        [ctrl = pipeline_->wsController](int64_t clientId, const std::string& /*reason*/) {
            ctrl->handleClientDisconnect(std::to_string(clientId));
        });

    // 注册消息回调
    ws_->onMessageReceived(
        [this](int64_t clientId, const json& message) {
            this->handleUserMessage(clientId, message);
        });

    log_->info("WebSocket 消息处理器已注册");
}

// ==================== 消息处理 ====================

void WebSocketMessageHandler::handleUserMessage(int64_t clientId, const json& message) {
    try {
        // 1. 认证用户
        std::string userId = authenticateAndGetUserId(clientId, message);
        if (userId.empty()) return;  // 认证失败，已发送错误

        // 2. 提取消息内容
        if (!message.contains("data") || !message["data"].contains("content")) {
            return;  // 非用户消息，静默忽略
        }

        std::string text = message["data"]["content"].get<std::string>();
        if (text.empty()) return;

        // 3. 提取元数据
        std::string language = "ja";
        if (message["data"].contains("language")) {
            language = message["data"]["language"].get<std::string>();
        }

        std::string senderName;
        if (message["data"].contains("username")) {
            senderName = message["data"]["username"].get<std::string>();
        }
        if (senderName.empty()) {
            senderName = userId.empty() ? "匿名用户" : userId;
        }

        // 4. 安全审查 + 持久化
        if (!moderateAndPersist(clientId, userId, text, senderName, "ws-client")) {
            return;  // 被拦截
        }

        // 5. 广播用户消息
        broadcastUserMessage(userId, senderName, text);

        // 6. 调用 Avatar 管线
        processAvatarPipeline(clientId, userId, text, language);

    } catch (const std::exception& e) {
        log_->error("WebSocket 消息处理异常: {}", e.what());
        sendError(clientId, std::string("内部错误: ") + e.what(), "INTERNAL_ERROR");
    }
}

// ==================== 认证 ====================

std::string WebSocketMessageHandler::authenticateAndGetUserId(
    int64_t clientId, const nlohmann::json& message) {

    std::string userId;

    // 从消息中提取 client 声称的 userId
    if (message.contains("data") && message["data"].contains("user_id")) {
        userId = message["data"]["user_id"].get<std::string>();
    }

    // 如果有 JWT access_token，用它验证并覆盖 userId（防伪造）
    if (message.contains("data") && message["data"].contains("access_token")) {
        std::string token = message["data"]["access_token"].get<std::string>();
        if (!token.empty() && pipeline_->authService) {
            int64_t tokenUserId = pipeline_->authService->getUserIdFromToken(token);
            if (tokenUserId <= 0) {
                sendError(clientId, "认证失败：令牌无效或已过期", "AUTH_FAILED");
                return "";
            }
            userId = std::to_string(tokenUserId);
        }
    }

    return userId;
}

// ==================== 审查 ====================

bool WebSocketMessageHandler::moderateAndPersist(
    int64_t clientId, const std::string& userId,
    const std::string& text, const std::string& senderName,
    const std::string& userIp) {

    int64_t numericUserId = 0;
    try { if (!userId.empty()) numericUserId = std::stoll(userId); } catch (...) {}

    if (!pipeline_->messageService || numericUserId <= 0) {
        return true;  // 无审查服务或不合法 userId，跳过审查
    }

    auto msgResult = pipeline_->messageService->sendMessage(numericUserId, text, userIp, "websocket");

    if (!msgResult.isSuccess()) {
        sendError(clientId, msgResult.getErrorMsg(), "MSG_REJECTED");
        return false;
    }

    auto savedMsg = msgResult.value();
    auto reviewStatus = savedMsg->getReviewStatus();

    if (reviewStatus == yachiyo::models::ReviewStatus::REJECTED) {
        sendError(clientId, "消息未通过内容审核", "MSG_BLOCKED");
        return false;
    }

    if (reviewStatus == yachiyo::models::ReviewStatus::MANUAL_REVIEW) {
        // 广播但等待审核
        broadcastUserMessage(userId, senderName, text);
        json notice = {
            {"type", "status"},
            {"data", {{"status", "pending_review"}, {"message", "消息已提交，待审核"}}}
        };
        ws_->sendToClient(clientId, notice);
        return false;
    }

    return true;
}

// ==================== 广播 ====================

void WebSocketMessageHandler::broadcastUserMessage(
    const std::string& userId, const std::string& senderName,
    const std::string& text) {

    json broadcast = {
        {"type", "user_broadcast"},
        {"data", {
            {"sender_id", userId},
            {"sender_name", senderName},
            {"content", text},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()}
        }}
    };

    auto clients = ws_->getClients();
    for (const auto& client : clients) {
        if (client.is_active) {
            ws_->sendToClient(client.client_id, broadcast);
        }
    }
}

// ==================== Avatar 管线 ====================

void WebSocketMessageHandler::processAvatarPipeline(
    int64_t clientId, const std::string& userId,
    const std::string& text, const std::string& language) {

    std::string clientIdStr = std::to_string(clientId);
    auto result = pipeline_->wsController->processUserMessage(
        clientIdStr, userId, text, language);

    if (!result.isSuccess()) {
        sendError(clientId, "处理消息失败", result.getError().code);
        return;
    }

    // 发送给请求者
    ws_->sendToClient(clientId, result.getValue());

    // 广播给其他观众
    json broadcastAvatar = result.getValue();
    broadcastAvatar["data"]["sender_client_id"] = clientId;
    auto clients = ws_->getClients();
    for (const auto& client : clients) {
        if (client.client_id != clientId && client.is_active) {
            ws_->sendToClient(client.client_id, broadcastAvatar);
        }
    }
}

// ==================== 错误 ====================

void WebSocketMessageHandler::sendError(
    int64_t clientId, const std::string& message, const std::string& code) {
    json error = {
        {"type", "error"},
        {"data", {{"message", message}, {"code", code}}}
    };
    ws_->sendToClient(clientId, error);
}

} // namespace yachiyo::handlers
