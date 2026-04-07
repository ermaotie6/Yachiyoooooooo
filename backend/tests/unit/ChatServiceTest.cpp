#include <gtest/gtest.h>
#include "services/ChatService.hpp"

using Yachiyo::Services::ChatServiceImpl;
using Yachiyo::DTO::ChatMessageDTO;
using Yachiyo::DTO::ChatSessionDTO;

class ChatServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 注意: 没有数据库连接的情况下，ChatServiceImpl 会回退到无持久化模式
        chatService = std::make_unique<ChatServiceImpl>();
    }
    
    std::unique_ptr<ChatServiceImpl> chatService;
};

// ==================== 构造/析构测试 ====================

TEST_F(ChatServiceTest, Construction) {
    EXPECT_NE(chatService, nullptr);
}

// ==================== 消息发送测试 (无数据库模式) ====================

TEST_F(ChatServiceTest, SendMessage_NoDB) {
    ChatMessageDTO msg;
    msg.senderId = "user_1";
    msg.receiverId = "user_2";
    msg.content = "测试消息";
    msg.messageType = "text";
    
    auto savedMsg = chatService->sendMessage(msg);
    
    // 无数据库时应生成临时 ID
    EXPECT_FALSE(savedMsg.id.empty());
    EXPECT_EQ(savedMsg.content, "测试消息");
    EXPECT_EQ(savedMsg.senderId, "user_1");
}

TEST_F(ChatServiceTest, SendMessage_EmptyContent) {
    ChatMessageDTO msg;
    msg.senderId = "user_1";
    msg.receiverId = "user_2";
    msg.content = "";
    
    auto savedMsg = chatService->sendMessage(msg);
    EXPECT_FALSE(savedMsg.id.empty());  // 仍然应该分配 ID
}

// ==================== 聊天历史测试 ====================

TEST_F(ChatServiceTest, GetChatHistory_NoDB) {
    auto history = chatService->getChatHistory("user_1", "user_2", 50, 0);
    // 无数据库时应返回空列表
    EXPECT_TRUE(history.empty());
}

// ==================== 会话管理测试 ====================

TEST_F(ChatServiceTest, GetChatSessions_NoDB) {
    auto sessions = chatService->getChatSessions("user_1");
    EXPECT_TRUE(sessions.empty());
}

TEST_F(ChatServiceTest, CreateChatSession_NoDB) {
    ChatSessionDTO session;
    session.userId = "user_1";
    session.targetId = "avatar";
    session.targetName = "Yachiyo";
    
    auto created = chatService->createChatSession(session);
    
    // 无数据库时应生成临时 ID
    EXPECT_FALSE(created.id.empty());
    EXPECT_EQ(created.userId, "user_1");
}

// ==================== 标记已读测试 ====================

TEST_F(ChatServiceTest, MarkAsRead) {
    bool result = chatService->markAsRead("msg_001", "user_1");
    EXPECT_TRUE(result);  // 当前实现直接返回 true
}

// ==================== 删除消息测试 ====================

TEST_F(ChatServiceTest, DeleteMessage_NoDB) {
    bool result = chatService->deleteMessage("msg_001", "user_1");
    // 无数据库时应返回 false
    EXPECT_FALSE(result);
}

// ==================== 删除会话测试 ====================

TEST_F(ChatServiceTest, DeleteChatSession_NoDB) {
    bool result = chatService->deleteChatSession("session_001", "user_1");
    // 即使无数据库，当前实现仍返回 true (静默成功)
    EXPECT_TRUE(result);
}

// ==================== 搜索消息测试 ====================

TEST_F(ChatServiceTest, SearchMessages_NoDB) {
    auto results = chatService->searchMessages("user_1", "关键词");
    EXPECT_TRUE(results.empty());
}

TEST_F(ChatServiceTest, SearchMessages_EmptyKeyword) {
    auto results = chatService->searchMessages("user_1", "");
    EXPECT_TRUE(results.empty());
}

// ==================== 清空历史测试 ====================

TEST_F(ChatServiceTest, ClearChatHistory_NoDB) {
    bool result = chatService->clearChatHistory("user_1", "user_2");
    EXPECT_TRUE(result);
}
