#include "../include/utils/OpencalwClient.hpp"
#include "../include/utils/Logger.hpp"
#include <curl/curl.h>
#include <cmath>
#include <sstream>

namespace yachiyo::utils {

// ==================== 辅助函数 ====================

// CURL写入回调
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ==================== 构造/析构 ====================

OpencalwClient::OpencalwClient(const std::string& apiKey, const std::string& apiUrl)
    : apiKey_(apiKey), apiUrl_(apiUrl) {
    LOG_INFO("初始化OpencalwClient: " + apiUrl);
}

// ==================== 单个内容审查 ====================

Result<OpencalwClient::ReviewResult> OpencalwClient::reviewContent(
    const std::string& message,
    int64_t userId
) {
    try {
        if (message.empty()) {
            return Result<ReviewResult>::Error("消息内容为空");
        }
        
        // 构建请求体
        json payload;
        payload["text"] = message;
        payload["userId"] = userId;
        payload["timestamp"] = std::time(nullptr);
        
        // 发送请求
        auto result = sendRequest("/api/review/text", payload);
        if (!result.isSuccess()) {
            return Result<ReviewResult>::Error(result.getErrorMsg());
        }
        
        // 解析响应
        auto reviewResult = parseResponse(result.getData());
        
        LOG_INFO("内容审查完成: userId=" + std::to_string(userId) + 
                 ", allowed=" + (reviewResult.isAllowed ? "true" : "false") +
                 ", score=" + std::to_string(reviewResult.riskScore));
        
        return Result<ReviewResult>::Success(reviewResult);
        
    } catch (const std::exception& e) {
        LOG_ERROR("内容审查异常: " + std::string(e.what()));
        return Result<ReviewResult>::Error(std::string(e.what()));
    }
}

// ==================== 批量审查 ====================

Result<std::vector<OpencalwClient::ReviewResult>> OpencalwClient::batchReview(
    const std::vector<std::pair<int64_t, std::string>>& messages
) {
    try {
        std::vector<ReviewResult> results;
        
        if (messages.empty()) {
            return Result<std::vector<ReviewResult>>::Success(results);
        }
        
        // 构建批量请求
        json payload;
        payload["batch"] = json::array();
        
        for (const auto& [userId, message] : messages) {
            json item;
            item["text"] = message;
            item["userId"] = userId;
            payload["batch"].push_back(item);
        }
        
        // 发送请求
        auto result = sendRequest("/api/review/batch", payload);
        if (!result.isSuccess()) {
            return Result<std::vector<ReviewResult>>::Error(result.getErrorMsg());
        }
        
        // 解析响应
        auto response = result.getData();
        if (response.contains("results") && response["results"].is_array()) {
            for (const auto& item : response["results"]) {
                results.push_back(parseResponse(item));
            }
        }
        
        LOG_INFO("批量内容审查完成: 共" + std::to_string(results.size()) + "条");
        
        return Result<std::vector<ReviewResult>>::Success(results);
        
    } catch (const std::exception& e) {
        LOG_ERROR("批量审查异常: " + std::string(e.what()));
        return Result<std::vector<ReviewResult>>::Error(std::string(e.what()));
    }
}

// ==================== 健康检查 ====================

Result<bool> OpencalwClient::healthCheck() {
    try {
        auto result = sendRequest("/api/health", json());
        if (!result.isSuccess()) {
            LOG_ERROR("openclaw健康检查失败");
            return Result<bool>::Error("API不可用");
        }
        
        LOG_INFO("openclaw健康检查通过");
        return Result<bool>::Success(true);
        
    } catch (const std::exception& e) {
        LOG_ERROR("健康检查异常: " + std::string(e.what()));
        return Result<bool>::Error(std::string(e.what()));
    }
}

// ==================== 获取限流信息 ====================

Result<json> OpencalwClient::getRateLimit() {
    try {
        auto result = sendRequest("/api/ratelimit", json());
        if (!result.isSuccess()) {
            return Result<json>::Error(result.getErrorMsg());
        }
        
        return Result<json>::Success(result.getData());
        
    } catch (const std::exception& e) {
        LOG_ERROR("获取限流信息异常: " + std::string(e.what()));
        return Result<json>::Error(std::string(e.what()));
    }
}

// ==================== HTTP请求 ====================

Result<json> OpencalwClient::sendRequest(const std::string& endpoint, const json& payload) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return Result<json>::Error("CURL初始化失败");
    }
    
    std::string url = apiUrl_ + endpoint;
    std::string readBuffer;
    
    try {
        // 设置请求头
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey_).c_str());
        
        // 发送请求 (带重试)
        json responseJson;
        int retries = 0;
        bool success = false;
        
        while (retries < MAX_RETRIES && !success) {
            readBuffer.clear();
            
            std::string payloadStr = payload.dump();
            
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT_SECONDS);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            
            CURLcode res = curl_easy_perform(curl);
            
            if (res != CURLE_OK) {
                LOG_WARN("CURL请求失败 (重试 " + std::to_string(retries + 1) + "/" + 
                        std::to_string(MAX_RETRIES) + "): " + curl_easy_strerror(res));
                retries++;
                continue;
            }
            
            // 检查HTTP状态码
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            
            if (httpCode == 200 || httpCode == 201) {
                try {
                    responseJson = json::parse(readBuffer);
                    success = true;
                } catch (const json::parse_error& e) {
                    LOG_ERROR("JSON解析失败: " + std::string(e.what()));
                    retries++;
                }
            } else {
                LOG_WARN("HTTP错误码: " + std::to_string(httpCode));
                retries++;
            }
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (!success) {
            return Result<json>::Error("请求失败，已重试" + std::to_string(MAX_RETRIES) + "次");
        }
        
        return Result<json>::Success(responseJson);
        
    } catch (const std::exception& e) {
        curl_easy_cleanup(curl);
        LOG_ERROR("HTTP请求异常: " + std::string(e.what()));
        return Result<json>::Error(std::string(e.what()));
    }
}

