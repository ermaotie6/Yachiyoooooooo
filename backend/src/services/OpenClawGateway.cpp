#include "services/OpenClawGateway.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <curl/curl.h>
#include <chrono>
#include <regex>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <thread>

namespace yachiyo::services {

// ==================== 辅助函数 ====================

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// ==================== 构造/析构 ====================

OpenClawGateway::OpenClawGateway()
    : openclaw_endpoint_(""), auth_token_(""), model_("deepseek/deepseek-v4-flash"), timeout_secs_(30) {
}

OpenClawGateway::~OpenClawGateway() {
    shutdown();
    clearCache();
}

// ==================== 初始化 ====================

bool OpenClawGateway::initialize(
    const std::string& openclawEndpoint, const std::string& authToken,
    const std::string& model, int timeoutSeconds) {

    LOG_INFO("初始化 OpenClaw 网关 v2 (JSON 输出 + 重试 + 熔断)");
    LOG_INFO("  endpoint: {}", openclawEndpoint);
    LOG_INFO("  model:    {}", model);
    LOG_INFO("  timeout:  {}s", timeoutSeconds);

    openclaw_endpoint_ = openclawEndpoint;
    auth_token_ = authToken;
    model_ = model;
    timeout_secs_ = timeoutSeconds;
    consecutive_failures_ = 0;
    circuit_open_until_ = {};

    // 尝试验证连通性
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("CURL 初始化失败");
        return false;
    }

    std::string readBuffer;
    std::string healthUrl = openclaw_endpoint_ + "/v1/models";
    curl_easy_setopt(curl, CURLOPT_URL, healthUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    struct curl_slist* headers = nullptr;
    if (!auth_token_.empty()) {
        std::string authHeader = "Authorization: Bearer " + auth_token_;
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && http_code == 200) {
        LOG_INFO("OpenClaw Gateway 已连接 (HTTP 200)");
    } else if (res != CURLE_OK) {
        LOG_WARN("OpenClaw Gateway 连通性检查失败: {}", curl_easy_strerror(res));
    } else {
        LOG_WARN("OpenClaw Gateway 返回 HTTP {}", http_code);
    }

    running_ = true;
    LOG_INFO("OpenClaw 网关初始化完成");
    return true;
}

void OpenClawGateway::setSystemPrompt(const std::string& prompt) {
    system_prompt_ = prompt;
    LOG_INFO("System prompt 已设置 ({} 字符)", prompt.size());
}

void OpenClawGateway::setEmotionHints(const std::vector<std::string>& emotions) {
    emotion_hints_ = emotions;
}

// ==================== 处理消息 ====================

Utils::Result<dto::OpenClawResponse> OpenClawGateway::processMessage(
    const std::string& userId, const std::string& text, const std::string& context) {

    auto startTime = std::chrono::steady_clock::now();

    // 检查缓存
    std::string cacheKey = generateCacheKey(userId + ":" + text);
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(cacheKey);
        if (it != cache_.end()) {
            auto now = std::chrono::steady_clock::now();
            auto cacheTime = std::chrono::steady_clock::time_point(
                std::chrono::steady_clock::duration(it->second.timestamp));
            auto ageSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - cacheTime).count();
            if (ageSeconds < it->second.ttl) {
                LOG_DEBUG("使用缓存的 OpenClaw 响应");
                return Utils::Result<dto::OpenClawResponse>::success(it->second.response);
            }
        }
    }

    // 构建用户消息
    std::string userMessage = context.empty() ? text : context + "\n\n（用户刚才说：）" + text;

    // 带重试和熔断的发送
    auto result = sendWithRetry(userMessage);
    if (!result.isSuccess()) {
        return Utils::Result<dto::OpenClawResponse>::error(result.getCode(), result.getMessage());
    }

    // 解析响应
    try {
        json responseJson = result.getData().value();
        std::string rawContent;

        if (responseJson.contains("choices") && responseJson["choices"].is_array()
            && !responseJson["choices"].empty()) {
            const auto& choice = responseJson["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                rawContent = choice["message"]["content"].get<std::string>();
            }
        }

        if (rawContent.empty()) {
            LOG_ERROR("OpenClaw 返回了空响应");
            return Utils::Result<dto::OpenClawResponse>::error("EMPTY_RESPONSE", "OpenClaw 返回空内容");
        }

        LOG_DEBUG("OpenClaw raw (前200字符): {}", rawContent.substr(0, 200));

        // === 解析：优先 JSON 结构化输出，fallback 文本标签 ===
        auto parsedJson = parseJsonResponse(rawContent);
        dto::OpenClawResponse response;

        if (parsedJson.has_value()) {
            response = parsedJson.value();
            LOG_DEBUG("JSON 解析成功: text={}, emotion={}, action={}",
                      response.text.substr(0, 30),
                      response.emotions.empty() ? "none" : response.emotions[0],
                      response.actions.empty() ? "none" : response.actions[0]);
        } else {
            // Fallback: 旧版 [emotion:xxx] [action:xxx] [translated] 标签格式
            response = parseTaggedResponse(rawContent);
            LOG_DEBUG("Fallback 文本标签解析: text={}", response.text.substr(0, 30));
        }

        response.success = true;
        auto endTime = std::chrono::steady_clock::now();
        response.processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();

        // 保存到缓存
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cache_[cacheKey] = {response,
                std::chrono::steady_clock::now().time_since_epoch().count(), 300};
        }

        LOG_INFO("OpenClaw 成功 ({}ms): text=\"{}\", emotion={}, action={}",
                 response.processingTimeMs,
                 response.text.substr(0, 40),
                 response.emotions.empty() ? "none" : response.emotions[0],
                 response.actions.empty() ? "none" : response.actions[0]);

        return Utils::Result<dto::OpenClawResponse>::success(response);

    } catch (const std::exception& e) {
        LOG_ERROR("解析 OpenClaw 响应失败: {}", e.what());
        return Utils::Result<dto::OpenClawResponse>::error("PARSE_ERROR",
            std::string("解析响应失败: ") + e.what());
    }
}

