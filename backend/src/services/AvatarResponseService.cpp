#include "services/AvatarResponseService.hpp"
#include "services/OpenClawGateway.hpp"
#include "services/GPTSoVITSService.hpp"
#include "services/Live2DAnimationService.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <chrono>

namespace yachiyo::services {

using AvatarResponse = AvatarResponseService::AvatarResponse;

AvatarResponseService::AvatarResponseService(
    std::shared_ptr<OpenClawGateway> openclaw,
    std::shared_ptr<GPTSoVITSService> tts,
    std::shared_ptr<Live2DAnimationService> animation
) 
    : openclaw_gateway_(openclaw),
      tts_service_(tts),
      animation_service_(animation),
      default_language_("zh-CN") {
}

AvatarResponseService::~AvatarResponseService() {
}

// ==================== 初始化 ====================

bool AvatarResponseService::initialize(
    const std::string& avatar_language
) {
    LOG_INFO("初始化 Avatar 响应服务 (精简版: 审查和翻译由 OpenClaw 统一处理)");

    if (!openclaw_gateway_) {
        LOG_ERROR("OpenClaw 网关未配置");
        return false;
    }

    if (!tts_service_) {
        LOG_ERROR("TTS 服务未配置");
        return false;
    }

    if (!animation_service_) {
        LOG_ERROR("动画服务未配置");
        return false;
    }

    default_language_ = avatar_language;

    LOG_INFO("Avatar 响应服务初始化完成");
    return true;
}

// ==================== 主处理流程 ====================

Utils::Result<AvatarResponse> AvatarResponseService::processUserMessage(
    const std::string& userId,
    const std::string& userText,
    const std::string& targetLanguage
) {
    LOG_INFO("处理用户消息: userId={}, text={}", userId, userText);

    auto startTime = std::chrono::system_clock::now();
    AvatarResponse response;
    response.requestId = "resp_" + std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()
    );
    response.userId = userId;
    response.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // ===== 步骤 1: 调用 OpenClaw 获取 AI 回复 + 审查 + 翻译 (一站式) =====
    LOG_DEBUG("调用 OpenClaw (一站式: AI对话 + 内容审查 + 翻译)");
    auto openclawResult = openclaw_gateway_->processMessage(userId, userText);

    if (!openclawResult.isSuccess()) {
        LOG_ERROR("OpenClaw 调用失败: {}", openclawResult.getError().message);
        return Utils::Result<AvatarResponse>::fail(1001, "OpenClaw 服务出错", response);
    }

    auto openclawResponse = openclawResult.getValue();

    // ===== 步骤 2: 检查审查结果 =====
    if (openclawResponse.moderation == "block") {
        LOG_WARN("OpenClaw 审查: 内容被阻止");
        response.text = openclawResponse.text;
        response.isBlocked = true;
        response.metadata["moderation"] = "block";
        return Utils::Result<AvatarResponse>::success(response);  // 不触发后续管线
    }

    response.originalText = openclawResponse.text;
    response.emotions = openclawResponse.emotions;
    response.actions = openclawResponse.actions;

    // ===== 步骤 3: 文本翻译 (使用 OpenClaw 内置翻译) =====
    std::string finalText = openclawResponse.text;

    if (!openclawResponse.translatedText.empty()
        && !targetLanguage.empty() && targetLanguage != "zh-CN") {
        finalText = openclawResponse.translatedText;
        response.translatedText = finalText;
        LOG_DEBUG("使用 OpenClaw 内置翻译: {} chars", finalText.size());
    } else {
        // OpenClaw 未返回 translation → 使用原文 (中文)
        LOG_DEBUG("OpenClaw 未返回 translation, 使用原文");
    }

    response.text = finalText;
    response.targetLanguage = targetLanguage;

    // ===== 步骤 4: 音频生成 (TTS) =====
    LOG_DEBUG("生成语音");
    std::string emotionType = openclawResponse.emotions.empty()
        ? "neutral" : openclawResponse.emotions[0];

    auto ttsResult = tts_service_->synthesizeWithEmotion(
        finalText, emotionType, GPTSoVITSService::VoicePreset::DEFAULT
    );

    if (!ttsResult.isSuccess()) {
        LOG_WARN("TTS 生成失败: {}", ttsResult.getError().message);
        response.audioUrl = "";
        response.audioDurationMs = 0;
    } else {
        auto ttsResponse = ttsResult.getValue();
        response.audioUrl = ttsResponse.audioUrl;
        response.audioDurationMs = ttsResponse.durationMs;
    }

