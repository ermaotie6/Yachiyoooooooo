#include <gtest/gtest.h>
#include "dto/ChatMessageDTO.hpp"

using Yachiyo::DTO::ChatMessageDTO;
using Yachiyo::DTO::ChatSessionDTO;

// ==================== ChatMessageDTO 测试 ====================

TEST(ChatMessageDTOTest, DefaultValues) {
    ChatMessageDTO msg;
    EXPECT_TRUE(msg.id.empty());
    EXPECT_FALSE(msg.isRead);
    EXPECT_EQ(msg.messageType, "text");
    EXPECT_TRUE(msg.metadata.is_object());
}

TEST(ChatMessageDTOTest, ToJson) {
    ChatMessageDTO msg;
    msg.id = "msg_001";
    msg.senderId = "user_1";
    msg.receiverId = "user_2";
    msg.content = "你好！";
    msg.isRead = true;
    msg.messageType = "text";
    
    json j = msg.toJson();
    
    EXPECT_EQ(j["id"], "msg_001");
    EXPECT_EQ(j["senderId"], "user_1");
    EXPECT_EQ(j["receiverId"], "user_2");
    EXPECT_EQ(j["content"], "你好！");
    EXPECT_TRUE(j["isRead"]);
    EXPECT_EQ(j["messageType"], "text");
    EXPECT_TRUE(j.contains("timestamp"));
}

TEST(ChatMessageDTOTest, FromJson) {
    json j;
    j["id"] = "msg_002";
    j["senderId"] = "user_A";
    j["receiverId"] = "user_B";
    j["content"] = "测试消息";
    j["isRead"] = false;
    j["messageType"] = "image";
    j["metadata"] = {{"key", "value"}};
    
    auto msg = ChatMessageDTO::fromJson(j);
    
    EXPECT_EQ(msg.id, "msg_002");
    EXPECT_EQ(msg.senderId, "user_A");
    EXPECT_EQ(msg.receiverId, "user_B");
    EXPECT_EQ(msg.content, "测试消息");
    EXPECT_FALSE(msg.isRead);
    EXPECT_EQ(msg.messageType, "image");
    EXPECT_EQ(msg.metadata["key"], "value");
}

TEST(ChatMessageDTOTest, FromJson_PartialData) {
    json j;
    j["content"] = "只有内容";
    
    auto msg = ChatMessageDTO::fromJson(j);
    EXPECT_EQ(msg.content, "只有内容");
    EXPECT_TRUE(msg.id.empty());
    EXPECT_FALSE(msg.isRead);
}

TEST(ChatMessageDTOTest, Roundtrip) {
    ChatMessageDTO original;
    original.id = "roundtrip";
    original.senderId = "s1";
    original.receiverId = "r1";
    original.content = "往返测试消息";
    original.isRead = true;
    original.messageType = "system";
    
    json j = original.toJson();
    auto restored = ChatMessageDTO::fromJson(j);
    
    EXPECT_EQ(original.id, restored.id);
    EXPECT_EQ(original.senderId, restored.senderId);
    EXPECT_EQ(original.receiverId, restored.receiverId);
    EXPECT_EQ(original.content, restored.content);
    EXPECT_EQ(original.isRead, restored.isRead);
    EXPECT_EQ(original.messageType, restored.messageType);
}

// ==================== ChatSessionDTO 测试 ====================

TEST(ChatSessionDTOTest, DefaultValues) {
    ChatSessionDTO session;
    EXPECT_EQ(session.unreadCount, 0);
    EXPECT_FALSE(session.isPinned);
    EXPECT_FALSE(session.isOnline);
}

TEST(ChatSessionDTOTest, ToJson) {
    ChatSessionDTO session;
    session.id = "sess_001";
    session.userId = "user_1";
    session.targetId = "avatar";
    session.targetName = "Yachiyo";
    session.lastMessage = "最后一条消息";
    session.unreadCount = 3;
    session.isPinned = true;
    session.isOnline = true;
    
    json j = session.toJson();
    
    EXPECT_EQ(j["id"], "sess_001");
    EXPECT_EQ(j["userId"], "user_1");
    EXPECT_EQ(j["targetName"], "Yachiyo");
    EXPECT_EQ(j["lastMessage"], "最后一条消息");
    EXPECT_EQ(j["unreadCount"], 3);
    EXPECT_TRUE(j["isPinned"]);
    EXPECT_TRUE(j["isOnline"]);
}
