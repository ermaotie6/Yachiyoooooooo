#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "services/AvatarResponseService.hpp"
#include "services/OpenClawGateway.hpp"
#include "services/TranslationService.hpp"
#include "services/GPTSoVITSService.hpp"
#include "services/Live2DAnimationService.hpp"
#include "services/DeepSeekModerationService.hpp"
#include "dto/CommonDTO.hpp"

namespace yachiyo::tests {

// ==================== Mock 服务基类 ====================

class MockOpenClawGateway {
public:
    virtual ~MockOpenClawGateway() = default;
    
    virtual Utils::Result<dto::OpenClawResponse> processMessage(
        const std::string& userId,
        const std::string& text
    );
};

class MockTranslationService {
public:
    virtual ~MockTranslationService() = default;
    
    virtual Utils::Result<dto::TranslationResponse> translate(
        const std::string& text,
        const std::string& sourceLang,
        const std::string& targetLang,
        TranslationService::Engine engine = TranslationService::Engine::AUTO
    );
};

class MockGPTSoVITSService {
public:
    virtual ~MockGPTSoVITSService() = default;
    
    virtual Utils::Result<dto::TTSResponse> synthesizeWithEmotion(
        const dto::TTSRequest& request
    );
};

class MockLive2DAnimationService {
public:
    virtual ~MockLive2DAnimationService() = default;
    
    virtual Utils::Result<dto::Live2DSequenceRequest> generateAnimationSequence(
        const std::vector<std::string>& emotions,
        const std::vector<std::string>& actions,
        Live2DAnimationService::Model model = Live2DAnimationService::Model::DEFAULT
    );
};

class MockDeepSeekModerationService {
public:
    virtual ~MockDeepSeekModerationService() = default;
    
    virtual Utils::Result<dto::ModerationResult> moderate(
        const std::string& text
    );
};

// ==================== 集成测试基类 ====================

class AvatarResponseServiceTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    
    // 初始化所有服务
    void initializeAllServices();
    
    // 创建 Mock 服务
    void createMockServices();
    
    // 测试工具方法
    std::string generateTestUserId();
    std::vector<std::string> generateTestMessages();
    
    // 成员变量
    std::shared_ptr<services::AvatarResponseService> avatar_service_;
    std::shared_ptr<services::OpenClawGateway> openclaw_gateway_;
    std::shared_ptr<services::TranslationService> translation_service_;
    std::shared_ptr<services::GPTSoVITSService> tts_service_;
    std::shared_ptr<services::Live2DAnimationService> animation_service_;
    std::shared_ptr<services::DeepSeekModerationService> moderation_service_;
};

// ==================== 具体测试 ====================

class TestAvatarServiceInitialization : public AvatarResponseServiceTest {};
class TestUserMessageProcessing : public AvatarResponseServiceTest {};
class TestTranslationIntegration : public AvatarResponseServiceTest {};
class TestContentModeration : public AvatarResponseServiceTest {};
class TestEmotionMapping : public AvatarResponseServiceTest {};
class TestPerformance : public AvatarResponseServiceTest {};

} // namespace yachiyo::tests
