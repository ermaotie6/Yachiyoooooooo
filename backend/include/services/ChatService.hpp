#pragma once

/**
 * @file ChatService.hpp
 * 
 * @note 架构待改进项 (P2-10):
 *       项目中存在两套独立的聊天系统:
 *       1. REST API 聊天 (ChatService — 本文件，存储到数据库)
 *       2. WebSocket 实时聊天 (WebSocketService/WebSocketController，走 Avatar 管线)
 *       这两套系统目前不共享会话/消息存储。如需统一，建议:
 *       - WebSocket 收到的消息也持久化到 ChatService
 *       - 或将 ChatService 定位为纯历史记录服务，WebSocket 负责实时通信
 *
 * @note 架构待改进项 (P1-7):
 *       密码哈希使用 SHA-256+随机盐，安全性低于 bcrypt/argon2。
 *       建议引入 libsodium 或 bcrypt 库替换 HashUtil 的密码哈希实现。
 */

#include <string>
#include <memory>
#include <vector>
#include "../models/Message.hpp"
#include "../dto/ChatRequest.hpp"
#include "../dto/ChatMessageDTO.hpp"
#include "../utils/Result.hpp"
#include "../utils/LogUtils.hpp"
#include <spdlog/spdlog.h>
#include <map>

namespace Yachiyo {
namespace Services {

// 引用 models 命名空间
namespace Models = yachiyo::models;

// 消息类型枚举 (ChatService 使用)
enum class MessageType {
    USER,       // 用户消息
    ASSISTANT,  // AI 助手消息
    SYSTEM      // 系统消息
};

// AI 提供商类型
enum class AIProvider {
    OPENAI,
    OLLAMA,
    LOCAL
};

// 聊天消息结构
struct ChatMessage {
    std::string role;    // "user", "assistant", "system"
    std::string content;
    
    ChatMessage(const std::string& role, const std::string& content)
        : role(role), content(content) {}
};

// 聊天配置
struct ChatConfig {
    AIProvider provider = AIProvider::OLLAMA;
    std::string model = "llama2";
    double temperature = 0.7;
    int maxTokens = 1000;
    
    // OpenAI 特定配置
    std::string openaiApiKey;
    std::string openaiBaseUrl = "https://api.openai.com/v1";
    
    // Ollama 特定配置
    std::string ollamaBaseUrl = "http://localhost:11434";
    
    // 本地模型配置
    std::string localModelPath;
};

/**
 * @brief AI聊天服务接口
 */
class ChatService {
public:
    virtual ~ChatService() = default;

    /**
     * @brief 发送聊天消息
     * @param request 聊天请求
     * @param userId 用户ID
     * @return 聊天响应
     */
    virtual Utils::Result<std::string> chat(const DTO::ChatRequest& request, int64_t userId) = 0;

    /**
     * @brief 创建新会话
     * @param userId 用户ID
     * @param title 会话标题（可选）
     * @return 会话ID
     */
    virtual Utils::Result<std::string> createConversation(int64_t userId, 
                                                         const std::string& title = "") = 0;

    /**
     * @brief 获取用户的所有会话
     * @param userId 用户ID
     * @return 会话列表
     */
    virtual Utils::Result<std::vector<std::string>> getConversations(int64_t userId) = 0;

    /**
     * @brief 获取会话消息历史
     * @param conversationId 会话ID
     * @param userId 用户ID
     * @return 消息列表
     */
    virtual Utils::Result<std::vector<Models::Message>> getConversationHistory(
        const std::string& conversationId, int64_t userId) = 0;

    /**
     * @brief 修改会话标题
     * @param conversationId 会话ID
     * @param userId 用户ID
     * @param newTitle 新标题
     * @return 操作结果
     */
    virtual Utils::Result<void> updateConversationTitle(
        const std::string& conversationId, 
        int64_t userId, 
        const std::string& newTitle) = 0;

    /**
     * @brief 删除会话
     * @param conversationId 会话ID
     * @param userId 用户ID
     * @return 操作结果
     */
    virtual Utils::Result<void> deleteConversation(
        const std::string& conversationId, int64_t userId) = 0;

