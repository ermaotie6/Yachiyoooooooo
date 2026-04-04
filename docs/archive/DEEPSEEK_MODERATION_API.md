# 🛡️ DeepSeek 内容审查接口文档

**版本**: 1.0  
**更新日期**: 2026年4月3日  
**用途**: Yachiyo 虚拱形象直播平台的内容审查服务

---

## 📋 目录

1. [概述](#概述)
2. [审查流程](#审查流程)
3. [请求格式](#请求格式)
4. [响应格式](#响应格式)
5. [审查类别](#审查类别)
6. [集成示例](#集成示例)
7. [性能优化](#性能优化)

---

## 概述

### 为什么使用 DeepSeek?

✅ **本地部署友好** - 支持本地 Ollama 部署
✅ **低成本** - 相比 OpenAI 审查 API 成本低 50%+
✅ **快速响应** - 本地推理延迟 <500ms
✅ **多语言支持** - 支持中文、日文、英文等
✅ **灵活定制** - 可自定义审查规则

### 审查职责

DeepSeek 在 Yachiyo 中专门负责：
- 🚫 检测有害内容 (暴力、仇恨、性骚扰等)
- 🔞 检测成人内容
- 📢 检测垃圾/广告
- 🎭 **辅助生成表情提示词** (基于 OpenClaw 文本)
- 💬 检测骂人/脏话

---

## 审查流程

```
┌─────────────────────────────────────────────────┐
│       接收 OpenClaw 响应文本 + 表情提示词        │
│       (例: "你好，我很开心" + ["开心"])          │
└────────────────┬────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────┐
│        构建 DeepSeek 审查请求 JSON              │
│  包含: 原始文本、表情提示词、上下文              │
└────────────────┬────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────┐
│    调用 DeepSeek (本地 Ollama 或云端 API)       │
└────────────────┬────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────┐
│         返回审查结果 + 风险评分                  │
│  {is_safe: true, categories: [], score: 0.1}    │
└────────────────┬────────────────────────────────┘
                 │
                 ↓
    ├─ 通过 ✅ → 推送给前端
    └─ 拒绝 ❌ → 返回错误或使用默认回复
```

---

## 请求格式

### 🔵 DeepSeek 审查请求

```json
{
  "message_id": "msg_20260403_001",
  "task_type": "comprehensive_check",
  "content": {
    "text": "你好，我很开心！希望你今天过得愉快。",
    "emotion_prompts": ["开心", "友善", "期待"],
    "emotion_confidence": {
      "开心": 0.95,
      "友善": 0.88
    },
    "user_name": "访客",
    "context": {
      "platform": "web",
      "language": "zh",
      "user_history": [
        {
          "content": "早上好",
          "safe": true
        }
      ]
    }
  },
  "settings": {
    "strict_mode": false,
    "categories": [
      "violence",
      "adult_content",
      "harassment",
      "spam",
      "hate_speech",
      "profanity"
    ],
    "return_details": true
  }
}
```

### 字段说明

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `message_id` | string | ✅ | 消息ID (追踪用) |
| `task_type` | string | ✅ | 任务类型: comprehensive_check / quick_check / emotion_verify |
| `content` | object | ✅ | 要审查的内容 |
| `settings` | object | ❌ | 审查配置 |

### content 对象详解

```json
{
  "text": "要审查的文本内容",
  "emotion_prompts": ["表情1", "表情2"],      // OpenClaw 返回的表情
  "emotion_confidence": {                    // 表情置信度
    "表情1": 0.95,
    "表情2": 0.88
  },
  "user_name": "用户昵称",
  "context": {
    "platform": "web|mobile|app",
    "language": "zh|ja|en",
    "user_history": [                        // 用户历史 (用于检测行为)
      {
        "content": "历史消息",
        "safe": true,
        "timestamp": 1712145600
      }
    ],
    "user_reputation": 0.8                   // 用户信誉度 (0-1)
  }
}
```

### settings 对象详解

```json
{
  "strict_mode": false,              // true: 严格模式 (推荐 false)
  "categories": [                    // 要检查的类别
    "violence",                      // 暴力内容
    "adult_content",                 // 成人内容
    "harassment",                    // 骚扰/网暴
    "spam",                          // 垃圾/广告
    "hate_speech",                   // 仇恨言论
    "profanity",                     // 脏话/骂人
    "misinformation"                 // 虚假信息
  ],
  "return_details": true,            // 返回详细信息
  "emotion_verify": true             // 验证表情是否匹配文本
}
```

---

## 响应格式

### 🟢 DeepSeek 审查响应

```json
{
  "message_id": "msg_20260403_001",
  "task_type": "comprehensive_check",
  "status": "success",
  "data": {
    "is_safe": true,
    "overall_risk_score": 0.12,
    "categories": {
      "violence": {
        "detected": false,
        "score": 0.05,
        "confidence": 0.99
      },
      "adult_content": {
        "detected": false,
        "score": 0.02,
        "confidence": 0.98
      },
      "harassment": {
        "detected": false,
        "score": 0.08,
        "confidence": 0.95
      },
      "spam": {
        "detected": false,
        "score": 0.01,
        "confidence": 0.99
      },
      "hate_speech": {
        "detected": false,
        "score": 0.03,
        "confidence": 0.98
      },
      "profanity": {
        "detected": false,
        "score": 0.00,
        "confidence": 1.00
      }
    },
    "emotion_verification": {
      "matched": true,
      "verified_emotions": ["开心", "友善"],
      "suggested_emotions": ["开心", "期待", "友善"],
      "confidence": 0.92
    },
    "recommended_action": "allow",
    "processing_time_ms": 245
  },
  "error": null
}
```

### 响应字段说明

| 字段 | 说明 |
|------|------|
| `message_id` | 原始请求的消息ID |
| `is_safe` | 是否安全 (true/false) |
| `overall_risk_score` | 总体风险分数 (0.0-1.0) |
| `categories` | 各类别的检查结果 |
| `emotion_verification` | 表情验证结果 |
| `recommended_action` | 推荐动作: allow / flag / reject |

### categories 对象详解

每个类别包含：

```json
{
  "detected": false,          // 是否检测到问题内容
  "score": 0.05,              // 风险分数 (0.0-1.0)
  "confidence": 0.99,         // 置信度 (0.0-1.0)
  "details": "无问题",         // 详细说明 (可选)
  "flagged_content": ""       // 问题内容片段 (可选)
}
```

### emotion_verification 对象详解

```json
{
  "matched": true,                           // 表情是否匹配文本
  "verified_emotions": ["开心", "友善"],     // 验证通过的表情
  "suggested_emotions": [                    // DeepSeek 建议的表情
    "开心",
    "期待",
    "友善"
  ],
  "confidence": 0.92,                        // 匹配度
  "reason": "文本传达的情感与表情标签一致"  // 匹配说明
}
```

### 推荐动作说明

| 动作 | 说明 | 处理 |
|------|------|------|
| `allow` | 内容安全，可直接推送 | ✅ 推送给前端 |
| `flag` | 需要标记审查，但可显示 | ⚠️ 显示但加标记 |
| `reject` | 内容有问题，拒绝 | ❌ 返回错误响应 |

---

## 审查类别

### 1️⃣ Violence (暴力)
- 关键词: 杀、打、砍、枪、炸等
- 示例: "我要砍死你"
- 处理: `reject`

### 2️⃣ Adult Content (成人内容)
- 关键词: 色、淫、性、裸露等
- 示例: "我很性感"
- 处理: `reject` 或 `flag`

### 3️⃣ Harassment (骚扰)
- 关键词: 骂人、人身攻击、威胁等
- 示例: "你是傻子"
- 处理: `reject`

### 4️⃣ Spam (垃圾/广告)
- 关键词: URL、重复、推销、链接等
- 示例: "点击这个链接xxx.com"
- 处理: `reject`

### 5️⃣ Hate Speech (仇恨言论)
- 关键词: 歧视、偏见、仇恨等
- 示例: "我讨厌某个群体"
- 处理: `reject`

### 6️⃣ Profanity (脏话)
- 关键词: 骂人、脏话等
- 示例: "你 TMD..."
- 处理: `flag` 或 `reject`

---

## 集成示例

### C++ 后端实现

```cpp
// backend/include/services/DeepSeekModerationService.hpp

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class DeepSeekModerationService {
private:
    std::string serviceEndpoint;  // Ollama 或 API 端点
    std::string model;            // "deepseek-chat"
    
public:
    struct ModerationResult {
        bool isSafe;
        float overallRiskScore;
        std::map<std::string, float> categoryScores;
        std::string recommendedAction;  // allow / flag / reject
        bool emotionMatched;
        std::vector<std::string> suggestedEmotions;
    };
    
    /**
     * 执行内容审查
     */
    ModerationResult moderateContent(
        const std::string& text,
        const std::vector<std::string>& emotions,
        const std::string& userId = ""
    ) {
        // 1. 构建审查请求
        json request = buildModerationRequest(text, emotions, userId);
        
        // 2. 调用 DeepSeek
        auto response = callDeepSeek(request);
        
        // 3. 解析响应
        return parseModerationResponse(response);
    }
    
    /**
     * 验证表情是否匹配文本
     */
    bool verifyEmotions(
        const std::string& text,
        const std::vector<std::string>& emotions
    ) {
        json request = {
            {"task_type", "emotion_verify"},
            {"content", {
                {"text", text},
                {"emotion_prompts", emotions}
            }}
        };
        
        auto response = callDeepSeek(request);
        return response["data"]["emotion_verification"]["matched"];
    }

private:
    json buildModerationRequest(
        const std::string& text,
        const std::vector<std::string>& emotions,
        const std::string& userId
    ) {
        return json{
            {"message_id", "msg_" + generateId()},
            {"task_type", "comprehensive_check"},
            {"content", {
                {"text", text},
                {"emotion_prompts", emotions},
                {"user_name", getUserName(userId)},
                {"context", {
                    {"platform", "web"},
                    {"language", "zh"},
                    {"user_history", getUserHistory(userId, 5)}
                }}
            }},
            {"settings", {
                {"strict_mode", false},
                {"categories", json::array({
                    "violence", "adult_content", "harassment",
                    "spam", "hate_speech", "profanity"
                })},
                {"return_details", true},
                {"emotion_verify", true}
            }}
        };
    }
    
    json callDeepSeek(const json& request) {
        // 使用 curl 调用 DeepSeek API 或本地 Ollama
        // 返回 JSON 响应
        // ... (curl 代码类似于 OpenClaw 网关)
    }
    
    ModerationResult parseModerationResponse(const json& response) {
        auto data = response["data"];
        
        ModerationResult result;
        result.isSafe = data["is_safe"];
        result.overallRiskScore = data["overall_risk_score"];
        result.recommendedAction = data["recommended_action"];
        result.emotionMatched = data["emotion_verification"]["matched"];
        result.suggestedEmotions = data["emotion_verification"]["suggested_emotions"];
        
        // 提取各类别分数
        for (auto& [category, info] : data["categories"].items()) {
            result.categoryScores[category] = info["score"];
        }
        
        return result;
    }
};
```

### 在消息服务中使用

```cpp
// backend/src/services/MessageServiceImpl.cpp

Result<std::shared_ptr<Message>> MessageServiceImpl::sendMessage(
    int64_t userId,
    const std::string& message,
    const std::string& userIp
) {
    try {
        // ... 前4层审查 ...
        
        // 第4层: 使用 DeepSeek 进行 AI 审查
        auto openClawResponse = openClawGateway->processMessage(request);
        
        auto deepseekResult = deepseekService->moderateContent(
            openClawResponse.text,
            openClawResponse.emotionPrompts,
            std::to_string(userId)
        );
        
        if (!deepseekResult.isSafe) {
            if (deepseekResult.recommendedAction == "reject") {
                msg->setReviewStatus(ReviewStatus::REJECTED);
                msg->setReviewReason("内容审查未通过");
                return Result<std::shared_ptr<Message>>::Error("消息被拦截");
            } 
            else if (deepseekResult.recommendedAction == "flag") {
                msg->setReviewStatus(ReviewStatus::MANUAL_REVIEW);
                msg->setReviewReason("需要人工审查");
            }
        } else {
            msg->setReviewStatus(ReviewStatus::APPROVED);
            
            // 使用 DeepSeek 建议的表情（如果更准确）
            if (deepseekResult.emotionMatched) {
                msg->setFinalEmotions(deepseekResult.suggestedEmotions);
            }
        }
        
        // ... 后续处理 ...
        
    } catch (const std::exception& e) {
        LOG_ERROR("消息处理错误: {}", e.what());
        return Result<std::shared_ptr<Message>>::Error(e.what());
    }
}
```

---

## 性能优化

### 缓存策略

```cpp
class DeepSeekCache {
private:
    std::unordered_map<std::string, CachedResult> cache;
    
public:
    // 生成缓存 key (基于文本内容)
    std::string generateCacheKey(const std::string& text) {
        // SHA256(text) 的前16个字符
        return hashText(text).substr(0, 16);
    }
    
    std::optional<ModerationResult> getFromCache(const std::string& text) {
        auto key = generateCacheKey(text);
        auto it = cache.find(key);
        
        if (it != cache.end() && !it->second.isExpired()) {
            return it->second.result;
        }
        return std::nullopt;
    }
    
    void saveToCache(const std::string& text, const ModerationResult& result) {
        auto key = generateCacheKey(text);
        cache[key] = {
            .result = result,
            .timestamp = std::time(nullptr)
        };
    }
};
```

### 批量审查

```cpp
// 同时审查多条消息，提高吞吐量
std::vector<ModerationResult> batchModerate(
    const std::vector<std::string>& texts,
    const std::vector<std::vector<std::string>>& emotions
) {
    std::vector<std::future<ModerationResult>> futures;
    
    for (size_t i = 0; i < texts.size(); ++i) {
        futures.push_back(
            std::async(std::launch::async, [this, &texts, &emotions, i]() {
                return moderateContent(texts[i], emotions[i]);
            })
        );
    }
    
    std::vector<ModerationResult> results;
    for (auto& f : futures) {
        results.push_back(f.get());
    }
    
    return results;
}
```

### 配置示例

```yaml
# backend/config/config.yaml

deepseek_moderation:
  enabled: true
  
  # 本地 Ollama 配置
  ollama:
    enabled: true
    endpoint: "http://localhost:11434"
    model: "deepseek-chat"
    timeout_seconds: 5
  
  # 云端 API 配置 (备选)
  api:
    enabled: false
    endpoint: "https://api.deepseek.com/v1"
    api_key: "${DEEPSEEK_API_KEY}"
  
  # 缓存配置
  cache:
    enabled: true
    ttl_seconds: 3600
    max_entries: 50000
  
  # 审查阈值
  thresholds:
    violence: 0.7
    adult_content: 0.6
    harassment: 0.75
    spam: 0.8
    hate_speech: 0.65
    profanity: 0.5
```

---

## 部署检查清单

- [ ] DeepSeek 模型已下载 (Ollama)
- [ ] Ollama 服务已启动 (端口 11434)
- [ ] API 端点配置正确
- [ ] 超时时间设置合理 (5-10秒)
- [ ] 缓存已启用
- [ ] 日志记录充分
- [ ] 性能测试通过

---

**参考链接**: https://ollama.ai/ | https://deepseek.com/  
**更新日期**: 2026年4月3日
