#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Yachiyo {
namespace DTO {

/**
 * OpenClaw 请求 DTO
 */
struct OpenClawRequest {
    std::string requestId;
    std::string text;
    std::string context;
    std::string userId;
    std::string conversationId;
    int maxTokens = 1000;
    float temperature = 0.7;
    std::vector<std::string> emotionHints;  // 情感提示
    
    // JSON 序列化
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        j["text"] = text;
        j["context"] = context;
        j["user_id"] = userId;
        j["conversation_id"] = conversationId;
        j["max_tokens"] = maxTokens;
        j["temperature"] = temperature;
        j["emotion_hints"] = emotionHints;
        return j;
    }
    
    // JSON 反序列化
    static OpenClawRequest fromJson(const json& j) {
        OpenClawRequest req;
        if (j.contains("request_id")) req.requestId = j["request_id"];
        if (j.contains("text")) req.text = j["text"];
        if (j.contains("context")) req.context = j["context"];
        if (j.contains("user_id")) req.userId = j["user_id"];
        if (j.contains("conversation_id")) req.conversationId = j["conversation_id"];
        if (j.contains("max_tokens")) req.maxTokens = j["max_tokens"];
        if (j.contains("temperature")) req.temperature = j["temperature"];
        if (j.contains("emotion_hints")) {
            req.emotionHints = j["emotion_hints"].get<std::vector<std::string>>();
        }
        return req;
    }
};

/**
 * OpenClaw 响应 DTO
 */
struct OpenClawResponse {
    std::string requestId;
    bool success = false;
    std::string text;
    std::vector<std::string> emotions;           // 表情提示词
    std::vector<std::string> actions;            // 基本动作命令
    int processingTimeMs = 0;
    std::string errorMessage;
    
    // JSON 序列化
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        j["success"] = success;
        j["text"] = text;
        j["emotions"] = emotions;
        j["actions"] = actions;
        j["processing_time_ms"] = processingTimeMs;
        if (!errorMessage.empty()) {
            j["error"] = errorMessage;
        }
        return j;
    }
    
    // JSON 反序列化
    static OpenClawResponse fromJson(const json& j) {
        OpenClawResponse resp;
        if (j.contains("request_id")) resp.requestId = j["request_id"];
        if (j.contains("success")) resp.success = j["success"];
        if (j.contains("text")) resp.text = j["text"];
        if (j.contains("emotions")) {
            resp.emotions = j["emotions"].get<std::vector<std::string>>();
        }
        if (j.contains("actions")) {
            resp.actions = j["actions"].get<std::vector<std::string>>();
        }
        if (j.contains("processing_time_ms")) resp.processingTimeMs = j["processing_time_ms"];
        if (j.contains("error")) resp.errorMessage = j["error"];
        return resp;
    }
};

} // namespace DTO
} // namespace Yachiyo

// 后向兼容别名
namespace yachiyo::dto {
    using Yachiyo::DTO::OpenClawRequest;
    using Yachiyo::DTO::OpenClawResponse;
}