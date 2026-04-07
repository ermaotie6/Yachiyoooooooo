#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Yachiyo {
namespace DTO {

/**
 * 内容审查请求 DTO
 */
struct ModerationRequest {
    std::string requestId;
    std::string content;
    std::vector<std::string> emotionTags;
    std::string language = "zh";
    float threshold = 0.5;  // 风险阈值
    
    // JSON 序列化
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        j["content"] = content;
        j["emotion_tags"] = emotionTags;
        j["language"] = language;
        j["threshold"] = threshold;
        return j;
    }
    
    // JSON 反序列化
    static ModerationRequest fromJson(const json& j) {
        ModerationRequest req;
        if (j.contains("request_id")) req.requestId = j["request_id"];
        if (j.contains("content")) req.content = j["content"];
        if (j.contains("emotion_tags")) {
            req.emotionTags = j["emotion_tags"].get<std::vector<std::string>>();
        }
        if (j.contains("language")) req.language = j["language"];
        if (j.contains("threshold")) req.threshold = j["threshold"];
        return req;
    }
};

/**
 * 审查结果 DTO (单个内容)
 */
struct ModerationResult {
    std::string category;          // 违规类别
    float riskScore = 0.0;         // 风险分数 [0.0-1.0]
    std::string verdict;           // pass / review / block
    std::string reason;
    
    // JSON 序列化
    json toJson() const {
        json j;
        j["category"] = category;
        j["risk_score"] = riskScore;
        j["verdict"] = verdict;
        j["reason"] = reason;
        return j;
    }
};

/**
 * 内容审查响应 DTO
 */
struct ModerationResponse {
    std::string requestId;
    bool success = false;
    std::string content;
    std::vector<ModerationResult> results;
    std::string overallVerdict = "pass";        // pass / review / block
    float overallRiskScore = 0.0;
    std::vector<std::string> emotionVerification;  // 已验证的情感
    std::string recommendedAction;               // 建议的动作
    int processingTimeMs = 0;
    std::string errorMessage;
    
    // JSON 序列化
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        j["success"] = success;
        j["content"] = content;
        
        json resultsArray = json::array();
        for (const auto& result : results) {
            resultsArray.push_back(result.toJson());
        }
        j["results"] = resultsArray;
        
        j["overall_verdict"] = overallVerdict;
        j["overall_risk_score"] = overallRiskScore;
        j["emotion_verification"] = emotionVerification;
        j["recommended_action"] = recommendedAction;
        j["processing_time_ms"] = processingTimeMs;
        
        if (!errorMessage.empty()) {
            j["error"] = errorMessage;
        }
        
        return j;
    }
    
    // JSON 反序列化
    static ModerationResponse fromJson(const json& j) {
        ModerationResponse resp;
        if (j.contains("request_id")) resp.requestId = j["request_id"];
        if (j.contains("success")) resp.success = j["success"];
        if (j.contains("content")) resp.content = j["content"];
        
        if (j.contains("results")) {
            for (const auto& resultJson : j["results"]) {
                ModerationResult result;
                if (resultJson.contains("category")) result.category = resultJson["category"];
                if (resultJson.contains("risk_score")) result.riskScore = resultJson["risk_score"];
                if (resultJson.contains("verdict")) result.verdict = resultJson["verdict"];
                if (resultJson.contains("reason")) result.reason = resultJson["reason"];
                resp.results.push_back(result);
            }
        }
        
        if (j.contains("overall_verdict")) resp.overallVerdict = j["overall_verdict"];
        if (j.contains("overall_risk_score")) resp.overallRiskScore = j["overall_risk_score"];
        if (j.contains("emotion_verification")) {
            resp.emotionVerification = j["emotion_verification"].get<std::vector<std::string>>();
        }
        if (j.contains("recommended_action")) resp.recommendedAction = j["recommended_action"];
        if (j.contains("processing_time_ms")) resp.processingTimeMs = j["processing_time_ms"];
        if (j.contains("error")) resp.errorMessage = j["error"];
        
        return resp;
    }
};

} // namespace DTO
} // namespace Yachiyo

// 后向兼容别名
namespace yachiyo::dto {
    using Yachiyo::DTO::ModerationRequest;
    using Yachiyo::DTO::ModerationResult;
    using Yachiyo::DTO::ModerationResponse;
}