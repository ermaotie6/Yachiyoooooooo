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
    : endpoint_("https://api.deepseek.com/v1/moderation"),
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
    
    // 初始化类别阈值
    initializeCategoryThresholds();
    
    LOG_INFO("DeepSeek 内容审核服务初始化完成");
    return true;
}

// ==================== 内容审核 ====================

Utils::Result<dto::ModerationResult> DeepSeekModerationService::moderate(
    const std::string& text
) {
    LOG_DEBUG("审核文本内容");
    
    // 1. 检查缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = moderation_cache_.find(text);
        
        if (it != moderation_cache_.end()) {
            auto ageSeconds = (std::chrono::system_clock::now().time_since_epoch().count() - 
                             it->second.timestamp) / 1000000000;
            if (ageSeconds < 3600) {  // 1小时 TTL
                LOG_DEBUG("返回缓存审核结果");
                return Utils::Result<dto::ModerationResult>::success(it->second.result);
            }
        }
    }
    
    // 2. 调用 DeepSeek API
    dto::ModerationResult result = callDeepSeekAPI(text);
    
    // 3. 缓存结果
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        CacheEntry entry;
        entry.result = result;
        entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        moderation_cache_[text] = entry;
    }
    
    return Utils::Result<dto::ModerationResult>::success(result);
}

// ==================== 批量审核 ====================

Utils::Result<std::vector<dto::ModerationResult>> DeepSeekModerationService::batchModerate(
    const std::vector<std::string>& texts
) {
    LOG_INFO("批量审核: 共 {} 条文本", texts.size());
    
    std::vector<dto::ModerationResult> results;
    
    for (const auto& text : texts) {
        auto result = moderate(text);
        if (result.isSuccess()) {
            results.push_back(result.getValue());
        } else {
            LOG_WARN("审核失败: {}", result.getError().message);
        }
    }
    
    LOG_INFO("批量审核完成: 成功 {} 条", results.size());
    return Utils::Result<std::vector<dto::ModerationResult>>::success(results);
}

// ==================== 情感验证 ====================

Utils::Result<dto::EmotionVerificationResult> DeepSeekModerationService::verifyEmotions(
    const std::vector<std::string>& emotions,
    const std::string& context
) {
    LOG_DEBUG("验证情感: {} 个情感在上下文: {}", emotions.size(), context);
    
    dto::EmotionVerificationResult result;
    result.isValid = true;
    result.riskScore = 0.0f;
    
    // 定义不适当的情感组合规则
    std::vector<std::string> risky_emotions = {"愤怒", "仇恨", "绝望"};
    
    for (const auto& emotion : emotions) {
        // 检查是否为高风险情感
        for (const auto& risky : risky_emotions) {
            if (emotion.find(risky) != std::string::npos) {
                result.riskScore += 0.3f;
            }
        }
    }
    
    // 如果文本包含危险关键词，增加风险分数
    if (context.find("自杀") != std::string::npos ||
        context.find("伤害") != std::string::npos) {
        result.riskScore += 0.5f;
    }
    
    // 规范化风险分数
    result.riskScore = std::min(1.0f, result.riskScore);
    
    // 如果风险分数超过阈值，标记为无效
    if (result.riskScore > 0.7f) {
        result.isValid = false;
        result.reason = "检测到高风险情感组合";
    }
    
    return Utils::Result<dto::EmotionVerificationResult>::success(result);
}

// ==================== 私有方法 ====================

