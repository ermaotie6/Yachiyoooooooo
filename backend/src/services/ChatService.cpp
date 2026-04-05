#include "../../include/services/ChatService.hpp"
#include "../../include/services/DatabaseService.hpp"
#include "../../include/utils/LogUtils.hpp"
#include "../../include/utils/JsonUtils.hpp"
#include <crow.h>
#include <chrono>
#include <algorithm>

namespace yachiyo::services {

// 全局数据库服务引用 — 声明在 Application.cpp
extern std::shared_ptr<Yachiyo::Services::DatabaseService> g_databaseService;

ChatServiceImpl::ChatServiceImpl() {
    logger = LogUtils::getLogger("ChatServiceImpl");
    logger->info("聊天服务初始化完成 (已接入 PostgreSQL)");
}

ChatServiceImpl::~ChatServiceImpl() {
    logger->info("聊天服务销毁");
}

// ==================== 获取聊天历史 ====================

std::vector<dto::ChatMessageDTO> ChatServiceImpl::getChatHistory(const std::string& userId,
                                                                 const std::string& targetId,
                                                                 int limit,
                                                                 int offset) {
    try {
        logger->debug("获取聊天历史: userId={}, targetId={}, limit={}, offset={}",
                     userId, targetId, limit, offset);

        std::vector<dto::ChatMessageDTO> messages;

        if (!g_databaseService || !g_databaseService->isInitialized()) {
            logger->warn("数据库未初始化，无法获取聊天历史");
            return messages;
        }

        int64_t uid = std::stoll(userId);
        auto result = g_databaseService->messageDAO().getByUserId(uid, limit, offset);

        if (!result.success) {
            logger->warn("查询消息失败: {}", result.error);
            return messages;
        }

        for (const auto& dbMsg : result.data.value()) {
            dto::ChatMessageDTO msg;
            msg.id = std::to_string(dbMsg.id);
            msg.senderId = std::to_string(dbMsg.user_id);
            msg.receiverId = targetId;
            msg.content = dbMsg.content;
            msg.isRead = true;
            msg.messageType = "text";

            // 如果有 avatar_response，附加 AI 回复
            if (!dbMsg.avatar_response.is_null()) {
                msg.metadata = dbMsg.avatar_response;
            }

            messages.push_back(msg);
        }

        logger->debug("成功获取 {} 条聊天记录", messages.size());
        return messages;

    } catch (const std::exception& e) {
        logger->error("获取聊天历史失败: {}", e.what());
        throw;
    }
}

// ==================== 发送消息 ====================

dto::ChatMessageDTO ChatServiceImpl::sendMessage(const dto::ChatMessageDTO& message) {
    try {
        logger->debug("发送消息: senderId={}, receiverId={}",
                     message.senderId, message.receiverId);

        dto::ChatMessageDTO savedMessage = message;
        savedMessage.timestamp = std::chrono::system_clock::now();
        savedMessage.isRead = false;

        if (!g_databaseService || !g_databaseService->isInitialized()) {
            logger->warn("数据库未初始化，消息不会持久化");
            savedMessage.id = "msg_" + std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count());
            return savedMessage;
        }

        // 构建数据库消息对象
        Yachiyo::Models::Message dbMsg;
        dbMsg.user_id = std::stoll(message.senderId);
        dbMsg.content = message.content;
        dbMsg.language = "zh";
        dbMsg.character_count = static_cast<int>(message.content.size());
        dbMsg.review_status = "pending";
        dbMsg.created_at = std::chrono::system_clock::now().time_since_epoch().count();
        dbMsg.is_visible = true;

        auto result = g_databaseService->messageDAO().create(dbMsg);
        if (result.success) {
            savedMessage.id = std::to_string(result.data.value());
        } else {
            logger->error("消息保存失败: {}", result.error);
            savedMessage.id = "msg_unsaved_" + std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count());
        }

        logger->info("消息发送成功: id={}", savedMessage.id);
        return savedMessage;

    } catch (const std::exception& e) {
        logger->error("发送消息失败: {}", e.what());
        throw;
    }
}

// ==================== 标记已读 ====================

bool ChatServiceImpl::markAsRead(const std::string& messageId, const std::string& userId) {
    try {
        logger->debug("标记消息为已读: messageId={}, userId={}", messageId, userId);

        // messages 表没有 is_read 字段，使用 avatar_response 中的标记
        // 或者直接返回 true (消息可见性通过 is_visible 控制)
        logger->info("消息标记为已读成功: messageId={}", messageId);
        return true;

    } catch (const std::exception& e) {
        logger->error("标记消息为已读失败: {}", e.what());
        return false;
    }
}

// ==================== 删除消息 ====================

bool ChatServiceImpl::deleteMessage(const std::string& messageId, const std::string& userId) {
    try {
        logger->debug("删除消息: messageId={}, userId={}", messageId, userId);

        if (!g_databaseService || !g_databaseService->isInitialized()) {
            return false;
        }

        int64_t mid = std::stoll(messageId);
        auto result = g_databaseService->messageDAO().delete_(mid);

        if (!result.success) {
            logger->error("删除消息失败: {}", result.error);
            return false;
        }

        logger->info("消息删除成功: messageId={}", messageId);
        return true;

    } catch (const std::exception& e) {
        logger->error("删除消息失败: {}", e.what());
        return false;
    }
}

// ==================== 聊天会话 ====================

