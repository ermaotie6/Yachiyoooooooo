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
    : bridge_endpoint_(""), callback_port_(8766), timeout_(30) {
}

OpenClawGateway::~OpenClawGateway() {
    shutdown();
    clearCache();
}

// ==================== 初始化 ====================

bool OpenClawGateway::initialize(
    const std::string& bridgeEndpoint,
    int callbackPort,
    int timeoutSeconds
) {
    LOG_INFO("初始化 OpenClaw 网关 (桥接模式): bridge={}, callbackPort={}", 
             bridgeEndpoint, callbackPort);
    
    bridge_endpoint_ = bridgeEndpoint;
    callback_port_ = callbackPort;
    timeout_ = timeoutSeconds;
    
    // 检查桥接服务是否可访问
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("CURL 初始化失败");
        return false;
    }
    
    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, (bridge_endpoint_ + "/health").c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_WARN("桥接服务不可用: {} — 将在首次请求时重试", curl_easy_strerror(res));
        // 不阻止初始化, 桥接服务可能稍后启动
    } else if (http_code == 200) {
        try {
            json healthResp = json::parse(readBuffer);
            LOG_INFO("桥接服务已连接: status={}, activeSessions={}", 
                     healthResp.value("status", "unknown"),
                     healthResp.value("activeSessions", 0));
        } catch (...) {
            LOG_INFO("桥接服务已连接 (HTTP 200)");
        }
    }
    
    running_ = true;
    LOG_INFO("OpenClaw 网关初始化完成 (桥接模式)");
    return true;
}

// ==================== 处理消息 (同步) ====================

Utils::Result<dto::OpenClawResponse> OpenClawGateway::processMessage(
    const std::string& userId,
    const std::string& text,
    const std::string& context
) {
    auto startTime = std::chrono::steady_clock::now();
    
    // 检查缓存
    std::string cacheKey = generateCacheKey(text);
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(cacheKey);
        if (it != cache_.end()) {
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
    
    // 构建桥接服务请求
    json request;
    request["request_id"] = "req_" + userId + "_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    request["user_id"] = userId;
    request["text"] = text;
    request["context"] = context;
    request["emotion_hints"] = emotion_hints_;
    request["max_tokens"] = 1000;
    request["temperature"] = 0.7;
    
    // 发送到桥接服务 (同步等待)
    auto result = sendToBridge(request);
    if (!result.isSuccess()) {
        return Utils::Result<dto::OpenClawResponse>::error(
            result.getCode(),
            result.getMessage()
        );
    }
    
    // 解析桥接服务返回的响应
    try {
        json responseJson = result.getData().value();
        dto::OpenClawResponse response = dto::OpenClawResponse::fromJson(responseJson);
        
        // 计算端到端处理时间
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
                300  // 5 分钟 TTL (对话场景缓存时间缩短)
            };
        }
        
        LOG_INFO("桥接服务返回成功 ({}ms): text=\"{}...\"", 
                 response.processingTimeMs,
                 response.text.substr(0, 40));
        
        return Utils::Result<dto::OpenClawResponse>::success(response);
    } catch (const std::exception& e) {
        LOG_ERROR("解析桥接服务响应失败: {}", e.what());
        return Utils::Result<dto::OpenClawResponse>::error(
            "PARSE_ERROR",
            std::string("解析响应失败: ") + e.what()
        );
    }
}

// ==================== 异步处理消息 ====================

std::string OpenClawGateway::processMessageAsync(
    const std::string& userId,
    const std::string& text,
    std::function<void(const Utils::Result<dto::OpenClawResponse>&)> callback
) {
    std::string requestId = "req_" + userId + "_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    
    // 注册回调
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_requests_[requestId] = {
            requestId,
            callback,
            std::chrono::steady_clock::now().time_since_epoch().count()
        };
    }
    
    // 在新线程中发送请求
    std::thread([this, userId, text, requestId]() {
        auto result = processMessage(userId, text);
        
        std::function<void(const Utils::Result<dto::OpenClawResponse>&)> cb;
        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            auto it = pending_requests_.find(requestId);
            if (it != pending_requests_.end()) {
                cb = it->second.callback;
                pending_requests_.erase(it);
            }
        }
        
        if (cb) {
            cb(result);
        }
    }).detach();
    
    return requestId;
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
    curl_easy_setopt(curl, CURLOPT_URL, (bridge_endpoint_ + "/health").c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    
    return res == CURLE_OK && http_code == 200;
}

// ==================== 关闭 ====================

void OpenClawGateway::shutdown() {
    running_ = false;
    if (callback_thread_.joinable()) {
        callback_thread_.join();
    }
}

// ==================== 私有方法 ====================

std::string OpenClawGateway::generateCacheKey(const std::string& text) const {
    return "bridge_" + std::to_string(std::hash<std::string>{}(text));
}

Utils::Result<json> OpenClawGateway::sendToBridge(const json& request) {
    LOG_DEBUG("发送请求到桥接服务: {}/process", bridge_endpoint_);
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        return Utils::Result<json>::error("CURL_INIT_ERROR", "CURL 初始化失败");
    }
    
    std::string readBuffer;
    std::string postData = request.dump();
    
    curl_easy_setopt(curl, CURLOPT_URL, (bridge_endpoint_ + "/process").c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout_);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("桥接服务请求失败: {}", curl_easy_strerror(res));
        return Utils::Result<json>::error(
            "BRIDGE_ERROR", 
            std::string("桥接服务不可达: ") + curl_easy_strerror(res)
        );
    }
    
    if (http_code >= 400) {
        LOG_ERROR("桥接服务返回错误: HTTP {}, body={}", http_code, readBuffer);
        return Utils::Result<json>::error(
            "BRIDGE_HTTP_ERROR",
            "桥接服务返回 HTTP " + std::to_string(http_code)
        );
    }
    
    try {
        json response = json::parse(readBuffer);
        
        // 检查桥接服务返回的 success 字段
        if (response.contains("success") && !response["success"].get<bool>()) {
            std::string errorMsg = response.value("error", "未知错误");
            LOG_ERROR("桥接服务返回失败: {}", errorMsg);
            return Utils::Result<json>::error("BRIDGE_PROCESS_ERROR", errorMsg);
        }
        
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
