#include "services/OpenClawGateway.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <curl/curl.h>
#include <chrono>
#include <algorithm>

namespace yachiyo::services {

// ==================== 辅助函数 ====================

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ==================== 构造/析构 ====================

OpenClawGateway::OpenClawGateway() 
    : endpoint_(""), timeout_(30) {
}

OpenClawGateway::~OpenClawGateway() {
    clearCache();
}

// ==================== 初始化 ====================

bool OpenClawGateway::initialize(const std::string& endpoint, int timeoutSeconds) {
    LOG_INFO("初始化 OpenClaw 网关: endpoint={}", endpoint);
    
    endpoint_ = endpoint;
    timeout_ = timeoutSeconds;
    
    // 检查端点是否可访问
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("CURL 初始化失败");
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, (endpoint_ + "/health").c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout_);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)timeout_);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_WARN("OpenClaw 端点不可用: {}", curl_easy_strerror(res));
        return false;
    }
    
    LOG_INFO("OpenClaw 网关初始化完成");
    return true;
}

// ==================== 处理消息 ====================

Utils::Result<dto::OpenClawResponse> OpenClawGateway::processMessage(
    const std::string& userId,
    const std::string& text,
    const std::string& context
) {
    auto startTime = std::chrono::steady_clock::now();
    
    // 生成缓存键
    std::string cacheKey = generateCacheKey(text);
    
    // 检查缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(cacheKey);
        if (it != cache_.end()) {
            // 检查 TTL（使用 chrono 正确计算秒数）
            auto now = std::chrono::steady_clock::now();
            auto cacheTime = std::chrono::steady_clock::time_point(
                std::chrono::steady_clock::duration(it->second.timestamp)
            );
            auto ageSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                now - cacheTime
            ).count();
            
            if (ageSeconds < it->second.ttl) {
                LOG_DEBUG("使用缓存的 OpenClaw 响应");
                return Utils::Result<dto::OpenClawResponse>::success(it->second.response);
            }
        }
    }
    
    // 构建请求
    json request;
    request["request_id"] = "req_" + userId + "_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    request["text"] = text;
    request["context"] = context;
    request["emotion_hints"] = emotion_hints_;
    request["max_tokens"] = 1000;
    request["temperature"] = 0.7;
    
    // 发送请求
    auto result = sendRequest(request);
    if (!result.isSuccess()) {
        return Utils::Result<dto::OpenClawResponse>::error(
            result.getCode(),
            result.getMessage()
        );
    }
    
    // 解析响应
    try {
        json responseJson = result.getData().value();
        dto::OpenClawResponse response = dto::OpenClawResponse::fromJson(responseJson);
        
        // 计算处理时间
        auto endTime = std::chrono::steady_clock::now();
        response.processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        ).count();
        
        // 保存到缓存
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cache_[cacheKey] = {
                response,
                std::chrono::steady_clock::now().time_since_epoch().count(),
                3600  // 1 小时 TTL
            };
        }
        
        return Utils::Result<dto::OpenClawResponse>::success(response);
    } catch (const std::exception& e) {
        LOG_ERROR("解析 OpenClaw 响应失败: {}", e.what());
        return Utils::Result<dto::OpenClawResponse>::error(
            "PARSE_ERROR",
            std::string("解析响应失败: ") + e.what()
        );
    }
}

// ==================== 情感提示 ====================

void OpenClawGateway::setEmotionHints(const std::vector<std::string>& emotions) {
    emotion_hints_ = emotions;
}

// ==================== 缓存管理 ====================

std::shared_ptr<dto::OpenClawResponse> OpenClawGateway::getCachedResponse(
    const std::string& cacheKey
) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = cache_.find(cacheKey);
    if (it != cache_.end()) {
        return std::make_shared<dto::OpenClawResponse>(it->second.response);
    }
    return nullptr;
}

void OpenClawGateway::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
}

// ==================== 健康检查 ====================

bool OpenClawGateway::isHealthy() const {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, (endpoint_ + "/health").c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    
    return res == CURLE_OK && http_code == 200;
}

// ==================== 私有方法 ====================

std::string OpenClawGateway::generateCacheKey(const std::string& text) const {
    // 使用简单的 hash (实际应使用 MD5/SHA256)
    return "openclaw_" + std::to_string(std::hash<std::string>{}(text));
}

Utils::Result<json> OpenClawGateway::sendRequest(const json& request) {
    LOG_DEBUG("发送 OpenClaw 请求");
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        return Utils::Result<json>::error("CURL_INIT_ERROR", "CURL 初始化失败");
    }
    
    std::string readBuffer;
    std::string postData = request.dump();
    
    curl_easy_setopt(curl, CURLOPT_URL, (endpoint_ + "/process").c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout_);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("OpenClaw 请求失败: {}", curl_easy_strerror(res));
        return Utils::Result<json>::error("REQUEST_ERROR", curl_easy_strerror(res));
    }
    
    try {
        json response = json::parse(readBuffer);
        return Utils::Result<json>::success(response);
    } catch (const std::exception& e) {
        LOG_ERROR("JSON 解析失败: {}", e.what());
        return Utils::Result<json>::error("JSON_ERROR", e.what());
    }
}

void OpenClawGateway::cleanupExpiredCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = cache_.begin(); it != cache_.end();) {
        auto cacheTime = std::chrono::steady_clock::time_point(
            std::chrono::steady_clock::duration(it->second.timestamp)
        );
        auto ageSeconds = std::chrono::duration_cast<std::chrono::seconds>(
            now - cacheTime
        ).count();
        if (ageSeconds > it->second.ttl) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace yachiyo::services