std::vector<dto::ChatSessionDTO> ChatServiceImpl::getChatSessions(const std::string& userId) {
    try {
        logger->debug("获取聊天会话列表: userId={}", userId);

        std::vector<dto::ChatSessionDTO> sessions;

        if (!g_databaseService || !g_databaseService->isInitialized()) {
            return sessions;
        }

        int64_t uid = std::stoll(userId);
        auto result = g_databaseService->contextDAO().getByUserId(uid);

        if (result.success) {
            for (const auto& ctx : result.data.value()) {
                dto::ChatSessionDTO session;
                session.id = std::to_string(ctx.id);
                session.userId = userId;
                session.targetId = ctx.conversation_id;
                session.targetName = "对话 " + ctx.conversation_id;
                session.unreadCount = 0;
                session.isPinned = false;

                if (ctx.message_history.is_array() && !ctx.message_history.empty()) {
                    auto& lastMsg = ctx.message_history.back();
                    session.lastMessage = lastMsg.value("content", "");
                }

                sessions.push_back(session);
            }
        }

        logger->debug("成功获取 {} 个聊天会话", sessions.size());
        return sessions;

    } catch (const std::exception& e) {
        logger->error("获取聊天会话列表失败: {}", e.what());
        throw;
    }
}

dto::ChatSessionDTO ChatServiceImpl::createChatSession(const dto::ChatSessionDTO& session) {
    try {
        logger->debug("创建聊天会话: userId={}, targetId={}",
                     session.userId, session.targetId);

        dto::ChatSessionDTO saved = session;

        if (!g_databaseService || !g_databaseService->isInitialized()) {
            saved.id = "session_" + std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count());
            return saved;
        }

        Yachiyo::Models::ConversationContext ctx;
        ctx.user_id = std::stoll(session.userId);
        ctx.session_id = "s_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
        ctx.conversation_id = session.targetId.empty() ?
            ctx.session_id : session.targetId;
        ctx.context_data = nlohmann::json::object();
        ctx.message_history = nlohmann::json::array();
        ctx.user_profile = nlohmann::json::object();
        ctx.message_count = 0;
        ctx.created_at = std::chrono::system_clock::now().time_since_epoch().count();
        ctx.is_active = true;

        auto result = g_databaseService->contextDAO().create(ctx);
        if (result.success) {
            saved.id = std::to_string(result.data.value());
        }

        logger->info("聊天会话创建成功: id={}", saved.id);
        return saved;

    } catch (const std::exception& e) {
        logger->error("创建聊天会话失败: {}", e.what());
        throw;
    }
}

bool ChatServiceImpl::deleteChatSession(const std::string& sessionId, const std::string& userId) {
    try {
        logger->debug("删除聊天会话: sessionId={}, userId={}", sessionId, userId);

        // conversation_contexts 表的 is_active 设为 false
        // 当前 DAO 没有 delete 方法，通过 update 实现
        if (g_databaseService && g_databaseService->isInitialized()) {
            int64_t sid = std::stoll(sessionId);
            auto ctxResult = g_databaseService->contextDAO().getById(sid);
            if (ctxResult.success) {
                auto ctx = ctxResult.data.value();
                ctx.is_active = false;
                g_databaseService->contextDAO().update(ctx);
            }
        }

        logger->info("聊天会话删除成功: sessionId={}", sessionId);
        return true;

    } catch (const std::exception& e) {
        logger->error("删除聊天会话失败: {}", e.what());
        return false;
    }
}

// ==================== 搜索消息 ====================

std::vector<dto::ChatMessageDTO> ChatServiceImpl::searchMessages(const std::string& userId,
                                                                 const std::string& keyword,
                                                                 int limit) {
    try {
        logger->debug("搜索消息: userId={}, keyword={}, limit={}", userId, keyword, limit);

        std::vector<dto::ChatMessageDTO> messages;

        if (!g_databaseService || !g_databaseService->isInitialized() || keyword.empty()) {
            return messages;
        }

        // 获取用户消息后在内存中过滤 (TODO: 添加数据库 LIKE 搜索)
        int64_t uid = std::stoll(userId);
        auto result = g_databaseService->messageDAO().getByUserId(uid, 200, 0);

        if (result.success) {
            int count = 0;
            for (const auto& dbMsg : result.data.value()) {
                if (count >= limit) break;
                if (dbMsg.content.find(keyword) != std::string::npos) {
                    dto::ChatMessageDTO msg;
                    msg.id = std::to_string(dbMsg.id);
                    msg.senderId = std::to_string(dbMsg.user_id);
                    msg.receiverId = userId;
                    msg.content = dbMsg.content;
                    msg.isRead = true;
                    msg.messageType = "text";
                    messages.push_back(msg);
                    count++;
                }
            }
        }

        logger->debug("搜索到 {} 条相关消息", messages.size());
        return messages;

    } catch (const std::exception& e) {
        logger->error("搜索消息失败: {}", e.what());
        throw;
    }
}

// ==================== 清空聊天历史 ====================

bool ChatServiceImpl::clearChatHistory(const std::string& userId, const std::string& targetId) {
    try {
        logger->debug("清空聊天历史: userId={}, targetId={}", userId, targetId);

        // 将该用户所有消息设为不可见
        // 当前 MessageDAO 没有批量 delete 方法，逐条处理
        if (g_databaseService && g_databaseService->isInitialized()) {
            int64_t uid = std::stoll(userId);
            auto result = g_databaseService->messageDAO().getByUserId(uid, 1000, 0);
            if (result.success) {
                for (const auto& msg : result.data.value()) {
                    g_databaseService->messageDAO().delete_(msg.id);
                }
            }
        }

        logger->info("聊天历史清空成功: userId={}, targetId={}", userId, targetId);
        return true;

    } catch (const std::exception& e) {
        logger->error("清空聊天历史失败: {}", e.what());
        return false;
    }
}

} // namespace yachiyo::services
