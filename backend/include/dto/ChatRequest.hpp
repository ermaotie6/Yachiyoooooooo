#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace Yachiyo {
namespace DTO {

/**
 * @brief 聊天请求DTO
 */
class ChatRequest {
public:
    ChatRequest() = default;
    
    ChatRequest(const std::string& message, const std::string& conversationId)
        : message(message), conversationId(conversationId) {}

    // Getters
    const std::string& getMessage() const { return message; }
    const std::string& getConversationId() const { return conversationId; }

    // Setters
    void setMessage(const std::string& newMessage) { message = newMessage; }
    void setConversationId(const std::string& newConversationId) { conversationId = newConversationId; }

    /**
     * @brief 验证请求数据是否有效
     * @return 验证结果和错误信息
     */
    std::pair<bool, std::string> validate() const {
        if (message.empty()) {
            return {false, "消息不能为空"};
        }
        if (conversationId.empty()) {
            return {false, "会话ID不能为空"};
        }
        return {true, ""};
    }

    // JSON 序列化/反序列化
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ChatRequest, message, conversationId)

private:
    std::string message;
    std::string conversationId;
};

} // namespace DTO
} // namespace Yachiyo