// ==================== JSON 解析（主路径） ====================

std::optional<dto::OpenClawResponse> OpenClawGateway::parseJsonResponse(const std::string& raw) {
    try {
        // 先尝试直接解析整个字符串为 JSON
        json j = json::parse(raw);

        if (!j.is_object()) return std::nullopt;
        if (!j.contains("text")) return std::nullopt;

        dto::OpenClawResponse resp;
        resp.text = j.value("text", "");

        if (j.contains("emotion")) {
            std::string emo = j["emotion"].get<std::string>();
            if (!emo.empty()) resp.emotions.push_back(emo);
        }
        if (j.contains("action")) {
            std::string act = j["action"].get<std::string>();
            if (!act.empty()) resp.actions.push_back(act);
        }
        if (j.contains("translation")) {
            resp.translatedText = j["translation"].get<std::string>();
        }
        if (j.contains("moderation")) {
            resp.moderation = j["moderation"].get<std::string>();
        }

        return resp;
    } catch (const json::exception&) {
        // 直接解析失败，尝试从文本中提取 JSON 块（带 markdown 代码块或其他包裹）
        try {
            auto jsonStart = raw.find('{');
            auto jsonEnd = raw.rfind('}');
            if (jsonStart == std::string::npos || jsonEnd == std::string::npos) return std::nullopt;

            std::string jsonStr = raw.substr(jsonStart, jsonEnd - jsonStart + 1);
            json j = json::parse(jsonStr);

            if (!j.is_object() || !j.contains("text")) return std::nullopt;

            dto::OpenClawResponse resp;
            resp.text = j.value("text", "");
            if (j.contains("emotion")) {
                std::string emo = j["emotion"].get<std::string>();
                if (!emo.empty()) resp.emotions.push_back(emo);
            }
            if (j.contains("action")) {
                std::string act = j["action"].get<std::string>();
                if (!act.empty()) resp.actions.push_back(act);
            }
            if (j.contains("translation")) {
                resp.translatedText = j["translation"].get<std::string>();
            }
            if (j.contains("moderation")) {
                resp.moderation = j["moderation"].get<std::string>();
            }
            return resp;
        } catch (const json::exception&) {
            return std::nullopt;
        }
    }
}

// ==================== 标签解析（Fallback 路径） ====================