dto::ModerationResult DeepSeekModerationService::callDeepSeekAPI(
    const std::string& text
) {
    LOG_DEBUG("调用 DeepSeek 审核 API");
    
    dto::ModerationResult result;
    result.text = text;
    result.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // 模拟 API 调用（实际部署时应使用真实 API）
    // 在此示例中进行基于规则的审核
    
    // 定义敏感词
    std::map<std::string, dto::ModerationCategory> sensitiveWords = {
        // 暴力相关
        {"杀", dto::ModerationCategory::VIOLENCE},
        {"打", dto::ModerationCategory::VIOLENCE},
        {"伤害", dto::ModerationCategory::VIOLENCE},
        
        // 成人相关
        {"裸", dto::ModerationCategory::ADULT},
        {"色", dto::ModerationCategory::ADULT},
        
        // 骚扰相关
        {"死", dto::ModerationCategory::HARASSMENT},
        {"滚", dto::ModerationCategory::HARASSMENT},
        
        // 垃圾内容
        {"点赞", dto::ModerationCategory::SPAM},
        {"转发", dto::ModerationCategory::SPAM},
        
        // 仇恨言论
        {"可恶", dto::ModerationCategory::HATE},
        {"讨厌", dto::ModerationCategory::HATE},
        
        // 粗言秽语
        {"操", dto::ModerationCategory::PROFANITY},
        {"妈", dto::ModerationCategory::PROFANITY},
    };
    
    // 扫描文本
    std::map<dto::ModerationCategory, float> categoryScores;
    for (const auto& [word, category] : sensitiveWords) {
        if (text.find(word) != std::string::npos) {
            categoryScores[category] += 0.2f;
        }
    }
    
    // 为每个类别设置结果
    for (const auto& [category, score] : categoryScores) {
        result.categories[category] = std::min(1.0f, score);
    }
    
    // 确定最终判决
    float maxScore = 0.0f;
    for (const auto& [category, score] : result.categories) {
        maxScore = std::max(maxScore, score);
    }
    
    if (maxScore >= category_thresholds_[dto::ModerationCategory::VIOLENCE]) {
        result.verdict = dto::Verdict::BLOCK;
        result.reason = "检测到不适当内容";
    } else if (maxScore >= 0.5f) {
        result.verdict = dto::Verdict::REVIEW;
        result.reason = "需要人工审核";
    } else {
        result.verdict = dto::Verdict::PASS;
        result.reason = "内容通过审核";
    }
    
    return result;
}

void DeepSeekModerationService::initializeCategoryThresholds() {
    LOG_DEBUG("初始化审核类别阈值");
    
    // 设置每个类别的阈值
    category_thresholds_[dto::ModerationCategory::VIOLENCE] = 0.8f;
    category_thresholds_[dto::ModerationCategory::ADULT] = 0.9f;
    category_thresholds_[dto::ModerationCategory::HARASSMENT] = 0.7f;
    category_thresholds_[dto::ModerationCategory::SPAM] = 0.6f;
    category_thresholds_[dto::ModerationCategory::HATE] = 0.7f;
    category_thresholds_[dto::ModerationCategory::PROFANITY] = 0.8f;
}

bool DeepSeekModerationService::validateResponse(const json& response) {
    LOG_DEBUG("验证 API 响应格式");
    
    // 检查必要字段
    if (!response.contains("categories") || !response.contains("verdict")) {
        LOG_ERROR("响应格式无效");
        return false;
    }
    
    return true;
}

std::string DeepSeekModerationService::categorizeRiskLevel(float score) {
    if (score >= 0.8f) return "高风险";
    if (score >= 0.5f) return "中风险";
    if (score >= 0.2f) return "低风险";
    return "无风险";
}

// ==================== 统计方法 ====================

ModerationStatistics DeepSeekModerationService::getStatistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return statistics_;
}

void DeepSeekModerationService::recordStatistic(
    const dto::ModerationResult& result
) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    statistics_.totalModerations++;
    
    if (result.verdict == dto::Verdict::PASS) {
        statistics_.passedCount++;
    } else if (result.verdict == dto::Verdict::REVIEW) {
        statistics_.reviewCount++;
    } else if (result.verdict == dto::Verdict::BLOCK) {
        statistics_.blockedCount++;
    }
    
    // 统计每个类别
    for (const auto& [category, score] : result.categories) {
        if (score > category_thresholds_[category]) {
            statistics_.categoryViolations[category]++;
        }
    }
}

// ==================== 缓存清理 ====================

void DeepSeekModerationService::clearExpiredCache(int maxAgeSeconds) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    
    std::vector<std::string> keysToRemove;
    for (const auto& entry : moderation_cache_) {
        int ageSeconds = (now - entry.second.timestamp) / 1000000000;
        if (ageSeconds > maxAgeSeconds) {
            keysToRemove.push_back(entry.first);
        }
    }
    
    for (const auto& key : keysToRemove) {
        moderation_cache_.erase(key);
    }
    
    LOG_INFO("缓存清理完成: 移除 {} 条过期记录", keysToRemove.size());
}

} // namespace yachiyo::services
