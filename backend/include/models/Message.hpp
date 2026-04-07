#pragma once

#include "BaseModel.hpp"
#include <string>
#include <ctime>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace yachiyo::models {

/**
 * @brief 审查状态枚举
 * - 0: 未审查
 * - 1: 通过
 * - 2: 拒绝
 * - 3: 待人工审查
 */
enum class ReviewStatus : int16_t {
    PENDING = 0,        // 未审查
    APPROVED = 1,       // 通过
    REJECTED = 2,       // 拒绝
    MANUAL_REVIEW = 3   // 待人工审查
};

/**
 * @brief 用户消息模型
 */
class Message : public BaseModel {
private:
    int64_t messageId = 0;
    int64_t userId = 0;
    std::string originalMessage;
    int32_t messageLength = 0;
    
    ReviewStatus reviewStatus = ReviewStatus::PENDING;
    std::string reviewReason;
    int64_t reviewedBy = 0;
    time_t reviewedAt = 0;
    
    // 恶意行为检测
    bool isSpam = false;
    bool isAbusive = false;
    bool isBlockedKeyword = false;
    double spamScore = 0.0;
    
    // 速率限制
    bool rateLimitViolated = false;
    
    // 元数据
    std::string userIp;
    std::string userAgent;
    time_t createdAt = 0;
    
public:
    Message() = default;
    
    // ==================== Getters ====================
    int64_t getMessageId() const { return messageId; }
    int64_t getUserId() const { return userId; }
    const std::string& getOriginalMessage() const { return originalMessage; }
    int32_t getMessageLength() const { return messageLength; }
    
    ReviewStatus getReviewStatus() const { return reviewStatus; }
    const std::string& getReviewReason() const { return reviewReason; }
    int64_t getReviewedBy() const { return reviewedBy; }
    time_t getReviewedAt() const { return reviewedAt; }
    
    bool getIsSpam() const { return isSpam; }
    bool getIsAbusive() const { return isAbusive; }
    bool getIsBlockedKeyword() const { return isBlockedKeyword; }
    double getSpamScore() const { return spamScore; }
    
    bool getRateLimitViolated() const { return rateLimitViolated; }
    
    const std::string& getUserIp() const { return userIp; }
    const std::string& getUserAgent() const { return userAgent; }
    time_t getCreatedAt() const { return createdAt; }
    
    // ==================== Setters ====================
    void setMessageId(int64_t id_) { messageId = id_; }
    void setUserId(int64_t userId_) { userId = userId_; }
    void setOriginalMessage(const std::string& msg) { originalMessage = msg; messageLength = msg.length(); }
    
    void setReviewStatus(ReviewStatus status) { reviewStatus = status; }
    void setReviewReason(const std::string& reason) { reviewReason = reason; }
    void setReviewedBy(int64_t userId_) { reviewedBy = userId_; }
    void setReviewedAt(time_t t) { reviewedAt = t; }
    
    void setIsSpam(bool spam) { isSpam = spam; }
    void setIsAbusive(bool abusive) { isAbusive = abusive; }
    void setIsBlockedKeyword(bool blocked) { isBlockedKeyword = blocked; }
    void setSpamScore(double score) { spamScore = score; }
    
    void setRateLimitViolated(bool violated) { rateLimitViolated = violated; }
    
    void setUserIp(const std::string& ip) { userIp = ip; }
    void setUserAgent(const std::string& agent) { userAgent = agent; }
    void setCreatedAt(time_t t) { createdAt = t; }
    
    // ==================== 辅助方法 ====================
    bool isPassed() const { return reviewStatus == ReviewStatus::APPROVED; }
    bool isRejected() const { return reviewStatus == ReviewStatus::REJECTED; }
    bool isPending() const { return reviewStatus == ReviewStatus::PENDING; }
    bool isHighRisk() const { return spamScore > 0.7 || isAbusive || isBlockedKeyword; }
    
    // ==================== JSON 序列化 ====================
    json toJson() const override {
        json j = BaseModel::toJson();
        
        std::string statusStr;
        switch (reviewStatus) {
            case ReviewStatus::APPROVED: statusStr = "approved"; break;
            case ReviewStatus::REJECTED: statusStr = "rejected"; break;
            case ReviewStatus::MANUAL_REVIEW: statusStr = "manual_review"; break;
            case ReviewStatus::PENDING: statusStr = "pending"; break;
        }
        
        j["messageId"] = messageId;
        j["userId"] = userId;
        j["originalMessage"] = originalMessage;
        j["messageLength"] = messageLength;
        j["reviewStatus"] = statusStr;
        j["reviewReason"] = reviewReason;
        j["isSpam"] = isSpam;
        j["isAbusive"] = isAbusive;
        j["isBlockedKeyword"] = isBlockedKeyword;
        j["spamScore"] = spamScore;
        j["rateLimitViolated"] = rateLimitViolated;
        j["createdAt"] = createdAt;
        
        return j;
    }
    
    // ==================== JSON 反序列化 ====================
    void fromJson(const json& j) override {
        BaseModel::fromJson(j);
        
        if (j.contains("messageId")) messageId = j["messageId"].get<int64_t>();
        if (j.contains("userId")) userId = j["userId"].get<int64_t>();
        if (j.contains("originalMessage")) originalMessage = j["originalMessage"].get<std::string>();
        if (j.contains("messageLength")) messageLength = j["messageLength"].get<int32_t>();
        
        std::string statusStr = j.value("reviewStatus", "pending");
        if (statusStr == "approved") reviewStatus = ReviewStatus::APPROVED;
        else if (statusStr == "rejected") reviewStatus = ReviewStatus::REJECTED;
        else if (statusStr == "manual_review") reviewStatus = ReviewStatus::MANUAL_REVIEW;
        else reviewStatus = ReviewStatus::PENDING;
        
        if (j.contains("reviewReason")) reviewReason = j["reviewReason"].get<std::string>();
        if (j.contains("isSpam")) isSpam = j["isSpam"].get<bool>();
        if (j.contains("isAbusive")) isAbusive = j["isAbusive"].get<bool>();
        if (j.contains("isBlockedKeyword")) isBlockedKeyword = j["isBlockedKeyword"].get<bool>();
        if (j.contains("spamScore")) spamScore = j["spamScore"].get<double>();
        if (j.contains("rateLimitViolated")) rateLimitViolated = j["rateLimitViolated"].get<bool>();
        if (j.contains("createdAt")) createdAt = j["createdAt"].get<time_t>();
    }
};

} // namespace yachiyo::models
