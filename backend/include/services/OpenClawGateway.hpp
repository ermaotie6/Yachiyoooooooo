#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>
#include "utils/Result.hpp"
#include "dto/OpenClawDTO.hpp"

using json = nlohmann::json;

namespace yachiyo::services {

/**
 * OpenClaw 网关服务 - 处理与 OpenClaw AI 的通信
 * 
 * 职责:
 * - 发送用户消息到 OpenClaw
 * - 接收包含文本、表情、动作的响应
 * - 管理请求/响应缓存
 * - 处理错误和降级
 */
class OpenClawGateway {
public:
    OpenClawGateway();
    ~OpenClawGateway();
    
    /**
     * 初始化网关 (连接到 OpenClaw 服务)
     */
    bool initialize(const std::string& endpoint, int timeoutSeconds = 30);
    
    /**
     * 处理用户消息
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
     * @param emotions 情感标签列表
     */
    void setEmotionHints(const std::vector<std::string>& emotions);
    
    /**
     * 获取缓存的响应 (如果存在)
     * @param cacheKey 缓存键
     * @return 缓存的响应，如果不存在返回 nullptr
     */
    std::shared_ptr<dto::OpenClawResponse> getCachedResponse(const std::string& cacheKey);
    
    /**
     * 清空所有缓存
     */
    void clearCache();
    
    /**
     * 获取服务状态
     */
    bool isHealthy() const;

private:
    struct CacheEntry {
        dto::OpenClawResponse response;
        int64_t timestamp;
        int ttl;  // 生存时间 (秒)
    };
    
    std::string endpoint_;
    int timeout_;
    std::map<std::string, CacheEntry> cache_;
    std::mutex cache_mutex_;
    std::vector<std::string> emotion_hints_;
    
    /**
     * 生成缓存键
     */
    std::string generateCacheKey(const std::string& text) const;
    
    /**
     * 执行 HTTP POST 请求到 OpenClaw
     */
    Utils::Result<json> sendRequest(const json& request);
    
    /**
     * 清理过期缓存
     */
    void cleanupExpiredCache();
};

} // namespace yachiyo::services