// ==================== 响应解析 ====================

OpencalwClient::ReviewResult OpencalwClient::parseResponse(const json& response) {
    ReviewResult result;
    result.rawResponse = response.dump();
    
    try {
        // 提取核心字段
        result.isAllowed = response.value("allowed", true);
        result.riskScore = response.value("score", 0.0);
        result.mainCategory = response.value("mainCategory", "");
        
        // 提取所有违规类别
        if (response.contains("categories") && response["categories"].is_array()) {
            for (const auto& cat : response["categories"]) {
                result.categories.push_back(cat.get<std::string>());
            }
        }
        
        // 如果未指定isAllowed，根据风险评分推断
        if (!response.contains("allowed")) {
            result.isAllowed = (result.riskScore < 0.5);
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("响应解析异常: " + std::string(e.what()));
        result.isAllowed = true;
        result.riskScore = 0.0;
    }
    
    return result;
}

// ==================== 风险评分计算 ====================

double OpencalwClient::calculateRiskScore(const std::vector<std::string>& categories) {
    if (categories.empty()) {
        return 0.0;
    }
    
    // 类别权重映射
    std::map<std::string, double> weights = {
        {"spam", 0.3},           // 垃圾内容
        {"abuse", 0.9},          // 辱骂
        {"violence", 0.95},      // 暴力
        {"adult", 0.8},          // 成人内容
        {"hatred", 0.9},         // 仇恨言论
        {"politics", 0.5},       // 敏感政治内容
        {"fraud", 0.8},          // 欺诈
        {"other", 0.2}           // 其他
    };
    
    double maxScore = 0.0;
    for (const auto& cat : categories) {
        auto it = weights.find(cat);
        double weight = (it != weights.end()) ? it->second : 0.5;
        maxScore = std::max(maxScore, weight);
    }
    
    return std::min(1.0, maxScore);
}

} // namespace yachiyo::utils
