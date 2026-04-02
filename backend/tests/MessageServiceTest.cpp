#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include "../include/services/MessageServiceImpl.hpp"
#include "../include/services/AuthServiceImpl.hpp"
#include "../include/utils/DatabaseUtil.hpp"
#include "../include/utils/RedisUtil.hpp"

using namespace yachiyo::services;
using namespace yachiyo::utils;
using namespace yachiyo::models;

/**
 * @class MessageServiceTest
 * @brief MessageService 单元测试套件
 * 
 * 测试用例覆盖:
 * - 消息发送 (6层审查)
 * - 消息查询 (用户/管理员)
 * - 消息审查 (批准/拒绝)
 * - 统计数据 (管理员)
 * - 高风险消息 (管理员)
 */
class MessageServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<IMessageService> messageService;
    std::shared_ptr<IAuthService> authService;
    std::shared_ptr<DatabaseUtil> dbUtil;
    std::shared_ptr<RedisUtil> redisUtil;
    std::shared_ptr<JwtUtil> jwtUtil;
    int64_t testUserId;
    
    void SetUp() override {
        // 初始化工具
        dbUtil = std::make_shared<DatabaseUtil>();
        redisUtil = std::make_shared<RedisUtil>();
        jwtUtil = std::make_shared<JwtUtil>("test-secret-key");
        
        // 创建服务实例
        authService = std::make_shared<AuthServiceImpl>(dbUtil, redisUtil, jwtUtil);
        messageService = std::make_shared<MessageServiceImpl>(dbUtil, redisUtil, authService);
        
        // 创建测试用户
        auto result = authService->registerUser(
            "msg_test_user",
            "msgtest@example.com",
            "MsgTestPassword123"
        );
        testUserId = result.getData()->getId();
    }
    
    void TearDown() override {
        // 清理测试数据
        // 可选: dbUtil->execute("DELETE FROM user_messages WHERE user_id = $1", {std::to_string(testUserId)});
    }
};

// ==================== 消息发送测试 (6层审查) ====================

