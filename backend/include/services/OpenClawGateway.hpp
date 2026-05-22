#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <chrono>
#include "utils/Result.hpp"
#include "utils/Compat.hpp"
#include "dto/OpenClawDTO.hpp"

using json = nlohmann::json;

namespace yachiyo::services {

/**
 * OpenClaw 网关服务 — 直连 OpenClaw Gateway 的 /v1/chat/completions API
 *
 * 架构:
 *   C++ 后端 —HTTP POST :8100/v1/chat/completions—→ OpenClaw Gateway
 *   C++ 后端 ←OpenAI-format JSON 响应—————————————— OpenClaw Gateway
 *
 * 特性:
 *   - 主路径: System Prompt 要求 JSON 结构化输出 → json::parse
 *   - Fallback: 旧版文本标签 [emotion:xxx] [action:xxx] [translated] → regex
 *   - 重试: 最多 2 次, 指数退避 {1s, 3s}
 *   - 熔断: 连续 3 次失败 → 短路 30s
 */
class OpenClawGateway {
public:
    OpenClawGateway();
    ~OpenClawGateway();

    /** 重试配置 */
    struct RetryConfig {
        int maxRetries = 2;
        std::vector<int> backoffMs = {1000, 3000};  // 指数退避
    };

    /** 熔断配置 */
    struct CircuitBreakerConfig {
        int failureThreshold = 3;
        int cooldownSeconds = 30;
    };

    bool initialize(
        const std::string& openclawEndpoint,
        const std::string& authToken = "",
        const std::string& model = "deepseek/deepseek-v4-flash",
        int timeoutSeconds = 30
    );

    void setSystemPrompt(const std::string& prompt);
    void setEmotionHints(const std::vector<std::string>& emotions);

    void setRetryConfig(const RetryConfig& cfg) { retry_config_ = cfg; }
    void setCircuitBreakerConfig(const CircuitBreakerConfig& cfg) { cb_config_ = cfg; }

    Utils::Result<dto::OpenClawResponse> processMessage(
        const std::string& userId,
        const std::string& text,
        const std::string& context = ""
    );

    std::shared_ptr<dto::OpenClawResponse> getCachedResponse(const std::string& cacheKey);
    void clearCache();
    bool isHealthy();
    void shutdown();

private:
    struct CacheEntry {
        dto::OpenClawResponse response;
        int64_t timestamp;
        int ttl;
    };

    // 配置
    std::string openclaw_endpoint_;
    std::string auth_token_;
    std::string model_;
    std::string system_prompt_;
    int timeout_secs_;
    RetryConfig retry_config_;
    CircuitBreakerConfig cb_config_;

    // 缓存
    std::map<std::string, CacheEntry> cache_;
    std::mutex cache_mutex_;

    // 情感提示
    std::vector<std::string> emotion_hints_;

    // 熔断状态
    std::atomic<int> consecutive_failures_{0};
    std::chrono::steady_clock::time_point circuit_open_until_{};

    // 运行状态
    std::atomic<bool> running_{false};

    // ====== 私有方法 ======

    std::string generateCacheKey(const std::string& text) const;
    Utils::Result<json> sendToOpenClaw(const std::string& userMessage);

    /** 带重试和熔断的发送 */
    Utils::Result<json> sendWithRetry(const std::string& userMessage);

    /** 检查熔断器状态 */
    bool isCircuitOpen();

    /** 记录请求结果（更新熔断计数） */
    void recordResult(bool success);

    /** 主解析路径: JSON 结构化输出 */
    std::optional<dto::OpenClawResponse> parseJsonResponse(const std::string& rawContent);

    /** Fallback 解析路径: 文本标签 (兼容旧版 System Prompt) */
    dto::OpenClawResponse parseTaggedResponse(const std::string& rawContent);

    void cleanupExpiredCache();
};

} // namespace yachiyo::services