    // ===== 步骤 5: 动画生成 =====
    LOG_DEBUG("生成动画命令");
    auto animationResult = animation_service_->generateAnimationSequence(
        openclawResponse.emotions,
        openclawResponse.actions,
        Live2DAnimationService::Model::DEFAULT
    );

    if (!animationResult.isSuccess()) {
        LOG_WARN("动画生成失败: {}", animationResult.getError().message);
        response.animationCommands.clear();
    } else {
        response.animationCommands = animationResult.getValue().commands;
    }

    // ===== 步骤 6: 计算处理时间 =====
    auto endTime = std::chrono::system_clock::now();
    response.processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    ).count();

    response.metadata["language"] = targetLanguage.empty() ? "zh-CN" : targetLanguage;
    response.metadata["processing_time_ms"] = std::to_string(response.processingTimeMs);
    response.metadata["moderation"] = "pass";

    LOG_INFO("消息处理完成 (耗时 {}ms)", response.processingTimeMs);

    return Utils::Result<AvatarResponse>::success(response);
}

// ==================== 从 OpenClaw 响应生成 =====

Utils::Result<AvatarResponse> AvatarResponseService::generateFromOpenClaw(
    const std::string& userId,
    const dto::OpenClawResponse& openclawResponse,
    const std::string& targetLanguage
) {
    LOG_INFO("从 OpenClaw 响应生成 Avatar 响应");

    AvatarResponse response;
    response.requestId = "resp_" + std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()
    );
    response.userId = userId;
    response.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    response.originalText = openclawResponse.text;
    response.emotions = openclawResponse.emotions;
    response.actions = openclawResponse.actions;

    // 翻译文本
    std::string finalText = openclawResponse.text;
    if (!targetLanguage.empty() && targetLanguage != "zh-CN"
        && !openclawResponse.translatedText.empty()) {
        finalText = openclawResponse.translatedText;
        response.translatedText = finalText;
    }

    response.text = finalText;
    response.targetLanguage = targetLanguage;

    // 生成语音
    std::string emotionForTTS = openclawResponse.emotions.empty() ?
        "neutral" : openclawResponse.emotions[0];

    auto ttsResult = tts_service_->synthesizeWithEmotion(
        finalText, emotionForTTS, GPTSoVITSService::VoicePreset::DEFAULT
    );
    if (ttsResult.isSuccess()) {
        auto ttsResponse = ttsResult.getValue();
        response.audioUrl = ttsResponse.audioUrl;
        response.audioDurationMs = ttsResponse.durationMs;
    }

    // 生成动画
    auto animationResult = animation_service_->generateAnimationSequence(
        openclawResponse.emotions,
        openclawResponse.actions
    );

    if (animationResult.isSuccess()) {
        response.animationCommands = animationResult.getValue().commands;
    }

    return Utils::Result<AvatarResponse>::success(response);
}

// ==================== 批量处理 =====

Utils::Result<std::vector<AvatarResponse>> AvatarResponseService::batchProcessMessages(
    const std::string& userId,
    const std::vector<std::string>& texts,
    const std::string& targetLanguage
) {
    LOG_INFO("批量处理消息: 共 {} 条", texts.size());

    std::vector<AvatarResponse> responses;

    for (const auto& text : texts) {
        auto result = processUserMessage(userId, text, targetLanguage);

        if (result.isSuccess()) {
            responses.push_back(result.getValue());
        } else {
            LOG_WARN("处理消息失败: {}", result.getError().message);
        }
    }

    LOG_INFO("批量处理完成: 成功 {} 条", responses.size());
    return Utils::Result<std::vector<AvatarResponse>>::success(responses);
}

// ==================== 缓存管理 =====

void AvatarResponseService::clearCache(int maxAgeSeconds) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::vector<std::string> keysToRemove;
    for (const auto& entry : response_cache_) {
        int ageSeconds = static_cast<int>((now - entry.second.timestamp) / 1000);
        if (ageSeconds > maxAgeSeconds) {
            keysToRemove.push_back(entry.first);
        }
    }

    for (const auto& key : keysToRemove) {
        response_cache_.erase(key);
    }

    LOG_INFO("缓存清理完成: 移除 {} 条记录", keysToRemove.size());
}

// ==================== 服务状态 ====================

bool AvatarResponseService::isHealthy() const {
    return openclaw_gateway_ && tts_service_ && animation_service_;
}

} // namespace yachiyo::services
