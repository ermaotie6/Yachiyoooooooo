#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Yachiyo {
namespace DTO {

/**
 * 翻译请求 DTO
 */
struct TranslationRequest {
    std::string requestId;
    std::string text;
    std::string sourceLanguage = "zh";
    std::string targetLanguage;
    std::string style = "casual";  // casual / formal
    bool preserveFormatting = true;
    bool preserveEmotions = true;
    bool cacheEnabled = true;
    int maxWaitMs = 3000;
    
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        j["text"] = text;
        j["source_language"] = sourceLanguage;
        j["target_language"] = targetLanguage;
        j["style"] = style;
        j["preserve_formatting"] = preserveFormatting;
        j["preserve_emotions"] = preserveEmotions;
        j["cache_enabled"] = cacheEnabled;
        j["max_wait_ms"] = maxWaitMs;
        return j;
    }
    
    static TranslationRequest fromJson(const json& j) {
        TranslationRequest req;
        if (j.contains("request_id")) req.requestId = j["request_id"];
        if (j.contains("text")) req.text = j["text"];
        if (j.contains("source_language")) req.sourceLanguage = j["source_language"];
        if (j.contains("target_language")) req.targetLanguage = j["target_language"];
        if (j.contains("style")) req.style = j["style"];
        if (j.contains("preserve_formatting")) req.preserveFormatting = j["preserve_formatting"];
        if (j.contains("preserve_emotions")) req.preserveEmotions = j["preserve_emotions"];
        if (j.contains("cache_enabled")) req.cacheEnabled = j["cache_enabled"];
        if (j.contains("max_wait_ms")) req.maxWaitMs = j["max_wait_ms"];
        return req;
    }
};

/**
 * 翻译响应 DTO
 */
struct TranslationResponse {
    std::string requestId;
    bool success = false;
    std::string sourceText;
    std::string translatedText;
    std::string sourceLanguage;
    std::string targetLanguage;
    float confidence = 0.0;
    float qualityScore = 0.0;
    bool cached = false;
    std::string engineUsed;
    int processingTimeMs = 0;
    std::vector<std::string> preservedEmotions;
    std::string errorMessage;
    
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        j["success"] = success;
        j["source_text"] = sourceText;
        j["translated_text"] = translatedText;
        j["source_language"] = sourceLanguage;
        j["target_language"] = targetLanguage;
        j["confidence"] = confidence;
        j["quality_score"] = qualityScore;
        j["cached"] = cached;
        j["engine_used"] = engineUsed;
        j["processing_time_ms"] = processingTimeMs;
        j["preserved_emotions"] = preservedEmotions;
        if (!errorMessage.empty()) {
            j["error"] = errorMessage;
        }
        return j;
    }
    
    static TranslationResponse fromJson(const json& j) {
        TranslationResponse resp;
        if (j.contains("request_id")) resp.requestId = j["request_id"];
        if (j.contains("success")) resp.success = j["success"];
        if (j.contains("source_text")) resp.sourceText = j["source_text"];
        if (j.contains("translated_text")) resp.translatedText = j["translated_text"];
        if (j.contains("source_language")) resp.sourceLanguage = j["source_language"];
        if (j.contains("target_language")) resp.targetLanguage = j["target_language"];
        if (j.contains("confidence")) resp.confidence = j["confidence"];
        if (j.contains("quality_score")) resp.qualityScore = j["quality_score"];
        if (j.contains("cached")) resp.cached = j["cached"];
        if (j.contains("engine_used")) resp.engineUsed = j["engine_used"];
        if (j.contains("processing_time_ms")) resp.processingTimeMs = j["processing_time_ms"];
        if (j.contains("preserved_emotions")) {
            resp.preservedEmotions = j["preserved_emotions"].get<std::vector<std::string>>();
        }
        if (j.contains("error")) resp.errorMessage = j["error"];
        return resp;
    }
};

} // namespace DTO
} // namespace Yachiyo

// 后向兼容别名
namespace yachiyo::dto {
    using Yachiyo::DTO::TranslationRequest;
    using Yachiyo::DTO::TranslationResponse;
}