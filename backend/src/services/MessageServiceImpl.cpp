#include "services/MessageServiceImpl.hpp"
#include "services/DeepSeekModerationService.hpp"
#include "utils/LogUtils.hpp"
#include "utils/RedisUtil.hpp"
#include <ctime>
#include <algorithm>
#include <cctype>

namespace yachiyo::services {

// 静态日志器
static auto logger = yachiyo::utils::LogUtils::getLogger("MessageServiceImpl");

/**
 * @brief 计算 UTF-8 字符串的 Unicode 字符数（与前端 Array.from(str).length 口径一致）
 * 
 * 正确处理 ASCII、多字节中文、emoji 等。
 */
static size_t utf8CharCount(const std::string& s) {
    size_t count = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            i += 1;           // 0xxxxxxx — ASCII
        } else if ((c >> 5) == 0x06) {
            i += 2;           // 110xxxxx — 2字节
        } else if ((c >> 4) == 0x0E) {
            i += 3;           // 1110xxxx — 3字节 (中文等 BMP 字符)
        } else if ((c >> 3) == 0x1E) {
            i += 4;           // 11110xxx — 4字节 (emoji 等 supplementary)
        } else {
            i += 1;           // 无效字节，跳过
        }
        ++count;
    }
    return count;
}

// ==================== 发送消息 (执行6层审查) ====================

Result<std::shared_ptr<Message>> MessageServiceImpl::sendMessage(
    int64_t userId,
    const std::string& message,
    const std::string& userIp,
    const std::string& userAgent
) {
    try {
        // 验证用户是否可以发送消息
        auto canSendResult = authService->canUserSendMessage(userId);
        if (!canSendResult.isSuccess()) {
            return Result<std::shared_ptr<Message>>::Error(canSendResult.getErrorMsg());
        }
        
        // 验证消息内容（使用 UTF-8 字符计数，与前端 Array.from(str).length 一致）
        size_t charLen = utf8CharCount(message);
        if (message.empty() || charLen > 50) {
            return Result<std::shared_ptr<Message>>::Error("消息长度必须1-50字符");
        }
        
        auto msg = std::make_shared<Message>();
        msg->setUserId(userId);
        msg->setOriginalMessage(message);
        msg->setUserIp(userIp);
        msg->setUserAgent(userAgent);
        msg->setCreatedAt(std::time(nullptr));
        
        // ==================== 6层安全审查 ====================
        
        // 跟踪是否已被拒绝 (早期拒绝不被后续层覆盖)
        bool alreadyRejected = false;
        
        // 第1层: 速率限制
        auto rateLimitResult = checkRateLimit(userId, userIp);
        if (!rateLimitResult.isSuccess()) {
            msg->setRateLimitViolated(true);
            msg->setReviewStatus(ReviewStatus::REJECTED);
            msg->setReviewReason("触发速率限制");
            alreadyRejected = true;
        }
        
        // 第2层: IP黑名单
        auto ipCheckResult = checkIpBlacklist(userIp);
        if (!ipCheckResult.isSuccess()) {
            msg->setReviewStatus(ReviewStatus::REJECTED);
            msg->setReviewReason("来自黑名单IP");
            alreadyRejected = true;
        }
        
        // 第3层: 敏感词检查 (仅在未被拒绝时才考虑降级为人工审查)
        auto keywordResult = checkBlockedKeywords(message);
        if (keywordResult.isSuccess()) {
            auto [foundKeyword, keywordScore] = keywordResult.value();
            if (foundKeyword) {
                msg->setIsBlockedKeyword(true);
                msg->setSpamScore(keywordScore);
                if (!alreadyRejected) {
                    msg->setReviewStatus(ReviewStatus::MANUAL_REVIEW);
                    msg->setReviewReason("包含敏感词");
                }
            }
        }
        
        // 第4层: AI内容审查
        auto aiResult = aiContentReview(message);
        if (aiResult.isSuccess()) {
            auto [isAbusive, aiScore] = aiResult.value();
            if (isAbusive) {
                msg->setIsAbusive(true);
                msg->setSpamScore(std::max(msg->getSpamScore(), aiScore));
                if (!alreadyRejected) {
                    if (aiScore > 0.9) {
                        msg->setReviewStatus(ReviewStatus::REJECTED);
                        msg->setReviewReason("AI审查不通过");
                        alreadyRejected = true;
                    } else {
                        msg->setReviewStatus(ReviewStatus::MANUAL_REVIEW);
                    }
                }
            }
        }
        
        // 第5层: 行为分析
        auto behaviorResult = behaviorAnalysis(userId, userIp);
        if (!behaviorResult.isSuccess()) {
            msg->setIsSpam(true);
            if (!alreadyRejected) {
                msg->setReviewStatus(ReviewStatus::MANUAL_REVIEW);
                msg->setReviewReason("异常行为检测");
            }
        }
        
        // 如果没有被标记为拒绝或人工审查，默认通过
        if (!alreadyRejected && msg->getReviewStatus() == ReviewStatus::PENDING) {
            msg->setReviewStatus(ReviewStatus::APPROVED);
        }
        
        // 第6层: 检查是否需要人工审查 (仅在未被拒绝时)
        if (!alreadyRejected && needsManualReview(msg)) {
            msg->setReviewStatus(ReviewStatus::MANUAL_REVIEW);
        }
        
        // 保存到数据库
        try {
            auto result = dbUtil->insert(
                "INSERT INTO user_messages (user_id, original_message, message_length, "
                "review_status, is_spam, is_abusive, is_blocked_keyword, spam_score, "
                "rate_limit_violated, user_ip, user_agent, created_at) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, NOW()) "
                "RETURNING id",
                {
                    std::to_string(userId),
                    message,
                    std::to_string(charLen),
                    std::to_string(static_cast<int>(msg->getReviewStatus())),
                    msg->getIsSpam() ? "true" : "false",
                    msg->getIsAbusive() ? "true" : "false",
                    msg->getIsBlockedKeyword() ? "true" : "false",
                    std::to_string(msg->getSpamScore()),
                    msg->getRateLimitViolated() ? "true" : "false",
                    userIp,
                    userAgent
                }
            );
            
            if (!result.empty()) {
                msg->setMessageId(std::stoll(result[0]["id"]));
            }
        } catch (const std::exception& e) {
            logger->error("保存消息失败: {}", e.what());
            return Result<std::shared_ptr<Message>>::Error("保存消息失败");
        }
        
        logger->info("消息创建成功: {}", msg->getMessageId());
        return Result<std::shared_ptr<Message>>::Success(msg);
        
    } catch (const std::exception& e) {
        logger->error("发送消息异常: {}", e.what());
        return Result<std::shared_ptr<Message>>::Error("发送失败，请稍后重试");
    }
}

