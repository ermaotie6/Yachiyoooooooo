#include "services/TranslationService.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <curl/curl.h>
#include <chrono>
#include <openssl/md5.h>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace yachiyo::services {

using json = nlohmann::json;

// ==================== 辅助函数 ====================

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// MD5 哈希（百度翻译签名需要）
static std::string md5(const std::string& input) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), digest);
    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return ss.str();
}

// URL 编码
static std::string urlEncode(const std::string& str) {
    CURL* curl = curl_easy_init();
    if (!curl) return str;
    char* encoded = curl_easy_escape(curl, str.c_str(), str.size());
    std::string result(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

// ==================== 构造/析构 ====================

TranslationService::TranslationService() {
    engine_priority_ = {Engine::BAIDU, Engine::DEEPSEEK};
}

TranslationService::~TranslationService() {
    clearCache();
}

// ==================== 初始化 ====================

bool TranslationService::initialize() {
    LOG_INFO("初始化翻译服务");
    
    // 配置百度翻译引擎
    engines_[Engine::BAIDU] = {
        "https://fanyi-api.baidu.com/api/trans/vip/translate",
        "",  // app_id:api_key 格式，从配置文件读取
        5,
        true
    };
    
    // 配置 DeepSeek 翻译引擎（备选）
    engines_[Engine::DEEPSEEK] = {
        "https://api.deepseek.com",
        "",  // 从配置文件读取
        5,
        true
    };
    
    LOG_INFO("翻译服务初始化完成");
    return true;
}

// ==================== 翻译 ====================

Utils::Result<dto::TranslationResponse> TranslationService::translate(
    const std::string& text,
    const std::string& sourceLanguage,
    const std::string& targetLanguage,
    Engine preferredEngine
) {
    auto startTime = std::chrono::steady_clock::now();
    
    // 生成缓存键
    std::string cacheKey = generateCacheKey(text, sourceLanguage, targetLanguage);
    
    // 检查缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(cacheKey);
        if (it != cache_.end()) {
            LOG_DEBUG("使用缓存的翻译");
            it->second.response.processingTimeMs = 
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime
                ).count();
            return Utils::Result<dto::TranslationResponse>::success(it->second.response);
        }
    }
    
    // 选择引擎
    std::vector<Engine> enginesInOrder;
    if (preferredEngine != Engine::AUTO) {
        enginesInOrder.push_back(preferredEngine);
        for (const auto& e : engine_priority_) {
            if (e != preferredEngine) enginesInOrder.push_back(e);
        }
    } else {
        enginesInOrder = engine_priority_;
    }
    
    // 尝试可用的引擎
    for (const auto& engine : enginesInOrder) {
        if (!engines_[engine].enabled) continue;
        
        Utils::Result<dto::TranslationResponse> result;
        
        switch (engine) {
            case Engine::BAIDU:
                result = translateViaBaidu(text, sourceLanguage, targetLanguage);
                break;
            case Engine::DEEPSEEK:
                result = translateViaDeepSeek(text, sourceLanguage, targetLanguage);
                break;
            default:
                continue;
        }
        
        if (result.isSuccess()) {
            auto response = result.getData().value();
            response.processingTimeMs = 
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime
                ).count();
            
            // 保存到缓存
            {
                std::lock_guard<std::mutex> lock(cache_mutex_);
                cache_[cacheKey] = {response, std::chrono::system_clock::now().time_since_epoch().count()};
            }
            
            return Utils::Result<dto::TranslationResponse>::success(response);
        }
    }
    
    // 所有引擎都失败，返回原文
    LOG_WARN("所有翻译引擎都失败，返回原文");
    dto::TranslationResponse fallback;
    fallback.translatedText = text;
    fallback.sourceLanguage = sourceLanguage;
    fallback.targetLanguage = targetLanguage;
    fallback.confidence = 0.0;
    fallback.qualityScore = 0.0;
    
    return Utils::Result<dto::TranslationResponse>::success(fallback);
}

// ==================== 批量翻译 ====================

std::vector<Utils::Result<dto::TranslationResponse>> TranslationService::batchTranslate(
    const std::vector<std::string>& texts,
    const std::string& sourceLanguage,
    const std::string& targetLanguage
) {
    std::vector<Utils::Result<dto::TranslationResponse>> results;
    
    for (const auto& text : texts) {
        results.push_back(translate(text, sourceLanguage, targetLanguage));
    }
    
    return results;
}

// ==================== 引擎优先级 ====================

