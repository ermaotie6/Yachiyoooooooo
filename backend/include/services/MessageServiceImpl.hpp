#pragma once

#include "MessageService.hpp"
#include "../utils/RedisUtil.hpp"
#include "../utils/DatabaseUtil.hpp"
#include <memory>

namespace yachiyo::services {

// 前向声明
class DeepSeekModerationService;

/**
 * @brief 消息服务实现 (6层安全审查系统)
 */
class MessageServiceImpl : public IMessageService {
private:
    std::shared_ptr<Yachiyo::Utils::DatabaseUtil> dbUtil;
    std::shared_ptr<IAuthService> authService;
    std::shared_ptr<DeepSeekModerationService> moderationService;
    
    // 配置参数
    static constexpr int MAX_MESSAGES_PER_MINUTE = 10;      // 每分钟最多消息数
    static constexpr int MAX_MESSAGES_PER_HOUR = 100;       // 每小时最多消息数
    static constexpr double AI_REVIEW_THRESHOLD = 0.7;      // AI审查阈值
    static constexpr double SPAM_SCORE_THRESHOLD = 0.6;    // 垃圾邮件阈值
    
public:
    MessageServiceImpl(
        std::shared_ptr<Yachiyo::Utils::DatabaseUtil> db = nullptr,
        std::shared_ptr<IAuthService> auth = nullptr,
        std::shared_ptr<DeepSeekModerationService> moderation = nullptr
    ) : dbUtil(db), authService(auth), moderationService(moderation) {}
    
    // ==================== 实现接口 ====================
    
    Result<std::shared_ptr<Message>> sendMessage(
        int64_t userId,
        const std::string& message,
        const std::string& userIp,
        const std::string& userAgent = ""
    ) override;
    
    Result<bool> reviewMessage(
        int64_t messageId,
        int64_t reviewerId,
        bool approved,
        const std::string& reason = ""
    ) override;
    
    Result<std::vector<std::shared_ptr<Message>>> getUserMessages(
        int64_t userId,
        int32_t limit = 20,
        int32_t offset = 0
    ) override;
    
    Result<std::vector<std::shared_ptr<Message>>> getPendingMessages(
        int32_t limit = 50,
        int32_t offset = 0
    ) override;
    
    Result<std::vector<std::shared_ptr<Message>>> getHighRiskMessages(
        int32_t limit = 50,
        int32_t offset = 0
    ) override;
    
    Result<json> getStatistics() override;
    
    Result<bool> deleteMessage(
        int64_t messageId,
        int64_t userId,
        bool isAdmin,
        const std::string& reason = ""
    ) override;

    Result<bool> hideMessage(
        int64_t messageId,
        const std::string& reason = ""
    ) override;

private:
    // ==================== 6层安全审查 ====================
    
    /**
     * @brief 第1层: 速率限制检查
     */
    Result<bool> checkRateLimit(int64_t userId, const std::string& userIp);
    
    /**
     * @brief 第2层: IP黑名单检查
     */
    Result<bool> checkIpBlacklist(const std::string& userIp);
    
    /**
     * @brief 第3层: 敏感词检查
     */
    Result<std::pair<bool, double>> checkBlockedKeywords(const std::string& message);
    
    /**
     * @brief 第4层: AI内容审查 (调用openclaw)
     */
    Result<std::pair<bool, double>> aiContentReview(const std::string& message);
    
    /**
     * @brief 第5层: 行为分析
     */
    Result<bool> behaviorAnalysis(int64_t userId, const std::string& userIp);
    
    /**
     * @brief 第6层: 需要人工审查标记
     */
    bool needsManualReview(const std::shared_ptr<Message>& message);
    
    // 辅助方法
    double calculateSpamScore(const std::shared_ptr<Message>& message);
    bool isAbusiveContent(const std::string& message);
};

} // namespace yachiyo::services
