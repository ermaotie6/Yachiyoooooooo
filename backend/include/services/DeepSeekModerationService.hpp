#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include "utils/Result.hpp"
#include "utils/Compat.hpp"
#include "dto/ModerationDTO.hpp"

namespace yachiyo::services {

// 引入 DTO 类型
using dto::ModerationRequest;
using dto::ModerationResult;
using dto::ModerationResponse;

/**
 * DeepSeek 内容审查服务
 * 
 * 职责:
 * - 内容安全审查（调用 DeepSeek Chat API 或回退到规则审核）
 * - 审查类别: 暴力、成人、骚扰、垃圾、仇恨、脏话
 * - 情感验证: 确保情感与文本内容匹配
 * - 风险评分: 0.0-1.0 量化风险等级
 */
class DeepSeekModerationService {
public:
    DeepSeekModerationService();
    ~DeepSeekModerationService();
    
    /**
     * 初始化服务
     * @param apiKey DeepSeek API Key
     * @param endpoint DeepSeek API 端点（默认 https://api.deepseek.com）
     */
    bool initialize(
        const std::string& apiKey = "",
        const std::string& endpoint = ""
    );
    
    /**
     * 审查单条内容
     * @param request 审查请求
     * @return 审查响应
     */
    Utils::Result<ModerationResponse> moderate(
        const ModerationRequest& request
    );
    
    /**
     * 批量审查
     * @param requests 请求列表
     * @return 结果列表
     */
    std::vector<Utils::Result<ModerationResponse>> batchModerate(
        const std::vector<ModerationRequest>& requests
    );
    
    /**
     * 验证情感是否与文本内容匹配
     * @param text 文本内容
     * @param emotions 声称的情感标签
     */
    struct EmotionVerificationResult {
        bool isValid = true;
        std::vector<std::string> matchedEmotions;
        std::vector<std::string> unmatchedEmotions;
        float confidence = 1.0f;
        float riskScore = 0.0f;
        std::string reason;
    };
    
    Utils::Result<EmotionVerificationResult> verifyEmotions(
        const std::string& text,
        const std::vector<std::string>& emotions
    );
    
    /**
     * 设置审查阈值
     */
    void setThreshold(float threshold);
    
    /**
     * 清空缓存
     */
    void clearCache();
    
    /**
     * 清理过期缓存
     */
    void clearExpiredCache(int maxAgeSeconds = 3600);
    
    /**
     * 获取服务状态
     */
    bool isHealthy() const;
    
    /**
     * 获取审核统计
     */
    struct ModerationStatistics {
        int64_t totalModerations = 0;
        int64_t passedCount = 0;
        int64_t reviewCount = 0;
        int64_t blockedCount = 0;
    };
    ModerationStatistics getStatistics();

private:
    struct CacheEntry {
        ModerationResponse response;
        int64_t timestamp;
    };
    
    std::string api_key_;
    std::string endpoint_;
    float threshold_ = 0.5f;
    bool enable_emotion_verification_ = true;
    
    std::map<std::string, CacheEntry> cache_;
    std::mutex cache_mutex_;
    
    ModerationStatistics statistics_;
    std::mutex stats_mutex_;
    
    /**
     * 生成缓存键
     */
    std::string generateCacheKey(const std::string& content) const;
    
    /**
     * 调用 DeepSeek Chat Completion API 进行内容审核
     */
    ModerationResponse callDeepSeekAPI(const std::string& text);
    
    /**
     * 基于规则的回退审核（当 API 不可用时）
     */
    ModerationResponse ruleBasedModerate(const std::string& text);
    
    /**
     * 记录审核统计
     */
    void recordStatistic(const ModerationResponse& result);
};

} // namespace yachiyo::services
