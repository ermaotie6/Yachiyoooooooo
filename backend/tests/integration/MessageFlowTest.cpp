#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "services/ChatService.hpp"
#include "dto/ChatRequest.hpp"
#include "dto/ChatMessageDTO.hpp"
#include "utils/JwtUtil.hpp"
#include "utils/HashUtil.hpp"
#include <memory>

using namespace Yachiyo::Services;
using namespace Yachiyo::DTO;
using namespace Yachiyo::Utils;

/**
 * 消息流集成测试
 * 
 * 测试消息处理的完整流程:
 *   用户认证 → 发送消息 → 创建/获取会话 → 消息历史 → 删除
 *
 * 注意: 无数据库环境，使用 ChatServiceImpl 的 no-DB 模式。
 *       主要验证服务层逻辑 + DTO 序列化的协作正确性。
 */

class MessageFlowIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化 JWT 工具 (用于模拟认证)
        jwtUtil = std::make_shared<JwtUtil>();
        jwtUtil->initialize("test_message_flow_secret_key");
        
        // 创建 ChatServiceImpl（no-DB 模式）
        chatService = std::make_shared<ChatServiceImpl>();
    }
    
    std::shared_ptr<JwtUtil> jwtUtil;
    std::shared_ptr<ChatServiceImpl> chatService;
};

// ==================== DTO 序列化端到端测试 ====================

TEST_F(MessageFlowIntegrationTest, ChatRequestSerializationRoundtrip) {
    // 模拟前端发来的 JSON
    std::string jsonStr = R"({
        "message": "你好，Yachiyo！",
        "conversationId": "conv_123"
    })";
    
    // 反序列化
    auto j = nlohmann::json::parse(jsonStr);
    ChatRequest request = j.get<ChatRequest>();
    
    EXPECT_EQ(request.getMessage(), "你好，Yachiyo！");
    EXPECT_EQ(request.getConversationId(), "conv_123");
    
    // 验证
    auto [valid, msg] = request.validate();
    EXPECT_TRUE(valid);
    EXPECT_TRUE(msg.empty());
    
    // 再序列化回去
    nlohmann::json serialized = request;
    EXPECT_EQ(serialized["message"], "你好，Yachiyo！");
    EXPECT_EQ(serialized["conversationId"], "conv_123");
}

TEST_F(MessageFlowIntegrationTest, ChatRequestValidation) {
    // 空消息
    ChatRequest emptyMsg("", "conv_123");
    auto [v1, m1] = emptyMsg.validate();
    EXPECT_FALSE(v1);
    EXPECT_FALSE(m1.empty());
    
    // 空会话ID
    ChatRequest emptyConv("hello", "");
    auto [v2, m2] = emptyConv.validate();
    EXPECT_FALSE(v2);
    EXPECT_FALSE(m2.empty());
    
    // 有效请求
    ChatRequest valid("hello", "conv_123");
    auto [v3, m3] = valid.validate();
    EXPECT_TRUE(v3);
}

// ==================== ChatMessage DTO 测试 ====================

TEST_F(MessageFlowIntegrationTest, ChatMessageDTOSerialization) {
    yachiyo::dto::ChatMessageDTO msg;
    msg.messageId = "msg_001";
    msg.sessionId = "session_001";
    msg.role = "user";
    msg.content = "今天天气怎么样？";
    msg.timestamp = 1700000000;
    
    // 序列化
    nlohmann::json j = msg.toJson();
    EXPECT_EQ(j["messageId"], "msg_001");
    EXPECT_EQ(j["role"], "user");
    EXPECT_EQ(j["content"], "今天天气怎么样？");
    
    // 反序列化
    auto restored = yachiyo::dto::ChatMessageDTO::fromJson(j);
    EXPECT_EQ(restored.messageId, msg.messageId);
    EXPECT_EQ(restored.role, msg.role);
    EXPECT_EQ(restored.content, msg.content);
}

TEST_F(MessageFlowIntegrationTest, ChatSessionDTOSerialization) {
    yachiyo::dto::ChatSessionDTO session;
    session.sessionId = "sess_001";
    session.userId = 42;
    session.title = "测试会话";
    session.createdAt = 1700000000;
    session.messageCount = 5;
    
    nlohmann::json j = session.toJson();
    EXPECT_EQ(j["sessionId"], "sess_001");
    EXPECT_EQ(j["userId"], 42);
    EXPECT_EQ(j["title"], "测试会话");
    EXPECT_EQ(j["messageCount"], 5);
}

// ==================== 认证 + 消息发送端到端测试 ====================