// ==================== 审查消息 ====================

Result<bool> MessageServiceImpl::reviewMessage(
    int64_t messageId,
    int64_t reviewerId,
    bool approved,
    const std::string& reason
) {
    try {
        ReviewStatus newStatus = approved ? ReviewStatus::APPROVED : ReviewStatus::REJECTED;
        
        dbUtil->execute(
            "UPDATE user_messages SET review_status = $1, reviewed_by = $2, "
            "reviewed_at = NOW(), review_reason = $3 WHERE id = $4",
            {
                std::to_string(static_cast<int>(newStatus)),
                std::to_string(reviewerId),
                reason,
                std::to_string(messageId)
            }
        );
        
        logger->info("消息审查完成: {}", messageId);
        return Result<bool>::Success(true);
        
    } catch (const std::exception& e) {
        logger->error("审查消息异常: {}", e.what());
        return Result<bool>::Error("审查失败");
    }
}

// ==================== 获取用户消息列表 ====================

Result<std::vector<std::shared_ptr<Message>>> MessageServiceImpl::getUserMessages(
    int64_t userId,
    int32_t limit,
    int32_t offset
) {
    try {
        std::vector<std::shared_ptr<Message>> messages;
        
        auto result = dbUtil->query(
            "SELECT id, user_id, original_message, message_length, review_status, "
            "is_spam, is_abusive, is_blocked_keyword, spam_score, created_at "
            "FROM user_messages WHERE user_id = $1 AND review_status = $2 "
            "ORDER BY created_at DESC LIMIT $3 OFFSET $4",
            {
                std::to_string(userId),
                std::to_string(static_cast<int>(ReviewStatus::APPROVED)),
                std::to_string(limit),
                std::to_string(offset)
            }
        );
        
        for (const auto& row : result) {
            auto msg = std::make_shared<Message>();
            msg->setMessageId(std::stoll(row.at("id")));
            msg->setUserId(std::stoll(row.at("user_id")));
            msg->setOriginalMessage(row.at("original_message"));
            msg->setReviewStatus(static_cast<ReviewStatus>(std::stoi(row.at("review_status"))));
            messages.push_back(msg);
        }
        
        return Result<std::vector<std::shared_ptr<Message>>>::Success(messages);
        
    } catch (const std::exception& e) {
        logger->error("获取用户消息异常: {}", e.what());
        return Result<std::vector<std::shared_ptr<Message>>>::Error("获取失败");
    }
}

