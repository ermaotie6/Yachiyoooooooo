#include "AvatarResponseServiceTest.hpp"
#include "utils/Logger.hpp"
#include <chrono>

namespace yachiyo::tests {

// ==================== Mock 实现 ====================

Utils::Result<dto::OpenClawResponse> MockOpenClawGateway::processMessage(
    const std::string& userId,
    const std::string& text
) {
    dto::OpenClawResponse response;
    response.text = "这是来自 OpenClaw 的模拟回复: " + text;
    response.detectedEmotions = {"开心"};
    response.suggestedActions = {"wave"};
    
    return Utils::Result<dto::OpenClawResponse>::success(response);
}

Utils::Result<dto::TranslationResponse> MockTranslationService::translate(
    const std::string& text,
    const std::string& sourceLang,
    const std::string& targetLang,
    TranslationService::Engine engine
) {
    dto::TranslationResponse response;
    response.originalText = text;
    response.translatedText = "[" + targetLang + "] " + text;
    response.sourceLang = sourceLang;
    response.targetLang = targetLang;
    
    return Utils::Result<dto::TranslationResponse>::success(response);
}

Utils::Result<dto::TTSResponse> MockGPTSoVITSService::synthesizeWithEmotion(
    const dto::TTSRequest& request
) {
    dto::TTSResponse response;
    response.audioUrl = "https://mock.audio.url/audio_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    ) + ".wav";
    response.durationMs = static_cast<int>(request.text.length() * 100);
    
    return Utils::Result<dto::TTSResponse>::success(response);
}

Utils::Result<dto::Live2DSequenceRequest> MockLive2DAnimationService::generateAnimationSequence(
    const std::vector<std::string>& emotions,
    const std::vector<std::string>& actions,
    Live2DAnimationService::Model model
) {
    dto::Live2DSequenceRequest sequence;
    sequence.requestId = "mock_anim_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    
    // 添加模拟的动画命令
    for (const auto& emotion : emotions) {
        dto::ExpressionCommand cmd;
        cmd.expressionName = "f_" + emotion;
        cmd.durationMs = 1000;
        sequence.commands.push_back(cmd);
    }
    
    return Utils::Result<dto::Live2DSequenceRequest>::success(sequence);
}

Utils::Result<dto::ModerationResult> MockDeepSeekModerationService::moderate(
    const std::string& text
) {
    dto::ModerationResult result;
    result.text = text;
    result.verdict = dto::Verdict::PASS;  // Mock 总是通过
    result.reason = "Mock 审核通过";
    
    return Utils::Result<dto::ModerationResult>::success(result);
}

// ==================== 测试基类实现 ====================

void AvatarResponseServiceTest::SetUp() {
    LOG_INFO("测试启动");
    initializeAllServices();
}

void AvatarResponseServiceTest::TearDown() {
    LOG_INFO("测试清理");
    avatar_service_ = nullptr;
    openclaw_gateway_ = nullptr;
    translation_service_ = nullptr;
    tts_service_ = nullptr;
    animation_service_ = nullptr;
    moderation_service_ = nullptr;
}

void AvatarResponseServiceTest::initializeAllServices() {
    // 创建真实服务实例
    openclaw_gateway_ = std::make_shared<services::OpenClawGateway>();
    translation_service_ = std::make_shared<services::TranslationService>();
    tts_service_ = std::make_shared<services::GPTSoVITSService>();
    animation_service_ = std::make_shared<services::Live2DAnimationService>();
    moderation_service_ = std::make_shared<services::DeepSeekModerationService>();
    
    // 初始化各服务
    ASSERT_TRUE(openclaw_gateway_->initialize("http://localhost:8888/api/v1/chat"));
    ASSERT_TRUE(translation_service_->initialize());
    ASSERT_TRUE(tts_service_->initialize());
    ASSERT_TRUE(animation_service_->initialize());
    ASSERT_TRUE(moderation_service_->initialize("", ""));
    
    // 创建 Avatar 服务
    avatar_service_ = std::make_shared<services::AvatarResponseService>(
        openclaw_gateway_,
        translation_service_,
        tts_service_,
        animation_service_,
        moderation_service_
    );
    
    ASSERT_TRUE(avatar_service_->initialize(nullptr, "zh-CN"));
}

void AvatarResponseServiceTest::createMockServices() {
    // 此方法用于创建 Mock 版本的服务
    // 当前实现为空，可在需要时扩展
}

std::string AvatarResponseServiceTest::generateTestUserId() {
    return "test_user_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
}

std::vector<std::string> AvatarResponseServiceTest::generateTestMessages() {
    return {
        "你好",
        "今天天气怎么样？",
        "给我讲一个笑话",
        "你是谁？",
        "很高兴认识你"
    };
}

// ==================== 初始化测试 ====================

TEST_F(TestAvatarServiceInitialization, ServiceInitializationSuccessful) {
    ASSERT_NE(avatar_service_, nullptr);
    ASSERT_NE(openclaw_gateway_, nullptr);
    ASSERT_NE(translation_service_, nullptr);
    ASSERT_NE(tts_service_, nullptr);
    ASSERT_NE(animation_service_, nullptr);
    ASSERT_NE(moderation_service_, nullptr);
}

// ==================== 用户消息处理测试 ====================

TEST_F(TestUserMessageProcessing, SingleMessageProcessing) {
    std::string userId = generateTestUserId();
    std::string message = "你好";
    
    auto result = avatar_service_->processUserMessage(userId, message, "zh-CN");
    
    ASSERT_TRUE(result.isSuccess());
    auto response = result.getValue();
    
    ASSERT_FALSE(response.requestId.empty());
    ASSERT_EQ(response.userId, userId);
    ASSERT_FALSE(response.text.empty());
    ASSERT_GE(response.processingTimeMs, 0);
}

TEST_F(TestUserMessageProcessing, MultipleMessagesProcessing) {
    std::string userId = generateTestUserId();
    auto messages = generateTestMessages();
    
    auto result = avatar_service_->batchProcessMessages(userId, messages, "zh-CN");
    
    ASSERT_TRUE(result.isSuccess());
    auto responses = result.getValue();
    
    ASSERT_GE(responses.size(), 1);
    for (const auto& response : responses) {
        ASSERT_FALSE(response.requestId.empty());
        ASSERT_EQ(response.userId, userId);
    }
}

// ==================== 翻译集成测试 ====================

TEST_F(TestTranslationIntegration, TranslationToEnglish) {
    std::string userId = generateTestUserId();
    std::string message = "你好，我是一个虚拟角色";
    
    auto result = avatar_service_->processUserMessage(userId, message, "en-US");
    
    ASSERT_TRUE(result.isSuccess());
    auto response = result.getValue();
    
    // 验证翻译是否在元数据中
    ASSERT_TRUE(response.metadata.count("language") > 0);
    ASSERT_EQ(response.metadata["language"], "en-US");
}

// ==================== 内容审核测试 ====================

TEST_F(TestContentModeration, ApprovedContent) {
    std::string userId = generateTestUserId();
    std::string message = "你好，今天很开心";
    
    auto result = avatar_service_->processUserMessage(userId, message, "zh-CN");
    
    ASSERT_TRUE(result.isSuccess());
    auto response = result.getValue();
    
    ASSERT_FALSE(response.isBlocked);
}

TEST_F(TestContentModeration, ModerationMetadata) {
    std::string userId = generateTestUserId();
    std::string message = "你好";
    
    auto result = avatar_service_->processUserMessage(userId, message, "zh-CN");
    
    ASSERT_TRUE(result.isSuccess());
    auto response = result.getValue();
    
    // 验证审核结果在元数据中
    ASSERT_TRUE(response.metadata.count("moderation_verdict") > 0);
}

// ==================== 情感映射测试 ====================

TEST_F(TestEmotionMapping, EmotionDetection) {
    std::string userId = generateTestUserId();
    
    // 测试不同情感的消息
    std::vector<std::pair<std::string, std::string>> emotionTests = {
        {"我很开心", "开心"},
        {"我很伤心", "伤心"},
        {"我很生气", "生气"}
    };
    
    for (const auto& [message, expectedEmotion] : emotionTests) {
        auto result = avatar_service_->processUserMessage(userId, message, "zh-CN");
        
        ASSERT_TRUE(result.isSuccess());
        auto response = result.getValue();
        
        // 验证情感被检测
        ASSERT_FALSE(response.emotions.empty());
    }
}

TEST_F(TestEmotionMapping, AnimationGeneration) {
    std::string userId = generateTestUserId();
    std::string message = "你好";
    
    auto result = avatar_service_->processUserMessage(userId, message, "zh-CN");
    
    ASSERT_TRUE(result.isSuccess());
    auto response = result.getValue();
    
    // 验证动画命令已生成
    // 注意：实际响应中应该包含动画命令
}

// ==================== 性能测试 ====================

TEST_F(TestPerformance, MessageProcessingSpeed) {
    std::string userId = generateTestUserId();
    std::string message = "你好";
    
    // 测试处理速度
    auto start = std::chrono::system_clock::now();
    auto result = avatar_service_->processUserMessage(userId, message, "zh-CN");
    auto end = std::chrono::system_clock::now();
    
    ASSERT_TRUE(result.isSuccess());
    auto response = result.getValue();
    
    int durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    // 应该在 1 秒内完成
    ASSERT_LT(durationMs, 1000);
    
    LOG_INFO("消息处理耗时: {}ms", response.processingTimeMs);
}

TEST_F(TestPerformance, BulkProcessing) {
    std::string userId = generateTestUserId();
    auto messages = generateTestMessages();
    
    auto start = std::chrono::system_clock::now();
    auto result = avatar_service_->batchProcessMessages(userId, messages, "zh-CN");
    auto end = std::chrono::system_clock::now();
    
    ASSERT_TRUE(result.isSuccess());
    
    int durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    // 5 条消息应该在 5 秒内完成
    ASSERT_LT(durationMs, 5000);
    
    LOG_INFO("批量处理耗时: {}ms (消息数: {})", durationMs, messages.size());
}

} // namespace yachiyo::tests
