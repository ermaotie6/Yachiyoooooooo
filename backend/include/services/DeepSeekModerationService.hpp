#pragma once

#include <string>
#include <memory>
#include <vector>
#include "utils/Result.hpp"
#include "dto/ModerationDTO.hpp"

namespace yachiyo::services {

/**
 * DeepSeek 内容审查服务
 * 
 * 职责:
 * - 6层内容安全审查系统
 * - 审查类别: 暴力、成人、骚扰、垃圾、仇恨、脏话
 * - 感情验证: 确保情感与文本内容匹配
 * - 风险评分: 0.0-1.0 量化风险等级
 */
class DeepSeekModerationService {
public:
    enum class ModerationCategory {
        VIOLENCE,       // 暴力内容
        ADULT,          // 成人内容
        HARASSMENT,     // 骚扰内容
        SPAM,           // 垃圾信息
        HATE,           // 仇恨言论
        PROFANITY       // 脏话
    };
    
    enum class Verdict {
        PASS,           // 通过 (风险低)
        REVIEW,         // 需要审查 (风险中)
        BLOCK           // 拦截 (风险高)
    };
    
    DeepSeekModerationService();
    ~DeepSeekModerationService();
    
    /**
     * 初始化服务
     * @param endpoint DeepSeek API 端点
     * @param useOllama 是否使用本地 Ollama (true) 或云端 API (false)
     */
    bool initialize(
        const std::string& endpoint = "http://localhost:11434",
        bool useOllama = true
    );
    
    /**
     * 审查单条内容
     * @param request 审查请求
     * @return 审查结果 (风险评分、类别、建议动作)
     */
    Utils::Result<dto::ModerationResponse> moderate(
        const dto::ModerationRequest& request
    );
    
    /**
     * 批量审查
     * @param requests 请求列表
     * @return 结果列表
     */
    std::vector<Utils::Result<dto::ModerationResponse>> batchModerate(
        const std::vector<dto::ModerationRequest>& requests
    );
    
    /**
     * 验证情感是否与文本内容匹配
     * @param text 文本内容
     * @param emotions 声称的情感标签
     * @return 匹配的情感列表、不匹配的情感、可信度
     */
    struct EmotionVerificationResult {
        std::vector<std::string> matchedEmotions;
        std::vector<std::string> unmatchedEmotions;
        float confidence;
    };
    
    Utils::Result<EmotionVerificationResult> verifyEmotions(
        const std::string& text,
        const std::vector<std::string>& emotions
    );
    
    /**
     * 设置审查阈值
     * @param threshold 风险分数阈值 [0.0-1.0]
     *        < 0.3: PASS
     *        0.3-0.7: REVIEW
     *        > 0.7: BLOCK
     */
    void setThreshold(float threshold);
    
    /**
     * 清空缓存
     */
    void clearCache();
    
    /**
     * 获取服务状态
     */
    bool isHealthy() const;

private:
    struct CacheEntry {
        dto::ModerationResponse response;
        int64_t timestamp;
    };
    
    std::string endpoint_;
    bool use_ollama_;
    float threshold_;
    std::map<std::string, CacheEntry> cache_;
    std::mutex cache_mutex_;
    
    /**
     * 生成缓存键
     */
    std::string generateCacheKey(const std::string& content) const;
    
    /**
     * 调用 DeepSeek 进行审查
     */
    Utils::Result<json> callDeepSeek(const std::string& prompt);
    
    /**
     * 解析 DeepSeek 响应
     */
    Utils::Result<dto::ModerationResponse> parseDeepSeekResponse(
        const std::string& content,
        const json& deepseekResponse
    );
    
    /**
     * 计算风险分数
     */
    float calculateRiskScore(const json& categoriesData);
    
    /**
     * 根据风险分数确定判决
     */
    Verdict determineVerdict(float riskScore);
    
    /**
     * 生成推荐动作
     */
    std::string generateRecommendedAction(
        Verdict verdict,
        const std::vector<std::string>& riskCategories
    );
};

} // namespace yachiyo::services
