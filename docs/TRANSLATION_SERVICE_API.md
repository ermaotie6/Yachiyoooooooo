# 🌐 翻译服务接口文档

**版本**: 1.0  
**更新日期**: 2026年4月3日  
**用途**: Yachiyo 虚拱形象直播平台的多语言翻译服务

---

## 📋 目录

1. [概述](#概述)
2. [支持语言](#支持语言)
3. [翻译引擎](#翻译引擎)
4. [请求格式](#请求格式)
5. [响应格式](#响应格式)
6. [集成示例](#集成示例)
7. [缓存与性能](#缓存与性能)

---

## 概述

### 翻译职责范围

✅ **OpenClaw 输出翻译** - 中文 → 日文/英文/韩文  
✅ **多语言支持** - 支持 6+ 种语言  
✅ **上下文感知** - 根据虚拱身份/平台调整翻译风格  
✅ **表情保留** - 翻译时保留情感表达  
✅ **缓存优化** - 避免重复翻译  

### 为什么独立于 OpenClaw?

OpenClaw 职责：处理文本 → 生成表情和动作
翻译职责：将文本转换为其他语言

**分离优点**:
- 🔄 可并行处理 (OpenClaw + 翻译同时执行)
- ⚡ 翻译缓存更有效
- 🔧 易于切换翻译引擎
- 📈 性能更好

---

## 支持语言

### 核心语言

| 代码 | 语言 | 优先级 | 说明 |
|------|------|--------|------|
| `zh` | 中文 | ⭐⭐⭐ | 源语言 |
| `ja` | 日文 | ⭐⭐⭐ | 虚拱原设定语言 |
| `en` | 英文 | ⭐⭐ | 国际用户 |
| `ko` | 韩文 | ⭐⭐ | 亚洲用户 |
| `es` | 西班牙语 | ⭐ | 可选 |
| `fr` | 法文 | ⭐ | 可选 |

### 语言代码对应

```json
{
  "zh-CN": "简体中文",
  "zh-TW": "繁体中文",
  "ja": "日文",
  "en": "英文",
  "en-US": "美式英文",
  "en-GB": "英式英文",
  "ko": "韩文",
  "es": "西班牙语",
  "fr": "法文"
}
```

---

## 翻译引擎

### 🔵 推荐配置 (多引擎优先级)

```
优先级1: DeepSeek (本地 Ollama - 推荐)
  ├─ 速度: ⚡⚡⚡ (本地推理 <200ms)
  ├─ 成本: 💰 (免费)
  ├─ 质量: ⭐⭐⭐ (7/10)
  └─ 配置: http://localhost:11434

优先级2: OpenAI (云端)
  ├─ 速度: ⚡⚡ (网络延迟 200-500ms)
  ├─ 成本: 💰💰💰 (每1000字约 $0.02)
  ├─ 质量: ⭐⭐⭐⭐⭐ (9/10)
  └─ 配置: API Key 方式

优先级3: Google 翻译 API
  ├─ 速度: ⚡⚡ (网络延迟)
  ├─ 成本: 💰💰 ($15/月起)
  ├─ 质量: ⭐⭐⭐⭐ (8/10)
  └─ 配置: API Key 方式

优先级4: 本地缓存
  ├─ 速度: ⚡⚡⚡⚡⚡ (<10ms)
  ├─ 成本: 💰 (免费)
  ├─ 质量: ⭐⭐ (预翻译内容)
  └─ 配置: SQLite/Redis
```

---

## 请求格式

### 🔵 翻译请求 JSON

```json
{
  "request_id": "trans_20260403_001",
  "source_language": "zh",
  "target_language": "ja",
  "content": {
    "text": "你好，我很开心！希望你今天过得愉快。",
    "context": {
      "speaker": "avatar",
      "tone": "friendly",
      "emotion_tags": ["开心", "友善"]
    }
  },
  "settings": {
    "preserve_formatting": true,
    "preserve_emotions": true,
    "style": "casual",
    "cache_enabled": true
  }
}
```

### 字段说明

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `request_id` | string | ✅ | 请求ID (追踪) |
| `source_language` | string | ✅ | 源语言代码 |
| `target_language` | string | ✅ | 目标语言代码 |
| `content` | object | ✅ | 要翻译的内容 |
| `settings` | object | ❌ | 翻译配置 |

### content 对象详解

```json
{
  "text": "要翻译的文本内容",
  "context": {
    "speaker": "avatar|user",                   // 说话者身份
    "tone": "friendly|professional|playful",    // 语气
    "emotion_tags": ["表情1", "表情2"],          // 情感标签
    "previous_translation": {                   // 前文翻译 (可选)
      "zh": "前面的中文",
      "ja": "前面的日文"
    }
  }
}
```

### settings 对象详解

```json
{
  "preserve_formatting": true,    // 保留格式 (标点/换行)
  "preserve_emotions": true,      // 保留情感表达
  "style": "casual|formal",       // 风格
  "cache_enabled": true,          // 使用缓存
  "quality": "fast|balanced|quality",  // 质量/速度权衡
  "max_wait_ms": 3000            // 最长等待时间
}
```

---

## 响应格式

### 🟢 翻译响应 JSON

```json
{
  "request_id": "trans_20260403_001",
  "status": "success",
  "data": {
    "source_text": "你好，我很开心！希望你今天过得愉快。",
    "translated_text": "こんにちは。私はとても幸せです。今日が素敵な一日になることを願っています。",
    "source_language": "zh",
    "target_language": "ja",
    "confidence": 0.94,
    "quality_score": 0.92,
    "cached": false,
    "engine_used": "deepseek",
    "processing_time_ms": 187,
    "alternative_translations": [
      {
        "text": "こんにちは。私はすごく幸せです。あなたの今日が素晴らしい一日になることを祈っています。",
        "confidence": 0.89,
        "style": "formal"
      }
    ],
    "preserved_emotions": ["开心"]
  },
  "error": null
}
```

### 响应字段说明

| 字段 | 说明 |
|------|------|
| `request_id` | 原始请求ID |
| `status` | success / error |
| `source_text` | 原始文本 |
| `translated_text` | 翻译后的文本 |
| `confidence` | 翻译置信度 (0.0-1.0) |
| `quality_score` | 翻译质量分数 (0.0-1.0) |
| `cached` | 是否来自缓存 |
| `engine_used` | 使用的翻译引擎 |
| `processing_time_ms` | 处理耗时 |
| `alternative_translations` | 其他翻译选项 |

---

## 集成示例

### C++ 后端实现

```cpp
// backend/include/services/TranslationService.hpp

#include <nlohmann/json.hpp>
#include <optional>
#include <map>

using json = nlohmann::json;

class TranslationService {
public:
    struct TranslationResult {
        bool success;
        std::string translatedText;
        float confidence;
        std::string engine;
        bool fromCache;
        int processingTimeMs;
        std::vector<std::string> preservedEmotions;
    };
    
    /**
     * 翻译文本
     */
    TranslationResult translate(
        const std::string& text,
        const std::string& sourceLanguage,
        const std::string& targetLanguage,
        const std::map<std::string, std::string>& context = {}
    ) {
        // 1. 检查缓存
        auto cached = getFromCache(text, sourceLanguage, targetLanguage);
        if (cached) {
            return cached.value();
        }
        
        // 2. 尝试按优先级调用引擎
        TranslationResult result;
        
        // 优先级1: 本地 DeepSeek
        if (canUseDeepSeek()) {
            result = translateViaDeepSeek(text, sourceLanguage, targetLanguage, context);
            if (result.success) {
                result.engine = "deepseek";
                saveToCache(text, sourceLanguage, targetLanguage, result);
                return result;
            }
        }
        
        // 优先级2: OpenAI
        if (canUseOpenAI()) {
            result = translateViaOpenAI(text, sourceLanguage, targetLanguage, context);
            if (result.success) {
                result.engine = "openai";
                saveToCache(text, sourceLanguage, targetLanguage, result);
                return result;
            }
        }
        
        // 优先级3: Google 翻译
        if (canUseGoogle()) {
            result = translateViaGoogle(text, sourceLanguage, targetLanguage);
            if (result.success) {
                result.engine = "google";
                saveToCache(text, sourceLanguage, targetLanguage, result);
                return result;
            }
        }
        
        // 失败处理
        return {
            .success = false,
            .translatedText = text,  // 返回原文
            .confidence = 0.0,
            .engine = "none",
            .fromCache = false
        };
    }
    
    /**
     * 批量翻译
     */
    std::vector<TranslationResult> batchTranslate(
        const std::vector<std::string>& texts,
        const std::string& sourceLanguage,
        const std::string& targetLanguage
    ) {
        std::vector<std::future<TranslationResult>> futures;
        
        for (const auto& text : texts) {
            futures.push_back(
                std::async(std::launch::async, [this, text, sourceLanguage, targetLanguage]() {
                    return translate(text, sourceLanguage, targetLanguage);
                })
            );
        }
        
        std::vector<TranslationResult> results;
        for (auto& f : futures) {
            results.push_back(f.get());
        }
        
        return results;
    }

private:
    TranslationResult translateViaDeepSeek(
        const std::string& text,
        const std::string& sourceLanguage,
        const std::string& targetLanguage,
        const std::map<std::string, std::string>& context
    ) {
        // 构建 prompt
        std::string prompt = buildTranslationPrompt(text, sourceLanguage, targetLanguage, context);
        
        // 调用 Ollama DeepSeek
        auto response = callOllama(prompt, "deepseek-chat");
        
        // 解析响应
        return parseTranslationResponse(response, "deepseek");
    }
    
    TranslationResult translateViaOpenAI(
        const std::string& text,
        const std::string& sourceLanguage,
        const std::string& targetLanguage,
        const std::map<std::string, std::string>& context
    ) {
        // 使用 OpenAI API
        // ... (类似于 curl 调用)
    }
    
    std::string buildTranslationPrompt(
        const std::string& text,
        const std::string& sourceLang,
        const std::string& targetLang,
        const std::map<std::string, std::string>& context
    ) {
        std::string langNames[] = {"中文", "日文", "英文", "韩文"};
        
        std::stringstream ss;
        ss << "请将以下" << getLangName(sourceLang) << "文本翻译为" 
           << getLangName(targetLang) << "：\n";
        ss << "原文：" << text << "\n";
        ss << "要求：\n";
        ss << "1. 保留原文的情感和语气\n";
        ss << "2. 根据上下文调整风格\n";
        ss << "3. 只返回翻译结果，不要包含其他说明\n";
        
        if (!context.empty()) {
            ss << "\n上下文：\n";
            for (const auto& [key, value] : context) {
                ss << key << ": " << value << "\n";
            }
        }
        
        return ss.str();
    }
};
```

### 在虚拱响应中使用

```cpp
// 在 AvatarResponseService 中调用翻译服务

class AvatarResponseService {
private:
    std::shared_ptr<TranslationService> translationService;
    
public:
    void processOpenClawResponse(
        const OpenClawResponse& openClawResponse,
        const std::string& targetLanguage
    ) {
        // 1. OpenClaw 返回中文文本
        std::string originalText = openClawResponse.text;
        
        // 2. 获取用户的目标语言
        if (targetLanguage != "zh") {
            // 3. 翻译文本
            auto translationResult = translationService->translate(
                originalText,
                "zh",
                targetLanguage,
                {
                    {"tone", "friendly"},
                    {"emotion_tags", stringArrayToString(openClawResponse.emotions)}
                }
            );
            
            if (translationResult.success) {
                // 使用翻译后的文本
                responseText = translationResult.translatedText;
                LOG_DEBUG("翻译完成: {} → {} (置信度: {}%)", 
                         "zh", targetLanguage, 
                         (int)(translationResult.confidence * 100));
            } else {
                // 使用原文
                LOG_WARN("翻译失败，使用原文");
                responseText = originalText;
            }
        } else {
            responseText = originalText;
        }
        
        // 4. 后续处理 (语音合成等)
        synthesizeVoice(responseText, targetLanguage);
    }
};
```

---

## 缓存与性能

### 缓存 Key 生成

```cpp
std::string generateCacheKey(
    const std::string& text,
    const std::string& sourceLang,
    const std::string& targetLang
) {
    // 使用 MD5/SHA256 哈希
    std::string combined = text + "_" + sourceLang + "_" + targetLang;
    return hashMD5(combined);
}
```

### Redis 缓存示例

```cpp
// 保存到缓存
redis->set(
    "trans:" + cacheKey,
    translationResult.translatedText,
    3600  // TTL: 1 小时
);

// 读取缓存
auto cached = redis->get("trans:" + cacheKey);
if (cached) {
    return cached.value();
}
```

### 配置示例

```yaml
# backend/config/config.yaml

translation:
  enabled: true
  
  # 优先级列表 (按顺序尝试)
  engines:
    - name: "deepseek"
      type: "ollama"
      enabled: true
      endpoint: "http://localhost:11434"
      model: "deepseek-chat"
      timeout_seconds: 3
      
    - name: "openai"
      type: "api"
      enabled: false
      endpoint: "https://api.openai.com/v1"
      api_key: "${OPENAI_API_KEY}"
      model: "gpt-3.5-turbo"
      timeout_seconds: 5
      
    - name: "google"
      type: "api"
      enabled: false
      endpoint: "https://translation.googleapis.com"
      api_key: "${GOOGLE_API_KEY}"
      timeout_seconds: 5
  
  # 缓存配置
  cache:
    enabled: true
    backend: "redis"  # redis / sqlite / memory
    ttl_seconds: 3600
    max_entries: 100000
  
  # 性能配置
  performance:
    batch_size: 10              # 批量翻译大小
    max_concurrent: 5           # 最大并发数
    fallback_to_source: true    # 失败时返回原文
```

---

## 常用翻译场景

### 场景1: 日本用户访问中文虚拱

```cpp
// 用户选择日文
std::string targetLang = getUserPreferredLanguage(userId);  // "ja"

auto translationResult = translationService->translate(
    openClawResponse.text,
    "zh",     // OpenClaw 输出的中文
    "ja",     // 用户首选语言
    {{"tone", "friendly"}}
);

// 结果: 中文文本翻译为日文
```

### 场景2: 多语言同时输出

```cpp
// 为所有语言生成翻译版本
std::map<std::string, std::string> translatedResponses;
std::vector<std::string> targetLanguages = {"ja", "en", "ko"};

auto results = translationService->batchTranslate(
    std::vector<std::string>(3, openClawResponse.text),
    "zh",
    targetLanguages
);

for (size_t i = 0; i < targetLanguages.size(); ++i) {
    translatedResponses[targetLanguages[i]] = results[i].translatedText;
}
```

---

## 部署检查清单

- [ ] DeepSeek 本地部署成功 (Ollama)
- [ ] 翻译引擎优先级已配置
- [ ] 缓存后端已准备 (Redis/SQLite)
- [ ] 性能测试通过 (<200ms/翻译)
- [ ] 错误处理完善 (降级方案)
- [ ] 日志记录充分
- [ ] 支持的语言对已验证

---

**参考链接**: https://ollama.ai/ | https://openai.com/  
**更新日期**: 2026年4月3日
