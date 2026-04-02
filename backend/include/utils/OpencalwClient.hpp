#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "../utils/Result.hpp"

using json = nlohmann::json;

namespace yachiyo::utils {

/**
 * @class OpencalwClient
 * @brief openclaw AI内容审查客户端
 * 
 * 功能:
 * - 调用openclaw API进行内容安全审查
 * - 返回内容违规类型和风险评分
 * - 支持错误重试和超时处理
 * 
 * Phase 3集成: 当前为框架实现，准备集成真实openclaw API
 */
class OpencalwClient {
public:
    /**
     * @struct ReviewResult
     * @brief AI审查结果
     */
    struct ReviewResult {
        bool isAllowed;              // 是否允许
        double riskScore;            // 风险评分 (0.0-1.0)
        std::string mainCategory;    // 主违规类别
        std::vector<std::string> categories; // 所有违规类别
        std::string rawResponse;     // 原始响应 (调试用)
    };
    
    OpencalwClient(const std::string& apiKey, const std::string& apiUrl);
    ~OpencalwClient() = default;
    
    /**
     * @brief 审查消息内容
     * @param message 要审查的消息
     * @param userId 用户ID (可选, 用于日志追踪)
     * @return 审查结果
     */
    Result<ReviewResult> reviewContent(const std::string& message, int64_t userId = 0);
    
    /**
     * @brief 批量审查消息
     * @param messages 消息列表
     * @return 审查结果列表
     */
    Result<std::vector<ReviewResult>> batchReview(
        const std::vector<std::pair<int64_t, std::string>>& messages
    );
    
    /**
     * @brief 健康检查 - 验证API连接
     * @return 是否连接成功
     */
    Result<bool> healthCheck();
    
    /**
     * @brief 获取API限流配额信息
     * @return 配额信息
     */
    Result<json> getRateLimit();
    
private:
    std::string apiKey_;
    std::string apiUrl_;
    static constexpr int TIMEOUT_SECONDS = 10;
    static constexpr int MAX_RETRIES = 3;
    
    /**
     * @brief 发送HTTP POST请求
     * @param endpoint API端点
     * @param payload 请求体
     * @return 响应
     */
    Result<json> sendRequest(const std::string& endpoint, const json& payload);
    
    /**
     * @brief 解析openclaw API响应
     * @param response API响应JSON
     * @return 解析后的审查结果
     */
    ReviewResult parseResponse(const json& response);
    
    /**
     * @brief 计算风险评分
     * @param categories 违规类别列表
     * @return 聚合风险评分
     */
    double calculateRiskScore(const std::vector<std::string>& categories);
};

} // namespace yachiyo::utils
