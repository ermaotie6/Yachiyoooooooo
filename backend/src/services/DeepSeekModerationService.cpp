#include "services/DeepSeekModerationService.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>

namespace yachiyo::services {

using json = nlohmann::json;

// ==================== 回调函数 ====================

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ==================== 构造/析构 ====================

DeepSeekModerationService::DeepSeekModerationService() 
    : endpoint_("https://api.deepseek.com"),
      enable_emotion_verification_(true) {
}

DeepSeekModerationService::~DeepSeekModerationService() {
}

// ==================== 初始化 ====================

bool DeepSeekModerationService::initialize(
    const std::string& apiKey,
    const std::string& endpoint
) {
    LOG_INFO("初始化 DeepSeek 内容审核服务");
    
    if (!apiKey.empty()) {
        api_key_ = apiKey;
    }
    if (!endpoint.empty()) {
        endpoint_ = endpoint;
    }
    
    LOG_INFO("DeepSeek 内容审核服务初始化完成 (endpoint: {})", endpoint_);
    return true;
}

// ==================== 内容审核 ====================

// ==================== 内容审核 ====================

Utils::Result<ModerationResponse> DeepSeekModerationService::moderate(
    const ModerationRequest& request
) {
    LOG_DEBUG("审核文本内容: {}", request.content.substr(0, 50));
    
    auto startTime = std::chrono::steady_clock::now();
    
    // 1. 检查缓存
    std::string cacheKey = generateCacheKey(request.content);
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(cacheKey);
        if (it != cache_.end()) {
            auto ageSeconds = (std::chrono::system_clock::now().time_since_epoch().count() - 
                             it->second.timestamp) / 1000000000;
            if (ageSeconds < 3600) {
                LOG_DEBUG("返回缓存审核结果");
                auto cached = it->second.response;
                cached.processingTimeMs = static_cast<int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - startTime
                    ).count());
                return Utils::Result<ModerationResponse>::success(cached);
            }
        }
    }
    
    // 2. 调用 DeepSeek API（或回退到规则审核）
    ModerationResponse apiResult = callDeepSeekAPI(request.content);
    apiResult.requestId = request.requestId;
    apiResult.content = request.content;
    apiResult.success = true;
    apiResult.processingTimeMs = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime
        ).count());
    
    // 3. 如果有情感标签，进行情感验证
    if (enable_emotion_verification_ && !request.emotionTags.empty()) {
        auto emotionResult = verifyEmotions(request.content, request.emotionTags);
        if (emotionResult.isSuccess()) {
            apiResult.emotionVerification = emotionResult.getData().value().matchedEmotions;
        }
    }
    
    // 4. 缓存结果
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_[cacheKey] = {apiResult, std::chrono::system_clock::now().time_since_epoch().count()};
    }
    
    // 5. 记录统计
    recordStatistic(apiResult);
    
    return Utils::Result<ModerationResponse>::success(apiResult);
}

// ==================== 批量审核 ====================

std::vector<Utils::Result<ModerationResponse>> DeepSeekModerationService::batchModerate(
    const std::vector<ModerationRequest>& requests
) {
    LOG_INFO("批量审核: 共 {} 条文本", requests.size());
    
    std::vector<Utils::Result<ModerationResponse>> results;
    for (const auto& request : requests) {
        results.push_back(moderate(request));
    }
    
    LOG_INFO("批量审核完成: {} 条", results.size());
    return results;
}

// ==================== 情感验证 ====================

Utils::Result<DeepSeekModerationService::EmotionVerificationResult> DeepSeekModerationService::verifyEmotions(
    const std::string& text,
    const std::vector<std::string>& emotions
) {
    LOG_DEBUG("验证情感: {} 个情感", emotions.size());
    
    EmotionVerificationResult result;
    result.isValid = true;
    result.riskScore = 0.0f;
    result.confidence = 1.0f;
    
    // 定义不适当的情感
    std::vector<std::string> risky_emotions = {"愤怒", "仇恨", "绝望"};
    
    for (const auto& emotion : emotions) {
        bool isRisky = false;
        for (const auto& risky : risky_emotions) {
            if (emotion.find(risky) != std::string::npos) {
                result.riskScore += 0.3f;
                result.unmatchedEmotions.push_back(emotion);
                isRisky = true;
                break;
            }
        }
        if (!isRisky) {
            result.matchedEmotions.push_back(emotion);
        }
    }
    
    if (text.find("自杀") != std::string::npos || text.find("伤害") != std::string::npos) {
        result.riskScore += 0.5f;
    }
    
    result.riskScore = std::min(1.0f, result.riskScore);
    
    if (result.riskScore > 0.7f) {
        result.isValid = false;
        result.reason = "检测到高风险情感组合";
    }
    
    return Utils::Result<EmotionVerificationResult>::success(result);
}

