#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include "utils/Result.hpp"
#include "utils/Compat.hpp"
#include "dto/TranslationDTO.hpp"

namespace yachiyo::services {

/**
 * 翻译服务 - 支持多语言翻译
 * 
 * 职责:
 * - 支持多个翻译引擎 (DeepSeek, OpenAI, Google)
 * - 根据引擎可用性自动降级
 * - 管理翻译缓存
 * - 支持批量翻译
 */
class TranslationService {
public:
    enum class Engine {
        BAIDU,      // 百度翻译 API
        DEEPSEEK,   // DeepSeek API
        AUTO        // 自动选择
    };
    
    TranslationService();
    ~TranslationService();
    
    /**
     * 初始化服务 (配置各个引擎)
     */
    bool initialize();
    
    /**
     * 翻译文本
     * @param text 要翻译的文本
     * @param sourceLanguage 源语言代码 (zh, ja, en, ko)
     * @param targetLanguage 目标语言代码
     * @param preferredEngine 优选引擎 (默认为自动)
     * @return 翻译结果
     */
    Utils::Result<dto::TranslationResponse> translate(
        const std::string& text,
        const std::string& sourceLanguage,
        const std::string& targetLanguage,
        Engine preferredEngine = Engine::AUTO
    );
    
    /**
     * 批量翻译
     * @param texts 文本列表
     * @param sourceLanguage 源语言
     * @param targetLanguage 目标语言
     * @return 翻译结果列表
     */
    std::vector<Utils::Result<dto::TranslationResponse>> batchTranslate(
        const std::vector<std::string>& texts,
        const std::string& sourceLanguage,
        const std::string& targetLanguage
    );
    
    /**
     * 配置翻译引擎的 API Key 和端点
     * @param engine 引擎类型
     * @param apiKey API 密钥
     * @param endpoint 端点 URL (可选，为空则保留默认值)
     */
    void configureEngine(Engine engine, const std::string& apiKey, const std::string& endpoint = "");

    /**
     * 设置各引擎的优先级
     */
    void setEnginePriority(const std::vector<Engine>& priority);
    
    /**
     * 清空缓存
     */
    void clearCache();

private:
    struct CacheEntry {
        dto::TranslationResponse response;
        int64_t timestamp;
    };
    
    struct EngineConfig {
        std::string endpoint;
        std::string apiKey;
        int timeoutSeconds;
        bool enabled;
    };
    
    std::map<Engine, EngineConfig> engines_;
    std::map<std::string, CacheEntry> cache_;
    std::mutex cache_mutex_;
    std::vector<Engine> engine_priority_;
    
    /**
     * 生成缓存键
     */
    std::string generateCacheKey(
        const std::string& text,
        const std::string& source,
        const std::string& target
    ) const;
    
    /**
     * 通过百度翻译 API 翻译
     */
    Utils::Result<dto::TranslationResponse> translateViaBaidu(
        const std::string& text,
        const std::string& sourceLanguage,
        const std::string& targetLanguage
    );
    
    /**
     * 通过 DeepSeek API 翻译
     */
    Utils::Result<dto::TranslationResponse> translateViaDeepSeek(
        const std::string& text,
        const std::string& sourceLanguage,
        const std::string& targetLanguage
    );
    
    /**
     * 获取可用的引擎列表
     */
    std::vector<Engine> getAvailableEngines() const;
};

} // namespace yachiyo::services