    /**
     * @brief 清除会话历史
     * @param conversationId 会话ID
     * @param userId 用户ID
     * @return 操作结果
     */
    virtual Utils::Result<void> clearConversationHistory(
        const std::string& conversationId, int64_t userId) = 0;

    /**
     * @brief 获取聊天配置
     * @return 聊天配置
     */
    virtual ChatConfig getConfig() const = 0;

    /**
     * @brief 更新聊天配置
     * @param config 新配置
     */
    virtual void updateConfig(const ChatConfig& config) = 0;
};

/**
 * @brief AI聊天服务实现类
 */
class ChatServiceImpl : public ChatService {
public:
    ChatServiceImpl();
    ~ChatServiceImpl();

    // 实现接口方法
    Utils::Result<std::string> chat(const DTO::ChatRequest& request, int64_t userId) override;
    Utils::Result<std::string> createConversation(int64_t userId, 
                                                 const std::string& title = "") override;
    Utils::Result<std::vector<std::string>> getConversations(int64_t userId) override;
    Utils::Result<std::vector<Models::Message>> getConversationHistory(
        const std::string& conversationId, int64_t userId) override;
    Utils::Result<void> updateConversationTitle(
        const std::string& conversationId, 
        int64_t userId, 
        const std::string& newTitle) override;
    Utils::Result<void> deleteConversation(
        const std::string& conversationId, int64_t userId) override;
    Utils::Result<void> clearConversationHistory(
        const std::string& conversationId, int64_t userId) override;
    ChatConfig getConfig() const override;
    void updateConfig(const ChatConfig& config) override;

    // ==================== 聊天消息/会话 API (前端通信用) ====================
    
    /**
     * @brief 获取聊天历史
     */
    std::vector<DTO::ChatMessageDTO> getChatHistory(const std::string& userId,
                                                     const std::string& targetId,
                                                     int limit = 50,
                                                     int offset = 0);
    
    /**
     * @brief 发送聊天消息
     */
    DTO::ChatMessageDTO sendMessage(const DTO::ChatMessageDTO& message);
    
    /**
     * @brief 获取聊天会话列表
     */
    std::vector<DTO::ChatSessionDTO> getChatSessions(const std::string& userId);
    
    /**
     * @brief 创建聊天会话
     */
    DTO::ChatSessionDTO createChatSession(const DTO::ChatSessionDTO& session);
    
    /**
     * @brief 搜索消息
     */
    std::vector<DTO::ChatMessageDTO> searchMessages(const std::string& userId,
                                                     const std::string& keyword);
    
    /**
     * @brief 删除消息
     */
    bool deleteMessage(const std::string& messageId, const std::string& userId);

private:
    /**
     * @brief 调用OpenAI API
     * @param messages 消息列表
     * @return AI响应
     */
    std::string callOpenAI(const std::vector<ChatMessage>& messages);

    /**
     * @brief 调用Ollama API
     * @param messages 消息列表
     * @return AI响应
     */
    std::string callOllama(const std::vector<ChatMessage>& messages);

    /**
     * @brief 调用本地模型
     * @param messages 消息列表
     * @return AI响应
     */
    std::string callLocalModel(const std::vector<ChatMessage>& messages);

    /**
     * @brief 生成会话ID
     * @return 唯一的会话ID
     */
    std::string generateConversationId() const;

    /**
     * @brief 验证会话权限
     * @param conversationId 会话ID
     * @param userId 用户ID
     * @return 是否有权限
     */
    bool validateConversationPermission(const std::string& conversationId, int64_t userId);

    /**
     * @brief 保存消息到数据库
     * @param conversationId 会话ID
     * @param content 消息内容
     * @param type 消息类型
     * @return 是否保存成功
     */
    bool saveMessage(const std::string& conversationId, 
                    const std::string& content, 
                    MessageType type);

    // 配置
    ChatConfig config;
    
    // 日志器
    std::shared_ptr<spdlog::logger> logger;
    
    // 会话缓存（实际应该使用Redis）
    std::map<std::string, int64_t> conversationOwners; // 会话ID -> 用户ID
};

} // namespace Services
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::services {
    using ChatService = Yachiyo::Services::ChatService;
    using ChatServiceImpl = Yachiyo::Services::ChatServiceImpl;
    using LogUtils = yachiyo::utils::LogUtils;
    namespace dto = yachiyo::dto;
}