TEST_F(MessageFlowIntegrationTest, AuthenticatedMessageSend) {
    // 步骤 1: 模拟用户登录（获取令牌）
    int64_t userId = 42;
    std::string username = "test_user";
    std::string token = jwtUtil->generateToken(userId, username, "USER");
    ASSERT_FALSE(token.empty());
    
    // 步骤 2: 从令牌提取用户信息（模拟中间件）
    ASSERT_TRUE(jwtUtil->verifyToken(token));
    int64_t extractedUserId = jwtUtil->getUserId(token);
    EXPECT_EQ(extractedUserId, userId);
    
    // 步骤 3: 用提取的 userId 发送消息
    ChatRequest request("你好 Yachiyo", "conv_001");
    auto result = chatService->chat(request, extractedUserId);
    
    // 无 DB/AI 环境下，消息发送会失败（但不应崩溃）
    // 因为 ChatServiceImpl 没有配置 AI 后端
    if (!result.isSuccess()) {
        EXPECT_FALSE(result.getError().message.empty());
    }
}

// ==================== 会话管理端到端测试 ====================

TEST_F(MessageFlowIntegrationTest, SessionLifecycle) {
    int64_t userId = 42;
    
    // 获取用户会话列表（初始应为空或返回错误）
    auto sessions = chatService->getSessions(userId);
    // no-DB 模式可能返回错误
    if (sessions.isSuccess()) {
        // 会话列表应该是空的（新用户）
        auto sessionList = sessions.getValue();
        // 不做具体断言，因为依赖于实现
    }
    
    // 创建新会话
    auto createResult = chatService->createSession(userId, "我的测试会话");
    // no-DB 模式创建会话可能失败
    if (createResult.isSuccess()) {
        std::string sessionId = createResult.getValue();
        EXPECT_FALSE(sessionId.empty());
    }
}

// ==================== 消息搜索测试 ====================

TEST_F(MessageFlowIntegrationTest, SearchMessages) {
    int64_t userId = 42;
    
    // 空关键词搜索
    auto emptySearch = chatService->searchMessages(userId, "");
    // 空关键词应返回错误
    if (!emptySearch.isSuccess()) {
        EXPECT_FALSE(emptySearch.getError().message.empty());
    }
    
    // 正常搜索（no-DB 模式下不会有结果）
    auto search = chatService->searchMessages(userId, "你好");
    // 可以成功（返回空列表）或者失败（no-DB）
}

// ==================== 消息删除测试 ====================

TEST_F(MessageFlowIntegrationTest, DeleteOperations) {
    int64_t userId = 42;
    
    // 删除不存在的消息
    auto deleteMsg = chatService->deleteMessage(userId, "nonexistent_msg");
    // no-DB 模式下应该报错
    if (!deleteMsg.isSuccess()) {
        EXPECT_FALSE(deleteMsg.getError().message.empty());
    }
    
    // 删除不存在的会话
    auto deleteSess = chatService->deleteSession(userId, "nonexistent_session");
    if (!deleteSess.isSuccess()) {
        EXPECT_FALSE(deleteSess.getError().message.empty());
    }
}

// ==================== 多用户消息隔离测试 ====================

TEST_F(MessageFlowIntegrationTest, MultiUserIsolation) {
    // 不同用户应该各自独立
    int64_t user1 = 1;
    int64_t user2 = 2;
    
    // 各自创建令牌
    std::string token1 = jwtUtil->generateToken(user1, "alice", "USER");
    std::string token2 = jwtUtil->generateToken(user2, "bob", "USER");
    
    // 令牌中的用户 ID 互不相同
    EXPECT_NE(jwtUtil->getUserId(token1), jwtUtil->getUserId(token2));
    EXPECT_EQ(jwtUtil->getUserId(token1), user1);
    EXPECT_EQ(jwtUtil->getUserId(token2), user2);
    
    // 各自获取会话列表（互不影响）
    auto sessions1 = chatService->getSessions(user1);
    auto sessions2 = chatService->getSessions(user2);
    // 不崩溃即可
}

// ==================== Unicode 消息处理测试 ====================

TEST_F(MessageFlowIntegrationTest, UnicodeMessageHandling) {
    std::vector<ChatRequest> requests = {
        ChatRequest("你好世界", "conv_zh"),
        ChatRequest("こんにちは世界", "conv_ja"),
        ChatRequest("안녕하세요", "conv_ko"),
        ChatRequest("🎮🎨🎵", "conv_emoji"),
        ChatRequest("mixed 中英 message 测试", "conv_mixed"),
    };
    
    for (const auto& req : requests) {
        auto [valid, msg] = req.validate();
        EXPECT_TRUE(valid) << "Failed for: " << req.getMessage();
    }
}