// ==================== 获取待审查消息 ====================

Result<std::vector<std::shared_ptr<Message>>> MessageServiceImpl::getPendingMessages(
    int32_t limit,
    int32_t offset
) {
    try {
        std::vector<std::shared_ptr<Message>> messages;
        
        auto result = dbUtil->query(
            "SELECT id, user_id, original_message, message_length, review_status, "
            "is_spam, is_abusive, is_blocked_keyword, spam_score, created_at "
            "FROM user_messages WHERE review_status = $1 "
            "ORDER BY created_at ASC LIMIT $2 OFFSET $3",
            {
                std::to_string(static_cast<int>(ReviewStatus::MANUAL_REVIEW)),
                std::to_string(limit),
                std::to_string(offset)
            }
        );
        
        for (const auto& row : result) {
            auto msg = std::make_shared<Message>();
            msg->setMessageId(std::stoll(row.at("id")));
            msg->setUserId(std::stoll(row.at("user_id")));
            msg->setOriginalMessage(row.at("original_message"));
            messages.push_back(msg);
        }
        
        return Result<std::vector<std::shared_ptr<Message>>>::Success(messages);
        
    } catch (const std::exception& e) {
        logger->error("获取待审查消息异常: {}", e.what());
        return Result<std::vector<std::shared_ptr<Message>>>::Error("获取失败");
    }
}

// ==================== 获取高风险消息 ====================

Result<std::vector<std::shared_ptr<Message>>> MessageServiceImpl::getHighRiskMessages(
    int32_t limit,
    int32_t offset
) {
    try {
        std::vector<std::shared_ptr<Message>> messages;
        
        auto result = dbUtil->query(
            "SELECT id, user_id, original_message, message_length, is_spam, "
            "is_abusive, is_blocked_keyword, spam_score, created_at "
            "FROM user_messages WHERE spam_score > $1 OR is_abusive = true "
            "ORDER BY spam_score DESC LIMIT $2 OFFSET $3",
            {
                std::to_string(SPAM_SCORE_THRESHOLD),
                std::to_string(limit),
                std::to_string(offset)
            }
        );
        
        for (const auto& row : result) {
            auto msg = std::make_shared<Message>();
            msg->setMessageId(std::stoll(row.at("id")));
            msg->setUserId(std::stoll(row.at("user_id")));
            msg->setOriginalMessage(row.at("original_message"));
            messages.push_back(msg);
        }
        
        return Result<std::vector<std::shared_ptr<Message>>>::Success(messages);
        
    } catch (const std::exception& e) {
        logger->error("获取高风险消息异常: {}", e.what());
        return Result<std::vector<std::shared_ptr<Message>>>::Error("获取失败");
    }
}

// ==================== 获取统计信息 ====================

