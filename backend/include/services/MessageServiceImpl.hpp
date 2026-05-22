#pragma once

#include "MessageService.hpp"
#include "AuthService.hpp"
#include "../utils/RedisUtil.hpp"
#include "../utils/DatabaseUtil.hpp"
#include <memory>

namespace yachiyo::services {

/**
 * @brief 消息服务实现 (5层规则审查, AI审查由 OpenClaw 统一处理)
 */
class MessageServiceImpl : public IMessageService {
private:
    std::shared_ptr<Yachiyo::Utils::DatabaseUtil> dbUtil;
    std::shared_ptr<IAuthService> authService;

    static constexpr int MAX_MESSAGES_PER_MINUTE = 10;
    static constexpr int MAX_MESSAGES_PER_HOUR = 100;
    static constexpr double SPAM_SCORE_THRESHOLD = 0.6;

public:
    MessageServiceImpl(
        std::shared_ptr<Yachiyo::Utils::DatabaseUtil> db = nullptr,
        std::shared_ptr<IAuthService> auth = nullptr,
        std::shared_ptr<void> /* unused - kept for backward compat */ = nullptr
    ) : dbUtil(db), authService(auth) {}

    Result<std::shared_ptr<Message>> sendMessage(
        int64_t userId, const std::string& message,
        const std::string& userIp, const std::string& userAgent = "") override;

    Result<bool> reviewMessage(
        int64_t messageId, int64_t reviewerId,
        bool approved, const std::string& reason = "") override;

    Result<std::vector<std::shared_ptr<Message>>> getUserMessages(
        int64_t userId, int32_t limit = 20, int32_t offset = 0) override;

    Result<std::vector<std::shared_ptr<Message>>> getPendingMessages(
        int32_t limit = 50, int32_t offset = 0) override;

    Result<std::vector<std::shared_ptr<Message>>> getHighRiskMessages(
        int32_t limit = 50, int32_t offset = 0) override;

    Result<json> getStatistics() override;

    Result<bool> deleteMessage(
        int64_t messageId, int64_t userId, bool isAdmin,
        const std::string& reason = "") override;

    Result<bool> hideMessage(
        int64_t messageId, const std::string& reason = "") override;

private:
    Result<bool> checkRateLimit(int64_t userId, const std::string& userIp);
    Result<bool> checkIpBlacklist(const std::string& userIp);
    Result<std::pair<bool, double>> checkBlockedKeywords(const std::string& message);
    Result<bool> behaviorAnalysis(int64_t userId, const std::string& userIp);
    bool needsManualReview(const std::shared_ptr<Message>& message);
    double calculateSpamScore(const std::shared_ptr<Message>& message);
    bool isAbusiveContent(const std::string& message);
};

} // namespace yachiyo::services