void TranslationService::setEnginePriority(const std::vector<Engine>& priority) {
    engine_priority_ = priority;
}

// ==================== 缓存管理 ====================

void TranslationService::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
}

// ==================== 私有方法 ====================

std::string TranslationService::generateCacheKey(
    const std::string& text,
    const std::string& source,
    const std::string& target
) const {
    return "trans_" + std::to_string(std::hash<std::string>{}(text + "_" + source + "_" + target));
}

Utils::Result<dto::TranslationResponse> TranslationService::translateViaBaidu(
    const std::string& text,
    const std::string& sourceLanguage,
    const std::string& targetLanguage
) {
    LOG_DEBUG("通过百度翻译 API 翻译");
    
    auto& config = engines_[Engine::BAIDU];
    if (config.apiKey.empty()) {
        return Utils::Result<dto::TranslationResponse>::error("BAIDU_NO_KEY", "百度翻译未配置 app_id 或 api_key");
    }
    
    // apiKey 格式: "app_id:secret_key"
    auto sep = config.apiKey.find(':');
    if (sep == std::string::npos) {
        return Utils::Result<dto::TranslationResponse>::error("BAIDU_BAD_KEY", "百度翻译 apiKey 格式错误，需要 app_id:secret_key");
    }
    std::string appId = config.apiKey.substr(0, sep);
    std::string secretKey = config.apiKey.substr(sep + 1);
    
    // 语言代码映射（ISO 639-1 → 百度翻译语言代码）
    auto mapLang = [](const std::string& lang) -> std::string {
        if (lang == "zh" || lang == "zh-CN" || lang == "zh-Hans") return "zh";
        if (lang == "zh-TW" || lang == "zh-Hant") return "cht";
        if (lang == "en") return "en";
        if (lang == "ja") return "jp";
        if (lang == "ko") return "kor";
        if (lang == "fr") return "fra";
        if (lang == "de") return "de";
        if (lang == "ru") return "ru";
        if (lang == "es") return "spa";
        if (lang == "pt") return "pt";
        if (lang == "auto" || lang.empty()) return "auto";
        return lang;  // 其他语言直接传递
    };
    
    std::string from = mapLang(sourceLanguage);
    std::string to = mapLang(targetLanguage);
    
    // 生成签名: MD5(appid + q + salt + 密钥)
    std::string salt = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::string signStr = appId + text + salt + secretKey;
    std::string sign = md5(signStr);
    
    // 构造请求 URL
    std::string url = config.endpoint
        + "?q=" + urlEncode(text)
        + "&from=" + from
        + "&to=" + to
        + "&appid=" + appId
        + "&salt=" + salt
        + "&sign=" + sign;
    
    // 发送 HTTP GET 请求
    CURL* curl = curl_easy_init();
    if (!curl) {
        return Utils::Result<dto::TranslationResponse>::error("CURL_INIT_FAIL", "CURL 初始化失败");
    }
    
    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config.timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("百度翻译 HTTP 请求失败: {}", curl_easy_strerror(res));
        return Utils::Result<dto::TranslationResponse>::error("BAIDU_HTTP_ERROR", curl_easy_strerror(res));
    }
    
    // 解析响应 JSON
    // 成功格式: {"from":"en","to":"zh","trans_result":[{"src":"hello","dst":"你好"}]}
    // 错误格式: {"error_code":"54001","error_msg":"Invalid Sign"}
    try {
        auto j = json::parse(responseBody);
        
        if (j.contains("error_code")) {
            std::string errCode = j["error_code"].get<std::string>();
            std::string errMsg = j.value("error_msg", "未知错误");
            LOG_ERROR("百度翻译 API 错误: {} - {}", errCode, errMsg);
            return Utils::Result<dto::TranslationResponse>::error("BAIDU_API_" + errCode, errMsg);
        }
        
        if (!j.contains("trans_result") || j["trans_result"].empty()) {
            return Utils::Result<dto::TranslationResponse>::error("BAIDU_NO_RESULT", "百度翻译返回空结果");
        }
        
        // 合并所有翻译段落
        std::string translatedText;
        for (const auto& item : j["trans_result"]) {
            if (!translatedText.empty()) translatedText += "\n";
            translatedText += item["dst"].get<std::string>();
        }
        
        dto::TranslationResponse response;
        response.translatedText = translatedText;
        response.sourceLanguage = j.value("from", sourceLanguage);
        response.targetLanguage = j.value("to", targetLanguage);
        response.confidence = 0.90;
        response.qualityScore = 0.85;
        response.engineUsed = "baidu";
        response.cached = false;
        
        return Utils::Result<dto::TranslationResponse>::success(response);
    } catch (const json::exception& e) {
        LOG_ERROR("百度翻译响应解析失败: {}", e.what());
        return Utils::Result<dto::TranslationResponse>::error("BAIDU_PARSE_ERROR", e.what());
    }
}

