#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include "utils/Result.hpp"
#include "utils/Compat.hpp"
#include "dto/OpenClawDTO.hpp"
#include "dto/TTSServiceDTO.hpp"
#include "dto/Live2DDTO.hpp"

using json = nlohmann::json;

namespace yachiyo::services {

class OpenClawGateway;
class GPTSoVITSService;
class Live2DAnimationService;

/**
 * Avatar 响应服务 — 整合 AI 对话 + TTS + 动画，生成完整虚拟形象响应
 *
 * 架构 (精简版):
 *   内容审查和翻译已收归 OpenClaw System Prompt 一站式处理，
 *   本服务不再依赖独立的 DeepSeekModerationService 和 TranslationService。
 *
 * 管线:
 *   用户消息 → [1] OpenClaw (对话+审查+翻译) → [2] TTS → [3] Live2D动画 → 响应
 */
class AvatarResponseService {
public:
    struct AvatarResponse {
        std::string responseId;
        std::string requestId;
        std::string messageId;
        std::string userId;

        std::string text;
        std::string originalText;
        std::string translatedText;
        std::string targetLanguage;

        std::vector<std::string> emotions;
        std::vector<std::string> actions;

        std::string audioUrl;
        std::string audioBase64;
        int audioDurationMs = 0;

        std::vector<dto::Live2DCommand> animationCommands;
        int totalDurationMs = 0;

        bool isBlocked = false;
        std::map<std::string, std::string> metadata;

        int64_t processingTimeMs = 0;
        int64_t timestamp = 0;
    };

    AvatarResponseService(
        std::shared_ptr<OpenClawGateway> openclaw,
        std::shared_ptr<GPTSoVITSService> tts,
        std::shared_ptr<Live2DAnimationService> animation
    );
    ~AvatarResponseService();

    bool initialize(const std::string& avatar_language = "zh-CN");

    Utils::Result<AvatarResponse> processUserMessage(
        const std::string& userId,
        const std::string& userMessage,
        const std::string& targetLanguage = "ja"
    );

    Utils::Result<AvatarResponse> generateFromOpenClaw(
        const std::string& userId,
        const dto::OpenClawResponse& openClawResponse,
        const std::string& targetLanguage = "ja"
    );

    Utils::Result<std::vector<AvatarResponse>> batchProcessMessages(
        const std::string& userId,
        const std::vector<std::string>& texts,
        const std::string& targetLanguage = "ja"
    );

    void clearCache(int maxAgeSeconds = 3600);
    bool isHealthy() const;

private:
    std::shared_ptr<OpenClawGateway> openclaw_gateway_;
    std::shared_ptr<GPTSoVITSService> tts_service_;
    std::shared_ptr<Live2DAnimationService> animation_service_;

    std::string default_language_;
    std::mutex cache_mutex_;
    std::map<std::string, AvatarResponse> response_cache_;
};

} // namespace yachiyo::services
