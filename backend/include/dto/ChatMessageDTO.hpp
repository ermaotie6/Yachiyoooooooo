#pragma once

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Yachiyo {
namespace DTO {

/**
 * @brief 聊天消息数据传输对象
 * 
 * 用于前后端通信的消息结构，与数据库 Message 模型解耦
 */
struct ChatMessageDTO {
    std::string id;
    std::string senderId;
    std::string receiverId;
    std::string content;
    bool isRead = false;
    std::string messageType = "text";  // "text", "image", "system"
    json metadata = json::object();
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
    
    ChatMessageDTO() = default;
    
    json toJson() const {
        json j;
        j["id"] = id;
        j["senderId"] = senderId;
        j["receiverId"] = receiverId;
        j["content"] = content;
        j["isRead"] = isRead;
        j["messageType"] = messageType;
        j["metadata"] = metadata;
        j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();
        return j;
    }
    
    static ChatMessageDTO fromJson(const json& j) {
        ChatMessageDTO msg;
        if (j.contains("id")) msg.id = j["id"].get<std::string>();
        if (j.contains("senderId")) msg.senderId = j["senderId"].get<std::string>();
        if (j.contains("receiverId")) msg.receiverId = j["receiverId"].get<std::string>();
        if (j.contains("content")) msg.content = j["content"].get<std::string>();
        if (j.contains("isRead")) msg.isRead = j["isRead"].get<bool>();
        if (j.contains("messageType")) msg.messageType = j["messageType"].get<std::string>();
        if (j.contains("metadata")) msg.metadata = j["metadata"];
        return msg;
    }
};

/**
 * @brief 聊天会话信息 DTO
 */
struct ChatSessionDTO {
    std::string id;
    std::string userId;
    std::string targetId;
    std::string targetName;
    std::string targetAvatar;
    std::string lastMessage;
    int64_t lastMessageTime = 0;
    int unreadCount = 0;
    bool isPinned = false;
    bool isOnline = false;
    
    json toJson() const {
        json j;
        j["id"] = id;
        j["userId"] = userId;
        j["targetId"] = targetId;
        j["targetName"] = targetName;
        j["targetAvatar"] = targetAvatar;
        j["lastMessage"] = lastMessage;
        j["lastMessageTime"] = lastMessageTime;
        j["unreadCount"] = unreadCount;
        j["isPinned"] = isPinned;
        j["isOnline"] = isOnline;
        return j;
    }
};

} // namespace DTO
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::dto {
    using Yachiyo::DTO::ChatMessageDTO;
    using Yachiyo::DTO::ChatSessionDTO;
}