Utils::Result<dto::TranslationResponse> TranslationService::translateViaDeepSeek(
    const std::string& text,
    const std::string& sourceLanguage,
    const std::string& targetLanguage
) {
    LOG_DEBUG("通过 DeepSeek 翻译");
    
    auto& config = engines_[Engine::DEEPSEEK];
    if (config.apiKey.empty()) {
        return Utils::Result<dto::TranslationResponse>::error("DEEPSEEK_NO_KEY", "DeepSeek API Key 未配置");
    }
    
    // 构造翻译 prompt
    std::string srcLang = sourceLanguage.empty() || sourceLanguage == "auto" ? "自动检测" : sourceLanguage;
    std::string prompt = "你是一个专业的翻译引擎。请将以下文本从 " + srcLang 
        + " 翻译成 " + targetLanguage 
        + "。只输出翻译结果，不要添加解释或其他内容。\n\n" + text;
    
    // 构造 DeepSeek Chat Completion 请求
    json requestBody = {
        {"model", "deepseek-chat"},
        {"messages", json::array({
            {{"role", "system"}, {"content", "你是一个精确的翻译引擎，只返回翻译后的文本，不添加任何额外说明。"}},
            {{"role", "user"}, {"content", prompt}}
        })},
        {"temperature", 0.1},
        {"max_tokens", 2048}
    };
    std::string body = requestBody.dump();
    
    // 发送 HTTP POST 请求
    CURL* curl = curl_easy_init();
    if (!curl) {
        return Utils::Result<dto::TranslationResponse>::error("CURL_INIT_FAIL", "CURL 初始化失败");
    }
    
    std::string responseBody;
    std::string url = config.endpoint + "/v1/chat/completions";
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "Authorization: Bearer " + config.apiKey;
    headers = curl_slist_append(headers, authHeader.c_str());
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config.timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("DeepSeek 翻译 HTTP 请求失败: {}", curl_easy_strerror(res));
        return Utils::Result<dto::TranslationResponse>::error("DEEPSEEK_HTTP_ERROR", curl_easy_strerror(res));
    }
    
    // 解析 OpenAI 兼容格式的响应
    try {
        auto j = json::parse(responseBody);
        
        if (j.contains("error")) {
            std::string errMsg = j["error"].value("message", "未知错误");
            LOG_ERROR("DeepSeek 翻译 API 错误: {}", errMsg);
            return Utils::Result<dto::TranslationResponse>::error("DEEPSEEK_API_ERROR", errMsg);
        }
        
        if (!j.contains("choices") || j["choices"].empty()) {
            return Utils::Result<dto::TranslationResponse>::error("DEEPSEEK_NO_RESULT", "DeepSeek 返回空结果");
        }
        
        std::string translatedText = j["choices"][0]["message"]["content"].get<std::string>();
        
        // 去除首尾空白
        auto trimStart = translatedText.find_first_not_of(" \t\n\r");
        auto trimEnd = translatedText.find_last_not_of(" \t\n\r");
        if (trimStart != std::string::npos) {
            translatedText = translatedText.substr(trimStart, trimEnd - trimStart + 1);
        }
        
        dto::TranslationResponse response;
        response.translatedText = translatedText;
        response.sourceLanguage = sourceLanguage;
        response.targetLanguage = targetLanguage;
        response.confidence = 0.92;
        response.qualityScore = 0.90;
        response.engineUsed = "deepseek";
        response.cached = false;
        
        return Utils::Result<dto::TranslationResponse>::success(response);
    } catch (const json::exception& e) {
        LOG_ERROR("DeepSeek 翻译响应解析失败: {}", e.what());
        return Utils::Result<dto::TranslationResponse>::error("DEEPSEEK_PARSE_ERROR", e.what());
    }
}

std::vector<TranslationService::Engine> TranslationService::getAvailableEngines() const {
    std::vector<Engine> available;
    for (const auto& [engine, config] : engines_) {
        if (config.enabled) {
            available.push_back(engine);
        }
    }
    return available;
}

} // namespace yachiyo::services
