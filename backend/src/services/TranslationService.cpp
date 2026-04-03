#include "services/TranslationService.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <curl/curl.h>
#include <chrono>

namespace yachiyo::services {

// ==================== 辅助函数 ====================

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ==================== 构造/析构 ====================

TranslationService::TranslationService() {
    engine_priority_ = {Engine::DEEPSEEK, Engine::OPENAI, Engine::GOOGLE};
}

TranslationService::~TranslationService() {
    clearCache();
}

// ==================== 初始化 ====================

bool TranslationService::initialize() {
    LOG_INFO("初始化翻译服务");
    
    // 配置各引擎
    engines_[Engine::DEEPSEEK] = {
        "http://localhost:11434",  // endpoint
        "",                        // apiKey
        3,                         // timeout
        true                       // enabled
    };
    
    engines_[Engine::OPENAI] = {
        "https://api.openai.com/v1",
        "",  // 应从配置文件读取
        5,
        false
    };
    
    engines_[Engine::GOOGLE] = {
        "https://translation.googleapis.com",
        "",  // 应从配置文件读取
        5,
        false
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
            case Engine::DEEPSEEK:
                result = translateViaDeepSeek(text, sourceLanguage, targetLanguage);
                break;
            case Engine::OPENAI:
                result = translateViaOpenAI(text, sourceLanguage, targetLanguage);
                break;
            case Engine::GOOGLE:
                result = translateViaGoogle(text, sourceLanguage, targetLanguage);
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

Utils::Result<dto::TranslationResponse> TranslationService::translateViaDeepSeek(
    const std::string& text,
    const std::string& sourceLanguage,
    const std::string& targetLanguage
) {
    LOG_DEBUG("通过 DeepSeek 翻译");
    
    // TODO: 实现 Ollama API 调用
    // 当前返回 mock 结果
    dto::TranslationResponse response;
    response.translatedText = text;  // Mock: 返回原文
    response.sourceLanguage = sourceLanguage;
    response.targetLanguage = targetLanguage;
    response.confidence = 0.9;
    response.qualityScore = 0.85;
    response.engineUsed = "deepseek";
    response.cached = false;
    
    return Utils::Result<dto::TranslationResponse>::success(response);
}

Utils::Result<dto::TranslationResponse> TranslationService::translateViaOpenAI(
    const std::string& text,
    const std::string& sourceLanguage,
    const std::string& targetLanguage
) {
    LOG_DEBUG("通过 OpenAI 翻译");
    
    // TODO: 实现 OpenAI API 调用
    return Utils::Result<dto::TranslationResponse>::error("OPENAI_DISABLED", "OpenAI 引擎未启用");
}

Utils::Result<dto::TranslationResponse> TranslationService::translateViaGoogle(
    const std::string& text,
    const std::string& sourceLanguage,
    const std::string& targetLanguage
) {
    LOG_DEBUG("通过 Google 翻译");
    
    // TODO: 实现 Google 翻译 API 调用
    return Utils::Result<dto::TranslationResponse>::error("GOOGLE_DISABLED", "Google 引擎未启用");
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