Result<json> MessageServiceImpl::getStatistics() {
    try {
        auto result = dbUtil->query(
            "SELECT "
            "COUNT(*) as total_messages, "
            "SUM(CASE WHEN review_status = 1 THEN 1 ELSE 0 END) as approved_count, "
            "SUM(CASE WHEN review_status = 2 THEN 1 ELSE 0 END) as rejected_count, "
            "SUM(CASE WHEN review_status = 3 THEN 1 ELSE 0 END) as pending_count, "
            "SUM(CASE WHEN is_spam = true THEN 1 ELSE 0 END) as spam_count "
            "FROM user_messages",
            {}
        );
        
        json stats;
        if (!result.empty()) {
            auto row = result[0];
            stats["total_messages"] = std::stoll(row.at("total_messages"));
            stats["approved_count"] = std::stoll(row.at("approved_count"));
            stats["rejected_count"] = std::stoll(row.at("rejected_count"));
            stats["pending_count"] = std::stoll(row.at("pending_count"));
            stats["spam_count"] = std::stoll(row.at("spam_count"));
        }
        
        return Result<json>::Success(stats);
        
    } catch (const std::exception& e) {
        logger->error("获取统计信息异常: {}", e.what());
        return Result<json>::Error("获取失败");
    }
}

// ==================== 删除消息 ====================

Result<bool> MessageServiceImpl::deleteMessage(
    int64_t messageId,
    int64_t userId,
    bool isAdmin,
    const std::string& reason
) {
    try {
        // 非管理员只能删除自己的消息
        if (!isAdmin) {
            auto result = dbUtil->query(
                "SELECT user_id FROM user_messages WHERE id = $1",
                {std::to_string(messageId)}
            );
            if (result.empty()) {
                return Result<bool>::Error("消息不存在");
            }
            int64_t ownerId = std::stoll(result[0].at("user_id"));
            if (ownerId != userId) {
                return Result<bool>::Error("权限不足，只能删除自己的消息");
            }
        }

        dbUtil->execute(
            "DELETE FROM user_messages WHERE id = $1",
            {std::to_string(messageId)}
        );

        logger->info("消息已删除: messageId={}, by userId={}, isAdmin={}, reason={}",
                    messageId, userId, isAdmin, reason);
        return Result<bool>::Success(true);

    } catch (const std::exception& e) {
        logger->error("删除消息异常: {}", e.what());
        return Result<bool>::Error("删除失败");
    }
}

// ==================== 隐藏消息 ====================

Result<bool> MessageServiceImpl::hideMessage(
    int64_t messageId,
    const std::string& reason
) {
    try {
        dbUtil->execute(
            "UPDATE user_messages SET review_status = $1, review_reason = $2 WHERE id = $3",
            {
                std::to_string(static_cast<int>(ReviewStatus::REJECTED)),
                reason,
                std::to_string(messageId)
            }
        );
        
        return Result<bool>::Success(true);
        
    } catch (const std::exception& e) {
        logger->error("隐藏消息异常: {}", e.what());
        return Result<bool>::Error("操作失败");
    }
}

// ==================== 辅助方法 (简化实现) ====================

Result<bool> MessageServiceImpl::checkRateLimit(int64_t userId, const std::string& userIp) {
    try {
        using RedisUtil = Yachiyo::Utils::RedisUtil;
        std::string key = "rate_limit:" + std::to_string(userId);
        // 使用 Redis INCR 原子命令实现计数器，避免 get+set 的竞态条件
        auto conn = RedisUtil::getConnection();
        if (!conn) {
            // Redis 不可用时放行
            return Result<bool>::Success(true);
        }
        // INCR 是原子操作，返回递增后的值；如果 key 不存在则初始化为 0 再 +1
        std::string countStr = conn->executeCommand("INCR", {key});
        int count = 0;
        try { count = std::stoi(countStr); } catch (...) { count = 1; }
        // 首次创建 key 时设置 TTL（仅当 count == 1 时设置，避免每次重置 TTL）
        if (count == 1) {
            conn->expire(key, 60); // 60秒窗口
        }
        RedisUtil::releaseConnection(conn);
        if (count > MAX_MESSAGES_PER_MINUTE) {
            return Result<bool>::Error("消息过于频繁，请稍后再试");
        }
        return Result<bool>::Success(true);
    } catch (...) {
        // Redis不可用时放行
        return Result<bool>::Success(true);
    }
}

Result<bool> MessageServiceImpl::checkIpBlacklist(const std::string& userIp) {
    try {
        auto result = dbUtil->query(
            "SELECT 1 FROM user_blacklist WHERE identifier = $1 AND identifier_type = 1 "
            "AND (expires_at IS NULL OR expires_at > NOW())",
            {userIp}
        );
        if (!result.empty()) {
            return Result<bool>::Error("该IP已被禁用");
        }
        return Result<bool>::Success(true);
    } catch (...) {
        return Result<bool>::Success(true);
    }
}

