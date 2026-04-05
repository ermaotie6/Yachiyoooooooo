#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include "utils/Result.hpp"
#include "dto/OpenClawDTO.hpp"
#include "dto/TranslationDTO.hpp"
#include "dto/TTSServiceDTO.hpp"
#include "dto/Live2DDTO.hpp"

using json = nlohmann::json;

namespace yachiyo::services {

// 前向声明
class OpenClawGateway;
class TranslationService;
class GPTSoVITSService;
class Live2DAnimationService;
class DeepSeekModerationService;

/**
 * 虚拱响应服务 - 整合所有服务生成完整的虚拱响应
 * 
 * 职责:
 * 1. 接收用户消息
 * 2. 调用 OpenClaw 生成初始响应 (文本 + 情感 + 动作)
 * 3. 调用翻译服务翻译为目标语言
 * 4. 调用 TTS 合成语音
 * 5. 调用 Live2D 生成动画命令
 * 6. 组装完整响应并返回给前端
 */
class AvatarResponseService {
public:
    struct AvatarResponse {
        std::string responseId;
        std::string requestId;
        std::string messageId;
        std::string userId;
        
        // 文本部分
        std::string text;                // 最终文本 (翻译后或原文)
        std::string originalText;        // 中文原文
        std::string translatedText;      // 翻译后文本
        std::string targetLanguage;      // 目标语言
        
        // 情感和动作
        std::vector<std::string> emotions;
        std::vector<std::string> actions;
        
        // 音频部分
        std::string audioUrl;
        std::string audioBase64;
        int audioDurationMs = 0;
        
        // 动画部分
        std::vector<dto::Live2DCommand> animationCommands;
        int totalDurationMs = 0;
        
        // 状态
        bool isBlocked = false;
        std::map<std::string, std::string> metadata;
        
        // 元数据
        int64_t processingTimeMs = 0;
        int64_t timestamp = 0;
    };
    
    AvatarResponseService(
        std::shared_ptr<OpenClawGateway> openclaw,
        std::shared_ptr<TranslationService> translation,
        std::shared_ptr<GPTSoVITSService> tts,
        std::shared_ptr<Live2DAnimationService> animation,
        std::shared_ptr<DeepSeekModerationService> moderation = nullptr
    );
    ~AvatarResponseService();
    
    /**
     * 初始化服务
     * @param avatar_language 虚拟形象默认语言
     */
    bool initialize(const std::string& avatar_language = "zh-CN");
    
    /**
     * 处理用户消息并生成完整虚拱响应
     * @param messageId 消息ID
     * @param userId 用户ID
     * @param userMessage 用户输入的消息
     * @param targetLanguage 目标语言 (默认为日文)
     * @return 完整的虚拱响应
     */
    Utils::Result<AvatarResponse> processUserMessage(
        const std::string& messageId,
        const std::string& userId,
        const std::string& userMessage,
        const std::string& targetLanguage = "ja"
    );
    
    /**
     * 从 OpenClaw 响应生成虚拱响应
     * @param messageId 消息ID
     * @param userId 用户ID
     * @param openClawResponse OpenClaw 的响应
     * @param targetLanguage 目标语言
     * @return 完整的虚拱响应
     */
    Utils::Result<AvatarResponse> generateFromOpenClaw(
        const std::string& messageId,
        const std::string& userId,
        const dto::OpenClawResponse& openClawResponse,
        const std::string& targetLanguage = "ja"
    );
    
    /**
     * 获取服务状态
     */
    bool isHealthy() const;

private:
    std::shared_ptr<OpenClawGateway> openclaw_gateway_;
    std::shared_ptr<TranslationService> translation_service_;
    std::shared_ptr<GPTSoVITSService> tts_service_;
    std::shared_ptr<Live2DAnimationService> animation_service_;
    std::shared_ptr<DeepSeekModerationService> moderation_service_;
    
    std::string default_language_;
    std::mutex service_mutex_;
    std::mutex cache_mutex_;
    std::map<std::string, AvatarResponse> response_cache_;
    
    /**
     * 内部处理流程步骤
     */
    
    // 步骤 1: 调用 OpenClaw
    Utils::Result<dto::OpenClawResponse> callOpenClaw(
        const std::string& userId,
        const std::string& message
    );
    
    // 步骤 2: 翻译文本
    Utils::Result<dto::TranslationResponse> translateText(
        const std::string& text,
        const std::string& targetLanguage
    );
    
    // 步骤 3: 合成语音
    Utils::Result<dto::TTSResponse> synthesizeVoice(
        const std::string& text,
        const std::vector<std::string>& emotions
    );
    
    // 步骤 4: 生成动画命令
    Utils::Result<dto::Live2DSequenceRequest> generateAnimationCommands(
        const std::vector<std::string>& emotions,
        const std::vector<std::string>& actions
    );
    
    /**
     * 组装最终响应
     */
    AvatarResponse assembleResponse(
        const std::string& messageId,
        const std::string& userId,
        const dto::OpenClawResponse& openclaw,
        const dto::TranslationResponse& translation,
        const dto::TTSResponse& tts,
        const dto::Live2DSequenceRequest& animation,
        int64_t startTime
    );
};

} // namespace yachiyo::services