// ==================== 阈值/缓存/状态 ====================

void DeepSeekModerationService::setThreshold(float threshold) {
    threshold_ = std::max(0.0f, std::min(1.0f, threshold));
}

void DeepSeekModerationService::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
}

void DeepSeekModerationService::clearExpiredCache(int maxAgeSeconds) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    
    std::vector<std::string> keysToRemove;
    for (const auto& [key, entry] : cache_) {
        int64_t ageSeconds = (now - entry.timestamp) / 1000000000;
        if (ageSeconds > maxAgeSeconds) {
            keysToRemove.push_back(key);
        }
    }
    
    for (const auto& key : keysToRemove) {
        cache_.erase(key);
    }
    
    LOG_INFO("缓存清理完成: 移除 {} 条过期记录", keysToRemove.size());
}

bool DeepSeekModerationService::isHealthy() const {
    return !endpoint_.empty();
}

// ==================== 统计 ====================

DeepSeekModerationService::ModerationStatistics DeepSeekModerationService::getStatistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return statistics_;
}

void DeepSeekModerationService::recordStatistic(const ModerationResponse& result) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    statistics_.totalModerations++;
    
    if (result.overallVerdict == "pass") {
        statistics_.passedCount++;
    } else if (result.overallVerdict == "review") {
        statistics_.reviewCount++;
    } else if (result.overallVerdict == "block") {
        statistics_.blockedCount++;
    }
}

// ==================== 私有方法 ====================

std::string DeepSeekModerationService::generateCacheKey(const std::string& content) const {
    return "mod_" + std::to_string(std::hash<std::string>{}(content));
}

