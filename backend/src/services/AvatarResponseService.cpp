#include "services/AvatarResponseService.hpp"
#include "services/OpenClawGateway.hpp"
#include "services/TranslationService.hpp"
#include "services/GPTSoVITSService.hpp"
#include "services/Live2DAnimationService.hpp"
#include "services/DeepSeekModerationService.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <chrono>

namespace yachiyo::services {

// 类型别名简化嵌套类型的使用
using AvatarResponse = AvatarResponseService::AvatarResponse;

// ==================== 构造/析构 ====================

AvatarResponseService::AvatarResponseService(
    std::shared_ptr<OpenClawGateway> openclaw,
    std::shared_ptr<TranslationService> translation,
    std::shared_ptr<GPTSoVITSService> tts,
    std::shared_ptr<Live2DAnimationService> animation,
    std::shared_ptr<DeepSeekModerationService> moderation
) 
    : openclaw_gateway_(openclaw),
      translation_service_(translation),
      tts_service_(tts),
      animation_service_(animation),
      moderation_service_(moderation),
      default_language_("zh-CN") {
}

AvatarResponseService::~AvatarResponseService() {
}

// ==================== 初始化 ====================

bool AvatarResponseService::initialize(
    const std::string& avatar_language
) {
    LOG_INFO("初始化 Avatar 响应服务");
    
    if (!openclaw_gateway_) {
        LOG_ERROR("OpenClaw 网关未配置");
        return false;
    }
    // OpenClaw 网关已在 Application.cpp 中通过配置文件初始化，
    // 此处不再重复初始化，避免用硬编码 URL 覆盖配置值。
    
    if (!tts_service_) {
        LOG_ERROR("TTS 服务未配置");
        return false;
    }
    // 注意: tts_service_ 已在 Application.cpp 中通过 initialize(endpoint, mode) 初始化，
    // 此处不再调用无参 initialize()，否则会覆盖已配置的 endpoint 和 mode。
    
    if (!animation_service_) {
        LOG_ERROR("动画服务未配置");
        return false;
    }
    // 注意: animation_service_ 已在 Application.cpp 中初始化。
    
    // 注意: translation_service_ 和 moderation_service_ 也已在 Application.cpp 中初始化，
    // 此处仅做空指针检查，不再重复调用 initialize()。
    
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
    
    // ===== 步骤1: 内容审核 =====
    if (moderation_service_) {
        LOG_DEBUG("执行内容审核");
        ModerationRequest modReq;
        modReq.content = userText;
        auto moderationResult = moderation_service_->moderate(modReq);
        
        if (!moderationResult.isSuccess()) {
            LOG_WARN("审核失败: {}", moderationResult.getError().message);
        } else {
            auto verdict = moderationResult.getValue().overallVerdict;
            response.metadata["moderation_verdict"] = verdict;
            
            if (verdict == "block") {
                LOG_WARN("内容被阻止");
                response.text = "抱歉，您的消息包含不当内容。";
                response.isBlocked = true;
                return Utils::Result<AvatarResponse>::success(response);
            }
        }
    }
    
    // ===== 步骤2: 调用 OpenClaw 获取 AI 回复 =====
    LOG_DEBUG("调用 OpenClaw 获取 AI 回复");
    auto openclawResult = openclaw_gateway_->processMessage(userId, userText);
    
    if (!openclawResult.isSuccess()) {
        LOG_ERROR("OpenClaw 调用失败: {}", openclawResult.getError().message);
        return Utils::Result<AvatarResponse>::fail(
            1001, "OpenClaw 服务出错", response
        );
    }
    
    auto openclawResponse = openclawResult.getValue();
    response.originalText = openclawResponse.text;
    response.emotions = openclawResponse.emotions;
    response.actions = openclawResponse.actions;
    
    // ===== 步骤3: 文本翻译 =====
    std::string finalText = openclawResponse.text;
    
    if (!targetLanguage.empty() && targetLanguage != "zh-CN") {
        if (translation_service_) {
            LOG_DEBUG("翻译文本为: {}", targetLanguage);
            auto translationResult = translation_service_->translate(
                openclawResponse.text,
                "zh-CN",
                targetLanguage,
                TranslationService::Engine::AUTO
            );
            
            if (translationResult.isSuccess()) {
                finalText = translationResult.getValue().translatedText;
                response.translatedText = finalText;
            } else {
                LOG_WARN("翻译失败，使用原文: {}", translationResult.getError().message);
            }
        }
    }
    
    response.text = finalText;
    response.targetLanguage = targetLanguage;
    
    // ===== 步骤4: 音频生成 (TTS) =====
    LOG_DEBUG("生成语音");
    std::string emotionType = "neutral";
    if (!openclawResponse.emotions.empty()) {
        emotionType = openclawResponse.emotions[0];
    }
    
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
    
    // ===== 步骤5: 动画生成 =====
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
        auto animationSeq = animationResult.getValue();
        response.animationCommands = animationSeq.commands;
    }
    
    // ===== 步骤6: 嘴部同步由前端 Web Audio API AnalyserNode 实时驱动 =====
    // 不再在后端生成 mock 嘴部同步命令，避免与前端实时分析冲突
    LOG_DEBUG("嘴部同步将由前端实时音频分析驱动");
    
    // ===== 步骤7: 计算处理时间 =====
    auto endTime = std::chrono::system_clock::now();
    int processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    ).count();
    
    response.processingTimeMs = processingTimeMs;
    response.metadata["language"] = targetLanguage.empty() ? "zh-CN" : targetLanguage;
    response.metadata["processing_time_ms"] = std::to_string(processingTimeMs);
    
    LOG_INFO("消息处理完成 (耗时 {}ms)", processingTimeMs);
    
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
    if (!targetLanguage.empty() && targetLanguage != "zh-CN" && translation_service_) {
        auto translationResult = translation_service_->translate(
            openclawResponse.text, "zh-CN", targetLanguage
        );
        if (translationResult.isSuccess()) {
            finalText = translationResult.getValue().translatedText;
            response.translatedText = finalText;
        }
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