Result<std::pair<bool, double>> MessageServiceImpl::checkBlockedKeywords(const std::string& message) {
    try {
        // 转换消息为小写以进行不区分大小写的匹配 (仅对 ASCII 字符有效，中文无大小写)
        std::string lowerMessage = message;
        std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);
        
        // 查询数据库中的敏感词 (使用 LOWER() 确保关键词也被转小写，与 lowerMessage 一致)
        auto result = dbUtil->query(
            "SELECT LOWER(keyword) AS keyword, severity FROM blocked_keywords WHERE is_active = true",
            {}
        );
        
        double maxScore = 0.0;
        bool foundKeyword = false;
        
        for (const auto& row : result) {
            std::string keyword = row.at("keyword");
            int severity = std::stoi(row.at("severity"));
            
            // 关键词匹配 (消息和关键词均已转小写，英文大小写不敏感)
            if (lowerMessage.find(keyword) != std::string::npos) {
                foundKeyword = true;
                // 根据严重程度计算评分 (1=低→0.3, 2=中→0.6, 3=高→0.9)
                double score = severity * 0.3;
                maxScore = std::max(maxScore, score);
            }
        }
        
        return Result<std::pair<bool, double>>::Success(std::make_pair(foundKeyword, maxScore));
        
    } catch (const std::exception& e) {
        logger->error("敏感词检查异常: {}", e.what());
        return Result<std::pair<bool, double>>::Success(std::make_pair(false, 0.0));
    }
}

Result<std::pair<bool, double>> MessageServiceImpl::aiContentReview(const std::string& message) {
    try {
        // Layer 4: AI 内容审查
        // 优先调用 DeepSeekModerationService (调用 DeepSeek Chat API)，
        // 若未配置或调用失败则回退到启发式审查。
        
        if (moderationService) {
            // 使用 DeepSeek 内容审查服务
            ModerationRequest modReq;
            modReq.content = message;
            auto modResult = moderationService->moderate(modReq);
            
            if (modResult.isSuccess()) {
                auto modResp = modResult.getValue();
                double riskScore = static_cast<double>(modResp.overallRiskScore);
                bool isAbusive = false;
                
                if (modResp.overallVerdict == "block") {
                    isAbusive = true;
                    riskScore = std::max(riskScore, 0.95);
                } else if (modResp.overallVerdict == "review") {
                    // 风险评分 >= 阈值时标记为可能辱骂
                    isAbusive = (riskScore >= AI_REVIEW_THRESHOLD);
                }
                
                logger->debug("DeepSeek 审核完成: verdict={}, riskScore={}",
                             modResp.overallVerdict, riskScore);
                return Result<std::pair<bool, double>>::Success(std::make_pair(isAbusive, riskScore));
            } else {
                logger->warn("DeepSeek 审核调用失败: {}，回退到启发式审查",
                            modResult.getError().message);
            }
        }
        
        // 回退: 基于启发式方法的 AI 内容审查
        double riskScore = 0.0;
        bool isAbusive = false;
        
        // 检查1: 全大写内容 (可能的大喊)
        int upperCount = 0;
        for (char c : message) {
            if (std::isupper(c)) upperCount++;
        }
        if (upperCount > message.length() * 0.5) {
            riskScore += 0.1;
        }
        
        // 检查2: 重复字符 (垃圾/情绪表达)
        for (size_t i = 0; i + 2 < message.length(); ++i) {
            if (message[i] == message[i+1] && message[i+1] == message[i+2]) {
                riskScore += 0.15;
                break;
            }
        }
        
        // 检查3: 特殊字符密度
        int specialCount = 0;
        for (char c : message) {
            if (!std::isalnum(c) && c != ' ') specialCount++;
        }
        if (specialCount > message.length() * 0.3) {
            riskScore += 0.2;
        }
        
        // 检查4: 消息长度异常
        if (message.length() < 2) {
            riskScore += 0.1;
        }
        if (message.length() > 400) {
            riskScore += 0.05;
        }
        
        // 限制评分在0-1范围
        riskScore = std::min(1.0, riskScore);
        
        // 如果评分超过0.6，标记为可能辱骂
        isAbusive = (riskScore > 0.6);
        
        // 返回 {是否滥用, 风险评分}
        return Result<std::pair<bool, double>>::Success(std::make_pair(isAbusive, riskScore));
        
    } catch (const std::exception& e) {
        logger->error("AI内容审查异常: {}", e.what());
        return Result<std::pair<bool, double>>::Success(std::make_pair(false, 0.0));
    }
}

