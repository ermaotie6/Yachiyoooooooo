#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "utils/Result.hpp"
#include "dto/OpenClawDTO.hpp"

using json = nlohmann::json;

namespace yachiyo::services {

/**
 * OpenClaw 网关服务 - 通过 Node.js 桥接服务与 OpenClaw 通信
 * 
 * 架构:
 *   C++ 后端 --HTTP POST :8765--> 桥接服务 --session--> OpenClaw
 *   C++ 后端 <--HTTP POST :8766-- 桥接服务 <--result--- OpenClaw
 * 
 * 同步模式: POST /process → 桥接服务同步返回结果
 * 异步模式: POST /process → 桥接服务异步回调 :8766/callback
 */
class OpenClawGateway {
public:
    OpenClawGateway();
    ~OpenClawGateway();
    
    /**
     * 初始化网关 (连接到桥接服务)
     * @param bridgeEndpoint 桥接服务接收端地址 (默认 http://localhost:8765)
     * @param callbackPort 本地回调监听端口 (默认 8766)
     * @param timeoutSeconds 请求超时
     */
    bool initialize(
        const std::string& bridgeEndpoint = "http://localhost:8765",
        int callbackPort = 8766,
        int timeoutSeconds = 30
    );
    
    /**
     * 处理用户消息 (同步模式 — 通过桥接服务转发到 OpenClaw)
     * @param userId 用户 ID
     * @param text 用户输入的文本
     * @param context 对话上下文
     * @return OpenClaw 响应 (包含文本、表情、动作)
     */
    Utils::Result<dto::OpenClawResponse> processMessage(
        const std::string& userId,
        const std::string& text,
        const std::string& context = ""
    );
    
    /**
     * 异步处理用户消息 (发送后不等待，通过回调获取结果)
     * @param userId 用户 ID
     * @param text 用户输入的文本
     * @param callback 结果回调函数
     * @return 请求 ID
     */
    std::string processMessageAsync(
        const std::string& userId,
        const std::string& text,
        std::function<void(const Utils::Result<dto::OpenClawResponse>&)> callback
    );
    
    /**
     * 设置情感提示 (可选)
     */
    void setEmotionHints(const std::vector<std::string>& emotions);
    
    /**
     * 获取缓存的响应 (如果存在)
     */
    std::shared_ptr<dto::OpenClawResponse> getCachedResponse(const std::string& cacheKey);
    
    /**
     * 清空所有缓存
     */
    void clearCache();
    
    /**
     * 获取桥接服务状态
     */
    bool isHealthy() const;
    
    /**
     * 关闭网关 (停止回调监听线程)
     */
    void shutdown();

private:
    struct CacheEntry {
        dto::OpenClawResponse response;
        int64_t timestamp;
        int ttl;  // 生存时间 (秒)
    };
    
    // 异步回调上下文
    struct PendingRequest {
        std::string requestId;
        std::function<void(const Utils::Result<dto::OpenClawResponse>&)> callback;
        int64_t timestamp;
    };
    
    std::string bridge_endpoint_;   // 桥接服务地址 (默认 http://localhost:8765)
    int callback_port_;             // 本地回调监听端口 (默认 8766)
    int timeout_;
    
    // 缓存
    std::map<std::string, CacheEntry> cache_;
    std::mutex cache_mutex_;
    
    // 情感提示
    std::vector<std::string> emotion_hints_;
    
    // 异步回调
    std::map<std::string, PendingRequest> pending_requests_;
    std::mutex pending_mutex_;
    
    // 回调监听线程
    std::thread callback_thread_;
    std::atomic<bool> running_{false};
    
    /**
     * 生成缓存键
     */
    std::string generateCacheKey(const std::string& text) const;
    
    /**
     * 发送 HTTP POST 请求到桥接服务
     */
    Utils::Result<json> sendToBridge(const json& request);
    
    /**
     * 清理过期缓存
     */
    void cleanupExpiredCache();
};

} // namespace yachiyo::services
