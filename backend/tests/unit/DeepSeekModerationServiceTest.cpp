#include <gtest/gtest.h>
#include "services/DeepSeekModerationService.hpp"

using yachiyo::services::DeepSeekModerationService;
using Yachiyo::DTO::ModerationRequest;
using Yachiyo::DTO::ModerationResponse;

class DeepSeekModerationServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = std::make_unique<DeepSeekModerationService>();
        // 不配置 API Key → 回退到规则审核
        service->initialize("", "");
    }
    
    std::unique_ptr<DeepSeekModerationService> service;
};

// ==================== 初始化测试 ====================

TEST_F(DeepSeekModerationServiceTest, Initialize_NoApiKey) {
    DeepSeekModerationService svc;
    EXPECT_TRUE(svc.initialize("", ""));
}

TEST_F(DeepSeekModerationServiceTest, Initialize_WithEndpoint) {
    DeepSeekModerationService svc;
    EXPECT_TRUE(svc.initialize("", "https://api.deepseek.com"));
    EXPECT_TRUE(svc.isHealthy());
}

// ==================== 规则审核测试 (无 API Key 回退) ====================

TEST_F(DeepSeekModerationServiceTest, Moderate_SafeContent) {
    ModerationRequest req;
    req.content = "今天天气真好，我很开心";
    
    auto result = service->moderate(req);
    ASSERT_TRUE(result.isSuccess());
    
    auto response = result.getValue();
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.overallVerdict, "pass");
    EXPECT_LT(response.overallRiskScore, 0.5f);
}

TEST_F(DeepSeekModerationServiceTest, Moderate_RiskyContent) {
    ModerationRequest req;
    req.content = "我要殴打那个人";
    
    auto result = service->moderate(req);
    ASSERT_TRUE(result.isSuccess());
    
    auto response = result.getValue();
    EXPECT_TRUE(response.success);
    // 应该检测到 violence 类别
    EXPECT_GT(response.overallRiskScore, 0.0f);
}

TEST_F(DeepSeekModerationServiceTest, Moderate_EmptyContent) {
    ModerationRequest req;
    req.content = "";
    
    auto result = service->moderate(req);
    ASSERT_TRUE(result.isSuccess());
    
    auto response = result.getValue();
    EXPECT_EQ(response.overallVerdict, "pass");
}

// ==================== 批量审核测试 ====================

TEST_F(DeepSeekModerationServiceTest, BatchModerate) {
    std::vector<ModerationRequest> requests;
    
    ModerationRequest req1;
    req1.content = "你好";
    requests.push_back(req1);
    
    ModerationRequest req2;
    req2.content = "今天很开心";
    requests.push_back(req2);
    
    auto results = service->batchModerate(requests);
    EXPECT_EQ(results.size(), 2u);
    
    for (const auto& result : results) {
        EXPECT_TRUE(result.isSuccess());
    }
}

// ==================== 情感验证测试 ====================

TEST_F(DeepSeekModerationServiceTest, VerifyEmotions_Safe) {
    std::vector<std::string> emotions = {"happy", "excited"};
    
    auto result = service->verifyEmotions("今天真开心", emotions);
    ASSERT_TRUE(result.isSuccess());
    
    auto verification = result.getValue();
    EXPECT_TRUE(verification.isValid);
    EXPECT_EQ(verification.matchedEmotions.size(), 2u);
}

TEST_F(DeepSeekModerationServiceTest, VerifyEmotions_Risky) {
    std::vector<std::string> emotions = {"愤怒", "仇恨"};
    
    auto result = service->verifyEmotions("测试文本", emotions);
    ASSERT_TRUE(result.isSuccess());
    
    auto verification = result.getValue();
    EXPECT_GT(verification.riskScore, 0.0f);
    EXPECT_FALSE(verification.unmatchedEmotions.empty());
}

// ==================== 阈值测试 ====================

TEST_F(DeepSeekModerationServiceTest, SetThreshold) {
    service->setThreshold(0.8f);
    // 验证不会崩溃
    
    service->setThreshold(-0.1f);  // 应被限制为 0
    service->setThreshold(1.5f);   // 应被限制为 1.0
}

// ==================== 缓存测试 ====================

TEST_F(DeepSeekModerationServiceTest, CacheHit) {
    ModerationRequest req;
    req.content = "缓存测试内容";
    
    // 第一次调用
    auto result1 = service->moderate(req);
    ASSERT_TRUE(result1.isSuccess());
    
    // 第二次调用应命中缓存
    auto result2 = service->moderate(req);
    ASSERT_TRUE(result2.isSuccess());
    
    EXPECT_EQ(result1.getValue().overallVerdict, result2.getValue().overallVerdict);
}

TEST_F(DeepSeekModerationServiceTest, ClearCache) {
    ModerationRequest req;
    req.content = "会被清理的缓存";
    service->moderate(req);
    
    service->clearCache();
    // 不应崩溃，后续调用应重新计算
    auto result = service->moderate(req);
    EXPECT_TRUE(result.isSuccess());
}

// ==================== 统计测试 ====================

TEST_F(DeepSeekModerationServiceTest, Statistics) {
    ModerationRequest req;
    req.content = "统计测试";
    service->moderate(req);
    
    auto stats = service->getStatistics();
    EXPECT_GE(stats.totalModerations, 1);
    EXPECT_GE(stats.passedCount, 0);
}