ModerationResponse DeepSeekModerationService::callDeepSeekAPI(
    const std::string& text
) {
    LOG_DEBUG("调用 DeepSeek 审核 API");
    
    ModerationResponse response;
    
    // 如果没有 API Key，回退到基于规则的审核
    if (api_key_.empty()) {
        LOG_WARN("DeepSeek API Key 未配置，使用基于规则的审核");
        return ruleBasedModerate(text);
    }
    
    // 构造 DeepSeek Chat Completion 请求
    json requestBody = {
        {"model", "deepseek-chat"},
        {"messages", json::array({
            {{"role", "system"}, {"content", 
                "你是一个专业的内容审核AI。请分析以下文本并返回JSON格式的审核结果。\n"
                "返回格式（严格JSON，不要添加任何其他内容）：\n"
                "{\n"
                "  \"verdict\": \"pass|review|block\",\n"
                "  \"reason\": \"简短说明\",\n"
                "  \"categories\": [\n"
                "    {\"category\": \"类别名\", \"risk_score\": 0.0-1.0, \"verdict\": \"pass|review|block\", \"reason\": \"说明\"}\n"
                "  ]\n"
                "}\n"
                "可能的类别: violence, adult, harassment, spam, hate, profanity\n"
                "评分规则：0.0表示完全无风险，1.0表示严重违规。只列出评分>0的类别。"
            }},
            {{"role", "user"}, {"content", "请审核以下文本：\n" + text}}
        })},
        {"temperature", 0.0},
        {"max_tokens", 512}
    };
    std::string body = requestBody.dump();
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("CURL 初始化失败，回退到规则审核");
        return ruleBasedModerate(text);
    }
    
    std::string responseBody;
    std::string url = endpoint_;
    // 确保 URL 以 /v1/chat/completions 结尾
    if (url.back() == '/') url.pop_back();
    if (url.find("/v1/chat/completions") == std::string::npos) {
        if (url.find("/v1") != std::string::npos) {
            url = url.substr(0, url.find("/v1")) + "/v1/chat/completions";
        } else {
            url += "/v1/chat/completions";
        }
    }
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "Authorization: Bearer " + api_key_;
    headers = curl_slist_append(headers, authHeader.c_str());
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("DeepSeek 审核 HTTP 请求失败: {}，回退到规则审核", curl_easy_strerror(res));
        return ruleBasedModerate(text);
    }
    
    // 解析响应
    try {
        auto j = json::parse(responseBody);
        
        if (j.contains("error")) {
            LOG_ERROR("DeepSeek API 错误: {}", j["error"].value("message", "未知错误"));
            return ruleBasedModerate(text);
        }
        
        std::string content = j["choices"][0]["message"]["content"].get<std::string>();
        
        // 提取 JSON（去掉可能的 markdown 代码块标记）
        auto jsonStart = content.find('{');
        auto jsonEnd = content.rfind('}');
        if (jsonStart != std::string::npos && jsonEnd != std::string::npos) {
            content = content.substr(jsonStart, jsonEnd - jsonStart + 1);
        }
        
        auto moderationJson = json::parse(content);
        
        response.overallVerdict = moderationJson.value("verdict", "pass");
        response.recommendedAction = moderationJson.value("reason", "AI 审核完成");
        
        // 解析各类别的评分
        float maxScore = 0.0f;
        if (moderationJson.contains("categories") && moderationJson["categories"].is_array()) {
            for (const auto& cat : moderationJson["categories"]) {
                ModerationResult mr;
                mr.category = cat.value("category", "unknown");
                mr.riskScore = cat.value("risk_score", 0.0f);
                mr.verdict = cat.value("verdict", "pass");
                mr.reason = cat.value("reason", "");
                maxScore = std::max(maxScore, mr.riskScore);
                response.results.push_back(mr);
            }
        }
        
        response.overallRiskScore = maxScore;
        response.success = true;
        
        return response;
    } catch (const json::exception& e) {
        LOG_ERROR("DeepSeek 审核响应解析失败: {}，回退到规则审核", e.what());
        return ruleBasedModerate(text);
    }
}

ModerationResponse DeepSeekModerationService::ruleBasedModerate(
    const std::string& text
) {
    ModerationResponse response;
    response.success = true;
    
    // 敏感词规则（API 不可用时的回退方案）
    struct SensitiveRule {
        std::string keyword;
        std::string category;
        float score;
    };
    
    std::vector<SensitiveRule> rules = {
        {"杀", "violence", 0.4f},
        {"伤害", "violence", 0.3f},
        {"殴打", "violence", 0.5f},
        {"裸", "adult", 0.5f},
        {"色情", "adult", 0.6f},
        {"去死", "harassment", 0.4f},
        {"操", "profanity", 0.4f},
    };
    
    std::map<std::string, float> categoryScores;
    for (const auto& rule : rules) {
        if (text.find(rule.keyword) != std::string::npos) {
            categoryScores[rule.category] += rule.score;
        }
    }
    
    float maxScore = 0.0f;
    for (const auto& [cat, score] : categoryScores) {
        float clampedScore = std::min(1.0f, score);
        maxScore = std::max(maxScore, clampedScore);
        
        ModerationResult mr;
        mr.category = cat;
        mr.riskScore = clampedScore;
        mr.verdict = clampedScore >= 0.8f ? "block" : (clampedScore >= 0.5f ? "review" : "pass");
        mr.reason = "规则匹配";
        response.results.push_back(mr);
    }
    
    response.overallRiskScore = maxScore;
    
    if (maxScore >= 0.8f) {
        response.overallVerdict = "block";
        response.recommendedAction = "规则审核: 检测到不适当内容";
    } else if (maxScore >= 0.5f) {
        response.overallVerdict = "review";
        response.recommendedAction = "规则审核: 需要人工审核";
    } else {
        response.overallVerdict = "pass";
        response.recommendedAction = "规则审核: 内容通过";
    }
    
    return response;
}

} // namespace yachiyo::services