dto::OpenClawResponse OpenClawGateway::parseTaggedResponse(const std::string& raw) {
    dto::OpenClawResponse response;

    // [emotion:xxx]
    static std::regex emotionRe(R"(\[emotion:([^\]]+)\])", std::regex::icase);
    std::smatch match;
    if (std::regex_search(raw, match, emotionRe)) {
        std::string emotion = match[1].str();
        emotion.erase(0, emotion.find_first_not_of(" \t"));
        emotion.erase(emotion.find_last_not_of(" \t") + 1);
        response.emotions.push_back(emotion);
    }

    // [action:xxx]
    static std::regex actionRe(R"(\[action:([^\]]+)\])", std::regex::icase);
    if (std::regex_search(raw, match, actionRe)) {
        std::string action = match[1].str();
        action.erase(0, action.find_first_not_of(" \t"));
        action.erase(action.find_last_not_of(" \t") + 1);
        response.actions.push_back(action);
    }

    // 正文 + [translated]
    std::string body = raw;
    body = std::regex_replace(body, std::regex(R"(\[emotion:[^\]]*\]\s*)"), "");
    body = std::regex_replace(body, std::regex(R"(\[action:[^\]]*\]\s*)"), "");

    static std::regex translatedRe(R"(\[translated\]\s*)", std::regex::icase);
    std::string translatedText;
    if (std::regex_search(body, match, translatedRe)) {
        std::string prefix = match.prefix().str();
        translatedText = match.suffix().str();
        body = prefix;
    }

    body.erase(0, body.find_first_not_of(" \t\n\r"));
    body.erase(body.find_last_not_of(" \t\n\r") + 1);
    response.text = body;

    translatedText.erase(0, translatedText.find_first_not_of(" \t\n\r"));
    translatedText.erase(translatedText.find_last_not_of(" \t\n\r") + 1);
    response.translatedText = translatedText;

    return response;
}

// ==================== 发送到 OpenClaw API ====================

Utils::Result<json> OpenClawGateway::sendToOpenClaw(const std::string& userMessage) {
    LOG_DEBUG("发送请求到 OpenClaw: {}/v1/chat/completions", openclaw_endpoint_);

    json requestBody;
    requestBody["model"] = model_;  // OpenClaw Gateway 路由 key (如 openclaw/yachiyo 或 deepseek/deepseek-v4-flash)
    requestBody["messages"] = json::array();
    requestBody["max_tokens"] = 500;
    requestBody["thinking"] = json::object({{"type", "disabled"}});

    // System Prompt
    if (!system_prompt_.empty()) {
        json sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = system_prompt_;
        requestBody["messages"].push_back(sysMsg);
    }

    // 用户消息
    json userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    requestBody["messages"].push_back(userMsg);

    std::string postData = requestBody.dump();

    CURL* curl = curl_easy_init();
    if (!curl) {
        return Utils::Result<json>::error("CURL_INIT_ERROR", "CURL 初始化失败");
    }

    std::string readBuffer;
    std::string chatUrl = openclaw_endpoint_ + "/v1/chat/completions";

    curl_easy_setopt(curl, CURLOPT_URL, chatUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_secs_));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!auth_token_.empty()) {
        std::string authHeader = "Authorization: Bearer " + auth_token_;
        headers = curl_slist_append(headers, authHeader.c_str());
    }
    if (!model_.empty() && model_.find("openclaw/") != 0) {
        // x-openclaw-model header 只用于纯模型名(如 deepseek/deepseek-v4-flash)
        // openclaw/yachiyo 这类 agent ID 不需要此 header
        std::string modelHeader = "x-openclaw-model: " + model_;
        headers = curl_slist_append(headers, modelHeader.c_str());
    }
    // 每次请求使用独立 session，防止 AI 保留跨对话记忆
    std::string sessionId = "yachiyo_" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::string sessionHeader = "x-openclaw-session: " + sessionId;
    headers = curl_slist_append(headers, sessionHeader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("OpenClaw API 请求失败: {}", curl_easy_strerror(res));
        return Utils::Result<json>::error("OPENCLAW_ERROR",
            std::string("OpenClaw 不可达: ") + curl_easy_strerror(res));
    }

    if (http_code >= 500) {
        LOG_ERROR("OpenClaw 服务端错误: HTTP {}", http_code);
        return Utils::Result<json>::error("OPENCLAW_SERVER_ERROR",
            "OpenClaw 返回 HTTP " + std::to_string(http_code));
    }

    if (http_code >= 400) {
        LOG_ERROR("OpenClaw 请求错误: HTTP {}", http_code);
        return Utils::Result<json>::error("OPENCLAW_CLIENT_ERROR",
            "OpenClaw 返回 HTTP " + std::to_string(http_code));
    }

    try {
        json response = json::parse(readBuffer);
        if (response.contains("error")) {
            std::string errMsg = response["error"].value("message", "未知错误");
            LOG_ERROR("OpenClaw API 错误: {}", errMsg);
            return Utils::Result<json>::error("OPENCLAW_API_ERROR", errMsg);
        }
        return Utils::Result<json>::success(response);
    } catch (const std::exception& e) {
        LOG_ERROR("JSON 解析失败: {}", e.what());
        return Utils::Result<json>::error("JSON_ERROR", e.what());
    }
}

