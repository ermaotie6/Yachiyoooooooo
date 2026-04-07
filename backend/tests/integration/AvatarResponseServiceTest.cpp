#include <gtest/gtest.h>
#include "services/AvatarResponseService.hpp"
#include "services/OpenClawGateway.hpp"
#include "services/TranslationService.hpp"
#include "services/GPTSoVITSService.hpp"
#include "services/Live2DAnimationService.hpp"
#include "services/DeepSeekModerationService.hpp"
#include <chrono>

using namespace yachiyo::services;

/**
 * AvatarResponseService 集成测试
 * 
 * 测试完整的 Avatar 响应管线:
 *   用户消息 → 审核 → OpenClaw → 翻译 → TTS → 动画 → 组装
 * 
 * 注意: 这些测试在无外部服务的环境下运行,
 *       各子服务内部的 mock/fallback 机制会自动生效。
 */
class AvatarResponseServiceIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建所有子服务（无外部连接，均使用 mock/fallback）
        openclaw = std::make_shared<OpenClawGateway>();
        translation = std::make_shared<TranslationService>();
        tts = std::make_shared<GPTSoVITSService>();
        animation = std::make_shared<Live2DAnimationService>();
        moderation = std::make_shared<DeepSeekModerationService>();
        
        // 初始化子服务（不连接真实端点）
        openclaw->initialize("http://localhost:8765", 5);
        translation->initialize();
        tts->initialize();
        animation->initialize();
        moderation->initialize("", "");
        
        // 创建 AvatarResponseService
        avatarService = std::make_shared<AvatarResponseService>(
            openclaw, translation, tts, animation, moderation
        );
    }
    
    void TearDown() override {
        if (openclaw) openclaw->shutdown();
    }
    
    std::shared_ptr<OpenClawGateway> openclaw;
    std::shared_ptr<TranslationService> translation;
    std::shared_ptr<GPTSoVITSService> tts;
    std::shared_ptr<Live2DAnimationService> animation;
    std::shared_ptr<DeepSeekModerationService> moderation;
    std::shared_ptr<AvatarResponseService> avatarService;
};

// ==================== 初始化测试 ====================

TEST_F(AvatarResponseServiceIntegrationTest, Initialize) {
    bool result = avatarService->initialize("zh-CN");
    // 注意: OpenClaw 桥接服务不可用时仍可初始化（发出警告）
    // TTS 和动画服务使用 mock，所以总是成功
    EXPECT_TRUE(result);
}

TEST_F(AvatarResponseServiceIntegrationTest, IsHealthy) {
    avatarService->initialize("zh-CN");
    // 核心依赖 (openclaw, tts, animation) 存在即视为健康
    EXPECT_TRUE(avatarService->isHealthy());
}

// ==================== 消息处理管线测试 ====================

TEST_F(AvatarResponseServiceIntegrationTest, ProcessUserMessage_BridgeOffline) {
    avatarService->initialize("zh-CN");
    
    // 桥接服务离线时，processUserMessage 应返回错误（而不是崩溃）
    auto result = avatarService->processUserMessage("test_user", "你好", "zh-CN");
    
    // 由于桥接服务不可用，这应该失败
    // 但关键是它不会崩溃，而是返回结构化的错误
    if (!result.isSuccess()) {
        EXPECT_FALSE(result.getError().message.empty());
    }
}

// ==================== 缓存测试 ====================

TEST_F(AvatarResponseServiceIntegrationTest, ClearCache) {
    avatarService->initialize("zh-CN");
    
    // 清空缓存不应崩溃
    avatarService->clearCache(0);   // 清除所有
    avatarService->clearCache(3600); // 清除 1 小时前的
}

// ==================== 批量处理测试 ====================

TEST_F(AvatarResponseServiceIntegrationTest, BatchProcess_BridgeOffline) {
    avatarService->initialize("zh-CN");
    
    std::vector<std::string> messages = {"你好", "今天天气如何", "给我讲个笑话"};
    
    // 桥接服务离线，但不应崩溃
    auto result = avatarService->batchProcessMessages("test_user", messages, "zh-CN");
    
    // 应该返回成功（即使所有子消息都失败了，batchProcess 本身成功）
    EXPECT_TRUE(result.isSuccess());
    // 结果可能为空（因为所有子消息处理都因 bridge 离线而失败）
    // 重要的是没有崩溃
}

// ==================== 从 OpenClaw 响应生成测试 ====================

TEST_F(AvatarResponseServiceIntegrationTest, GenerateFromOpenClaw) {
    avatarService->initialize("zh-CN");
    
    // 手动构造 OpenClaw 响应（模拟 OpenClaw 返回数据）
    Yachiyo::DTO::OpenClawResponse openclawResp;
    openclawResp.requestId = "test_req";
    openclawResp.success = true;
    openclawResp.text = "你好！我是 Yachiyo，很高兴见到你。";
    openclawResp.emotions = {"happy"};
    openclawResp.actions = {"wave"};
    
    auto result = avatarService->generateFromOpenClaw("test_user", openclawResp, "zh-CN");
    
    ASSERT_TRUE(result.isSuccess());
    auto response = result.getValue();
    
    EXPECT_EQ(response.userId, "test_user");
    EXPECT_FALSE(response.requestId.empty());
    EXPECT_FALSE(response.text.empty());
    EXPECT_EQ(response.originalText, "你好！我是 Yachiyo，很高兴见到你。");
    EXPECT_EQ(response.emotions.size(), 1u);
    EXPECT_EQ(response.actions.size(), 1u);
}

TEST_F(AvatarResponseServiceIntegrationTest, GenerateFromOpenClaw_WithTranslation) {
    avatarService->initialize("zh-CN");
    
    Yachiyo::DTO::OpenClawResponse openclawResp;
    openclawResp.success = true;
    openclawResp.text = "很高兴认识你";
    openclawResp.emotions = {"happy"};
    openclawResp.actions = {};
    
    // 请求翻译为英文
    auto result = avatarService->generateFromOpenClaw("test_user", openclawResp, "en-US");
    
    ASSERT_TRUE(result.isSuccess());
    auto response = result.getValue();
    
    // 原文应保留
    EXPECT_EQ(response.originalText, "很高兴认识你");
    // 最终文本（如果翻译服务 mock 可用，应该不同于原文）
    EXPECT_FALSE(response.text.empty());
}

TEST_F(AvatarResponseServiceIntegrationTest, GenerateFromOpenClaw_MultipleEmotions) {
    avatarService->initialize("zh-CN");
    
    Yachiyo::DTO::OpenClawResponse openclawResp;
    openclawResp.success = true;
    openclawResp.text = "嗯...让我想想";
    openclawResp.emotions = {"thoughtful", "curious"};
    openclawResp.actions = {"tilt_head", "touch_chin"};
    
    auto result = avatarService->generateFromOpenClaw("test_user", openclawResp, "zh-CN");
    
    ASSERT_TRUE(result.isSuccess());
    auto response = result.getValue();
    
    EXPECT_EQ(response.emotions.size(), 2u);
    EXPECT_EQ(response.actions.size(), 2u);
}
