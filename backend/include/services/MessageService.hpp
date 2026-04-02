#pragma once

#include "../models/Message.hpp"
#include "../utils/Result.hpp"
#include <string>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace yachiyo::services {

using Models::Message, Models::ReviewStatus;
using Utils::Result;

/**
 * @brief 消息服务接口 (Phase 2 - 6层安全审查系统)
 * 
 * 6层审查流程:
 * 1. 速率限制 - 防止消息洪泛
 * 2. IP检查 - 黑名单IP检测
 * 3. 关键词过滤 - 敏感词检测
 * 4. AI内容审查 - 使用openclaw检测有害内容
 * 5. 行为分析 - 检测异常用户行为
 * 6. 人工审查 - 标记待人工审查的消息
 */
class IMessageService {
public:
    virtual ~IMessageService() = default;
    
    /**
     * @brief 发送消息 (执行6层安全检查)
     * @param userId 用户ID
     * @param message 消息内容
     * @param userIp 用户IP
     * @param userAgent 用户浏览器信息
     * @return Result<std::shared_ptr<Message>> 消息对象或错误信息
     */
    virtual Result<std::shared_ptr<Message>> sendMessage(
        int64_t userId,
        const std::string& message,
        const std::string& userIp,
        const std::string& userAgent = ""
    ) = 0;
    
    /**
     * @brief 审查消息 (管理员功能)
     * @param messageId 消息ID
     * @param reviewerId 审查员ID
     * @param approved 是否通过审查
     * @param reason 审查理由
     * @return Result<bool> 操作是否成功
     */
    virtual Result<bool> reviewMessage(
        int64_t messageId,
        int64_t reviewerId,
        bool approved,
        const std::string& reason = ""
    ) = 0;
    
    /**
     * @brief 获取用户消息列表
     * @param userId 用户ID
     * @param limit 返回数量
     * @param offset 偏移量
     * @return Result<std::vector<std::shared_ptr<Message>>> 消息列表
     */
    virtual Result<std::vector<std::shared_ptr<Message>>> getUserMessages(
        int64_t userId,
        int32_t limit = 20,
        int32_t offset = 0
    ) = 0;
    
    /**
     * @brief 获取待审查消息列表 (管理员功能)
     * @param limit 返回数量
     * @param offset 偏移量
     * @return Result<std::vector<std::shared_ptr<Message>>> 消息列表
     */
    virtual Result<std::vector<std::shared_ptr<Message>>> getPendingMessages(
        int32_t limit = 50,
        int32_t offset = 0
    ) = 0;
    
    /**
     * @brief 获取高风险消息列表 (管理员功能)
     * @param limit 返回数量
     * @param offset 偏移量
     * @return Result<std::vector<std::shared_ptr<Message>>> 高风险消息列表
     */
    virtual Result<std::vector<std::shared_ptr<Message>>> getHighRiskMessages(
        int32_t limit = 50,
        int32_t offset = 0
    ) = 0;
    
    /**
     * @brief 获取消息统计信息
     * @return Result<json> 统计数据JSON
     */
    virtual Result<json> getStatistics() = 0;
    
    /**
     * @brief 阻止消息被看到 (隐藏消息)
     * @param messageId 消息ID
     * @param reason 隐藏理由
     * @return Result<bool> 操作是否成功
     */
    virtual Result<bool> hideMessage(
        int64_t messageId,
        const std::string& reason = ""
    ) = 0;
};

} // namespace yachiyo::services