// ==================== 重试 + 熔断 ====================

Utils::Result<json> OpenClawGateway::sendWithRetry(const std::string& userMessage) {
    // 熔断检查
    if (isCircuitOpen()) {
        LOG_WARN("熔断器打开，快速失败");
        return Utils::Result<json>::error("CIRCUIT_OPEN",
            "OpenClaw 服务暂时不可用（熔断中），请稍后重试");
    }

    Utils::Result<json> lastError = Utils::Result<json>::error("UNKNOWN", "");

    for (int attempt = 0; attempt <= retry_config_.maxRetries; ++attempt) {
        if (attempt > 0) {
            int delayMs = retry_config_.backoffMs[std::min(
                attempt - 1, static_cast<int>(retry_config_.backoffMs.size()) - 1)];
            LOG_INFO("重试 {}/{} — 等待 {}ms...", attempt, retry_config_.maxRetries, delayMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }

        auto result = sendToOpenClaw(userMessage);

        if (result.isSuccess()) {
            recordResult(true);
            return result;
        }

        lastError = result;
        const std::string& code = result.getCode();

        // 客户端错误 (4xx) — 不重试
        if (code.find("CLIENT_ERROR") != std::string::npos
            || code.find("API_ERROR") != std::string::npos) {
            LOG_WARN("客户端错误 ({}), 不重试", code);
            recordResult(false);
            return result;
        }
    }

    recordResult(false);
    LOG_ERROR("所有重试均已失败 ({}/{} 次)", retry_config_.maxRetries, retry_config_.maxRetries);
    return lastError;
}

bool OpenClawGateway::isCircuitOpen() {
    if (consecutive_failures_ < cb_config_.failureThreshold) return false;
    auto now = std::chrono::steady_clock::now();
    return now < circuit_open_until_;
}

void OpenClawGateway::recordResult(bool success) {
    if (success) {
        consecutive_failures_ = 0;
        circuit_open_until_ = {};
    } else {
        int prev = consecutive_failures_.fetch_add(1);
        if (prev + 1 >= cb_config_.failureThreshold) {
            circuit_open_until_ = std::chrono::steady_clock::now()
                + std::chrono::seconds(cb_config_.cooldownSeconds);
            LOG_WARN("熔断器触发! 连续 {} 次失败, 冷却 {}s",
                     prev + 1, cb_config_.cooldownSeconds);
        }
    }
}

// ==================== 缓存管理 ====================

std::shared_ptr<dto::OpenClawResponse> OpenClawGateway::getCachedResponse(
    const std::string& cacheKey) {
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

void OpenClawGateway::cleanupExpiredCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto now = std::chrono::steady_clock::now();
    for (auto it = cache_.begin(); it != cache_.end();) {
        auto ct = std::chrono::steady_clock::time_point(
            std::chrono::steady_clock::duration(it->second.timestamp));
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - ct).count();
        if (age > it->second.ttl) it = cache_.erase(it);
        else ++it;
    }
}

// ==================== 健康检查 ====================

bool OpenClawGateway::isHealthy() {
    // 使用熔断状态作为快速判断
    if (isCircuitOpen()) return false;

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, (openclaw_endpoint_ + "/v1/models").c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    bool healthy = res == CURLE_OK;
    recordResult(healthy);
    return healthy;
}

void OpenClawGateway::shutdown() {
    running_ = false;
}

std::string OpenClawGateway::generateCacheKey(const std::string& text) const {
    return "oc_" + std::to_string(std::hash<std::string>{}(text));
}

} // namespace yachiyo::services