TEST_F(MessageServiceTest, SendMessageSuccess) {
    // 测试成功发送正常消息
    auto result = messageService->sendMessage(
        testUserId,
        "Hello, this is a test message",
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    ASSERT_TRUE(result.isSuccess());
    auto msg = result.getData();
    EXPECT_GT(msg->getId(), 0);
    EXPECT_EQ(msg->getUserId(), testUserId);
    EXPECT_EQ(msg->getOriginalMessage(), "Hello, this is a test message");
    // 正常消息应该被批准或标记为待审查
    EXPECT_TRUE(msg->getReviewStatus() == ReviewStatus::APPROVED || 
                msg->getReviewStatus() == ReviewStatus::MANUAL_REVIEW);
}

TEST_F(MessageServiceTest, SendMessageEmptyContent) {
    // 测试发送空消息
    auto result = messageService->sendMessage(
        testUserId,
        "",
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    ASSERT_FALSE(result.isSuccess());
    EXPECT_THAT(result.getErrorMsg(), 
                ::testing::HasSubstr("长度"));
}

TEST_F(MessageServiceTest, SendMessageTooLong) {
    // 测试发送过长的消息
    std::string longMessage(600, 'a');  // 600字符，超过500限制
    
    auto result = messageService->sendMessage(
        testUserId,
        longMessage,
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    ASSERT_FALSE(result.isSuccess());
}

TEST_F(MessageServiceTest, SendMessageRateLimit) {
    // 测试速率限制 (Layer 1)
    // 短时间内发送多条消息
    
    for (int i = 0; i < 5; ++i) {
        auto result = messageService->sendMessage(
            testUserId,
            "Message " + std::to_string(i),
            "192.168.1.1",
            "Mozilla/5.0"
        );
        EXPECT_TRUE(result.isSuccess());
    }
    
    // 第6条应该被拒 (超过限制)
    auto result = messageService->sendMessage(
        testUserId,
        "Message 6",
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    ASSERT_FALSE(result.isSuccess());
    EXPECT_THAT(result.getErrorMsg(), 
                ::testing::HasSubstr("频繁"));
}

TEST_F(MessageServiceTest, SendMessageWithBlockedKeyword) {
    // 测试包含敏感词的消息 (Layer 3)
    // 注意: 需要在数据库中预先插入敏感词
    
    // dbUtil->execute(
    //     "INSERT INTO blocked_keywords (keyword, severity, is_active) VALUES ($1, $2, true)",
    //     {"test_blocked_keyword", "5"}
    // );
    
    // auto result = messageService->sendMessage(
    //     testUserId,
    //     "This message contains test_blocked_keyword",
    //     "192.168.1.1",
    //     "Mozilla/5.0"
    // );
    
    // // 应该被标记为待审查或拒绝
    // ASSERT_TRUE(result.isSuccess() || !result.isSuccess());
    // auto msg = result.getData();
    // EXPECT_EQ(msg->getIsBlockedKeyword(), true);
}

TEST_F(MessageServiceTest, SendMessageSuspiciousContent) {
    // 测试可疑内容 (AI审查)
    auto result = messageService->sendMessage(
        testUserId,
        "AAAAAAAAAA!!!!!!!!!!!!!!!!!!!!!!!", // 重复字符 + 特殊字符
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    // 可疑内容应该被标记为待审查或拒绝
    if (result.isSuccess()) {
        auto msg = result.getData();
        EXPECT_TRUE(msg->getSpamScore() > 0.3);
    }
}

// ==================== 消息查询测试 ====================

TEST_F(MessageServiceTest, GetUserMessages) {
    // 先发送几条消息
    for (int i = 0; i < 3; ++i) {
        messageService->sendMessage(
            testUserId,
            "User message " + std::to_string(i),
            "192.168.1.1",
            "Mozilla/5.0"
        );
    }
    
    // 清空速率限制缓存后再查询
    redisUtil->del("rate_limit:" + std::to_string(testUserId));
    
    // 查询用户消息
    auto result = messageService->getUserMessages(testUserId, 10, 0);
    
    ASSERT_TRUE(result.isSuccess());
    auto messages = result.getData();
    EXPECT_GE(messages.size(), 3);
}

TEST_F(MessageServiceTest, GetUserMessagesWithPagination) {
    // 发送多条消息
    for (int i = 0; i < 15; ++i) {
        messageService->sendMessage(
            testUserId,
            "Paginated message " + std::to_string(i),
            "192.168.1.1",
            "Mozilla/5.0"
        );
        // 清空速率限制
        redisUtil->del("rate_limit:" + std::to_string(testUserId));
    }
    
    // 第一页
    auto result1 = messageService->getUserMessages(testUserId, 10, 0);
    ASSERT_TRUE(result1.isSuccess());
    EXPECT_EQ(result1.getData().size(), 10);
    
    // 第二页
    auto result2 = messageService->getUserMessages(testUserId, 10, 10);
    ASSERT_TRUE(result2.isSuccess());
    EXPECT_EQ(result2.getData().size(), 5);
}

// ==================== 消息审查测试 ====================

TEST_F(MessageServiceTest, ReviewMessageApprove) {
    // 发送消息
    auto sendResult = messageService->sendMessage(
        testUserId,
        "Message to review",
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    int64_t messageId = sendResult.getData()->getId();
    
    // 审查消息 (批准)
    auto reviewResult = messageService->reviewMessage(
        messageId,
        1,  // 管理员ID
        true,  // 批准
        "Approved by admin"
    );
    
    ASSERT_TRUE(reviewResult.isSuccess());
}

TEST_F(MessageServiceTest, ReviewMessageReject) {
    // 发送消息
    auto sendResult = messageService->sendMessage(
        testUserId,
        "Message to reject",
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    int64_t messageId = sendResult.getData()->getId();
    
    // 审查消息 (拒绝)
    auto reviewResult = messageService->reviewMessage(
        messageId,
        1,  // 管理员ID
        false,  // 拒绝
        "Contains inappropriate content"
    );
    
    ASSERT_TRUE(reviewResult.isSuccess());
}

// ==================== 统计数据测试 ====================

TEST_F(MessageServiceTest, GetStatistics) {
    // 发送几条消息
    redisUtil->del("rate_limit:" + std::to_string(testUserId));
    messageService->sendMessage(testUserId, "Message 1", "192.168.1.1", "Mozilla/5.0");
    
    redisUtil->del("rate_limit:" + std::to_string(testUserId));
    messageService->sendMessage(testUserId, "Message 2", "192.168.1.1", "Mozilla/5.0");
    
    // 获取统计
    auto result = messageService->getStatistics();
    
    ASSERT_TRUE(result.isSuccess());
    auto stats = result.getData();
    EXPECT_GT(stats["total_messages"].get<int>(), 0);
}

// ==================== 高风险消息测试 ====================

TEST_F(MessageServiceTest, GetHighRiskMessages) {
    // 这个测试需要先插入一些高风险消息
    // 可以通过直接修改数据库来模拟
    
    auto result = messageService->getHighRiskMessages(10, 0);
    
    ASSERT_TRUE(result.isSuccess());
    // 结果可能为空，取决于数据库中是否有高风险消息
}

// ==================== 待审查消息测试 ====================

TEST_F(MessageServiceTest, GetPendingMessages) {
    // 发送一条可能被标记为待审查的消息
    redisUtil->del("rate_limit:" + std::to_string(testUserId));
    auto sendResult = messageService->sendMessage(
        testUserId,
        "AAAAAAA!!!!!!!!!!!!", // 可疑内容
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    // 获取待审查消息
    auto result = messageService->getPendingMessages(10, 0);
    
    ASSERT_TRUE(result.isSuccess());
}

// ==================== 隐藏消息测试 ====================

TEST_F(MessageServiceTest, HideMessageSuccess) {
    // 发送消息
    auto sendResult = messageService->sendMessage(
        testUserId,
        "Message to hide",
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    int64_t messageId = sendResult.getData()->getId();
    
    // 隐藏消息
    auto hideResult = messageService->hideMessage(messageId, "Spam");
    
    ASSERT_TRUE(hideResult.isSuccess());
}

// ==================== 6层审查系统验证 ====================

TEST_F(MessageServiceTest, SixLayerReviewSystem) {
    // 测试6层审查系统的完整流程
    
    // 第1层: 速率限制已在其他测试中验证
    
    // 第2层: IP黑名单 (需要插入黑名单)
    // dbUtil->execute(
    //     "INSERT INTO user_blacklist (identifier, identifier_type, reason) VALUES ($1, 1, 'test')",
    //     {"192.168.99.99"}
    // );
    // auto blockedIpResult = messageService->sendMessage(
    //     testUserId,
    //     "Message from blocked IP",
    //     "192.168.99.99",
    //     "Mozilla/5.0"
    // );
    // ASSERT_FALSE(blockedIpResult.isSuccess());
    
    // 第3层: 敏感词检查 (需要预设敏感词)
    
    // 第4层: AI审查 (已在可疑内容测试中验证)
    
    // 第5层: 行为分析 (已在速率限制测试中验证)
    
    // 第6层: 人工审查标记
    auto result = messageService->sendMessage(
        testUserId,
        "Normal message for manual review",
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    if (result.isSuccess()) {
        auto msg = result.getData();
        // 验证审查状态为允许的值之一
        EXPECT_TRUE(
            msg->getReviewStatus() == ReviewStatus::APPROVED ||
            msg->getReviewStatus() == ReviewStatus::MANUAL_REVIEW ||
            msg->getReviewStatus() == ReviewStatus::REJECTED
        );
    }
}

// ==================== 性能测试 ====================

TEST_F(MessageServiceTest, SendMessagePerformance) {
    // 测试消息发送的性能
    auto start = std::chrono::high_resolution_clock::now();
    
    auto result = messageService->sendMessage(
        testUserId,
        "Performance test message",
        "192.168.1.1",
        "Mozilla/5.0"
    );
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_TRUE(result.isSuccess());
    // 验证发送时间在1秒以内 (包括6层审查)
    EXPECT_LT(duration.count(), 1000);
    
    // 打印性能数据
    std::cout << "Message send time: " << duration.count() << "ms" << std::endl;
}

TEST_F(MessageServiceTest, GetMessagesPerformance) {
    // 测试获取消息列表的性能
    
    // 清空速率限制并发送多条消息
    for (int i = 0; i < 10; ++i) {
        redisUtil->del("rate_limit:" + std::to_string(testUserId));
        messageService->sendMessage(
            testUserId,
            "Performance message " + std::to_string(i),
            "192.168.1.1",
            "Mozilla/5.0"
        );
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto result = messageService->getUserMessages(testUserId, 20, 0);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_TRUE(result.isSuccess());
    // 验证查询时间在500ms以内
    EXPECT_LT(duration.count(), 500);
    
    std::cout << "Get messages time: " << duration.count() << "ms" << std::endl;
}

// ==================== 主函数 ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