Result<bool> MessageServiceImpl::behaviorAnalysis(int64_t userId, const std::string& userIp) {
    try {
        using RedisUtil = Yachiyo::Utils::RedisUtil;
        bool hasAbnormalBehavior = false;
        
        // 行为分析1: 短时间内大量消息 (基于Redis原子计数器)
        std::string userKey = "user_activity:" + std::to_string(userId);
        try {
            // 使用 INCR 原子操作，与 checkRateLimit 保持一致，避免 get+set 竞态条件
            auto conn = RedisUtil::getConnection();
            if (conn) {
                std::string countStr = conn->executeCommand("INCR", {userKey});
                int messageCount = 0;
                try { messageCount = std::stoi(countStr); } catch (...) { messageCount = 1; }
                // 首次创建 key 时设置 TTL (5分钟窗口)
                if (messageCount == 1) {
                    conn->expire(userKey, 300);
                }
                RedisUtil::releaseConnection(conn);
                if (messageCount > 20) {
                    hasAbnormalBehavior = true;
                    logger->warn("用户异常行为检测: {} 5分钟内消息数:{}", userId, messageCount);
                }
            }
        } catch (...) {}
        
        // 行为分析2: 多个IP发送消息 (可能的账号共享或被盗用)
        auto result = dbUtil->query(
            "SELECT DISTINCT user_ip FROM user_messages "
            "WHERE user_id = $1 AND created_at > NOW() - INTERVAL '24 hours' "
            "LIMIT 10",
            {std::to_string(userId)}
        );
        
        if (result.size() > 5) {
            hasAbnormalBehavior = true;
            logger->warn("用户异常行为检测: {} 24小时内从{}个不同IP发送消息", userId, result.size());
        }
        
        // 行为分析3: 检查该IP的其他用户是否大量发送被拒消息
        auto ipSpamResult = dbUtil->query(
            "SELECT COUNT(*) as rejected_count FROM user_messages "
            "WHERE user_ip = $1 AND review_status = $2 "
            "AND created_at > NOW() - INTERVAL '1 hour'",
            {userIp, std::to_string(static_cast<int>(ReviewStatus::REJECTED))}
        );
        
        if (!ipSpamResult.empty()) {
            int rejectedCount = std::stoi(ipSpamResult[0]["rejected_count"]);
            if (rejectedCount > 10) {
                hasAbnormalBehavior = true;
                logger->warn("IP异常行为检测: {} 1小时内被拒消息数:{}", userIp, rejectedCount);
            }
        }
        
        if (hasAbnormalBehavior) {
            return Result<bool>::Error("检测到异常行为");
        }
        
        return Result<bool>::Success(true);
        
    } catch (const std::exception& e) {
        logger->error("行为分析异常: {}", e.what());
        return Result<bool>::Success(true);
    }
}

bool MessageServiceImpl::needsManualReview(const std::shared_ptr<Message>& message) {
    // 多因素判定是否需要人工审查
    
    // 因素1: 风险评分
    if (message->getSpamScore() > 0.5) {
        return true;
    }
    
    // 因素2: 包含敏感词但风险中等
    if (message->getIsBlockedKeyword() && message->getSpamScore() > 0.3) {
        return true;
    }
    
    // 因素3: AI检测到可能辱骂内容
    if (message->getIsAbusive() && message->getSpamScore() > 0.4) {
        return true;
    }
    
    // 因素4: 异常行为标记
    if (message->getIsSpam() && message->getSpamScore() > 0.3) {
        return true;
    }
    
    // 因素5: 速率限制违规
    if (message->getRateLimitViolated()) {
        return true;
    }
    
    // 所有检查都通过
    return false;
}

} // namespace yachiyo::services
