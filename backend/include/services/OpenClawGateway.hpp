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
#include "utils/Compat.hpp"
#include "dto/OpenClawDTO.hpp"

using json = nlohmann::json;

namespace yachiyo::services {

/**
 * OpenClaw 网关服务 - 通过 Node.js 桥接服务与 OpenClaw 通信
 * 
 * 架构:
 *   C++ 后端 --HTTP POST :8765--> 桥接服务 --转发--> OpenClaw (:8000)
 *   C++ 后端 <--JSON 响应-------- 桥接服务 <--响应-- OpenClaw (:8000)
 * 
 * 纯同步模式: POST /process → 桥接服务同步转发并返回结果
 * 桥接服务是无状态 HTTP 代理，无会话管理，无异步回调。
 */
class OpenClawGateway {
public:
    OpenClawGateway();
    ~OpenClawGateway();
    
    /**
     * 初始化网关 (连接到桥接服务)
     * @param bridgeEndpoint 桥接服务地址 (默认 http://localhost:8765)
     * @param timeoutSeconds 请求超时
     */
    bool initialize(
        const std::string& bridgeEndpoint = "http://localhost:8765",
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
    
    std::string bridge_endpoint_;   // 桥接服务地址 (默认 http://localhost:8765)
    int timeout_;
    
    // 缓存
    std::map<std::string, CacheEntry> cache_;
    std::mutex cache_mutex_;
    
    // 情感提示
    std::vector<std::string> emotion_hints_;
    
    // 运行状态
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
