# 🤖 OpenClaw 集成接口文档

**版本**: 1.0  
**更新日期**: 2026年4月3日  
**用途**: Yachiyo 虚拱形象直播平台与 OpenClaw 的通信规范

---

## 📋 目录

1. [概述](#概述)
2. [请求格式](#请求格式)
3. [响应格式](#响应格式)
4. [集成示例](#集成示例)
5. [错误处理](#错误处理)
6. [缓存策略](#缓存策略)

---

## 概述

### 工作流程

```
用户消息 (中文)
    ↓
【OpenClaw JSON 请求】
    ↓
OpenClaw 处理 (调用外部 AI 模型)
    ↓
【OpenClaw JSON 响应】
    ↓
提取: 文本 + 表情提示词 + 动作命令
    ↓
后续处理 (翻译、语音、动画)
```

### 关键特性

- 📤 **单向 Request → Response 模式**
- 📝 **JSON 格式通信**
- 🎭 **返回表情提示词** (用于 DeepSeek 审查、前端显示)
- 🎬 **返回基本动作命令** (用于 Live2D 驱动)
- ⏱️ **建议超时**: 5-10 秒
- 🔄 **支持缓存**: 相同消息可复用结果

---

## 请求格式

### 🔵 OpenClaw 请求 JSON

```json
{
  "message_id": "msg_20260403_001",
  "user_message": "你好啊，八千代！",
  "user_name": "访客",
  "avatar_name": "八千代辉夜姬",
  "platform": "web",
  "language": "zh",
  "context": {
    "mood": "neutral",
    "energy_level": 0.5,
    "previous_messages": [
      {
        "role": "user",
        "content": "早上好"
      },
      {
        "role": "avatar",
        "content": "早上好！今天天气真好呢"
      }
    ]
  },
  "settings": {
    "tone": "friendly",
    "max_length": 200,
    "include_emotions": true,
    "include_actions": true
  }
}
```

### 字段说明

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `message_id` | string | ✅ | 唯一消息ID，用于追踪和缓存 |
| `user_message` | string | ✅ | 用户输入的文本 (中文) |
| `user_name` | string | ✅ | 用户昵称 |
| `avatar_name` | string | ✅ | 虚拱名称 (八千代辉夜姬) |
| `platform` | string | ❌ | 平台类型: web/mobile/app |
| `language` | string | ❌ | 语言: zh/ja/en/ko (默认zh) |
| `context` | object | ❌ | 上下文信息 |
| `settings` | object | ❌ | 响应配置 |

### context 字段详解

```json
{
  "mood": "happy|sad|angry|neutral|shy",        // 虚拱当前心情
  "energy_level": 0.0-1.0,                      // 能量等级
  "previous_messages": [                        // 历史对话 (最多5条)
    {
      "role": "user|avatar",
      "content": "消息内容"
    }
  ],
  "time_of_day": "morning|afternoon|evening",   // 时间段
  "special_event": "birthday|holiday|etc"       // 特殊事件
}
```

### settings 字段详解

```json
{
  "tone": "friendly|professional|playful|serious",  // 语气
  "max_length": 200,                                // 最大字数
  "include_emotions": true,                         // 是否返回表情
  "include_actions": true,                          // 是否返回动作
  "response_style": "concise|detailed|poetic"       // 响应风格
}
```

---

## 响应格式

### 🟢 OpenClaw 响应 JSON

```json
{
  "message_id": "msg_20260403_001",
  "response_id": "resp_20260403_001",
  "status": "success",
  "data": {
    "text": "哈哈，你好呀！今天天气真好呢，我也很开心！",
    "emotion_prompts": [
      "开心",
      "热情",
      "期待"
    ],
    "emotion_confidence": {
      "开心": 0.95,
      "热情": 0.88,
      "期待": 0.76
    },
    "actions": [
      {
        "name": "微笑",
        "duration_ms": 1500,
        "intensity": 0.8,
        "easing": "ease-in-out"
      },
      {
        "name": "挥手",
        "duration_ms": 2000,
        "intensity": 0.6,
        "easing": "ease-out"
      },
      {
        "name": "点头",
        "duration_ms": 1000,
        "intensity": 0.5,
        "easing": "linear"
      }
    ],
    "processing_time_ms": 3215
  },
  "error": null
}
```

### 响应字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `message_id` | string | 原始请求的消息ID (用于匹配) |
| `response_id` | string | 响应的唯一ID |
| `status` | string | success / error / timeout |
| `data` | object | 响应数据 (见下表) |
| `error` | object | 错误信息 (失败时) |

### data 字段详解

| 字段 | 类型 | 说明 |
|------|------|------|
| `text` | string | OpenClaw 生成的回应文本 (中文) |
| `emotion_prompts` | array | 表情提示词列表 (用于 DeepSeek 审查) |
| `emotion_confidence` | object | 每个表情的置信度 (0.0-1.0) |
| `actions` | array | 动作命令列表 (见下表) |
| `processing_time_ms` | number | 处理耗时 (毫秒) |

### actions 数组详解

每个动作对象包含：

```json
{
  "name": "微笑|挥手|点头|摇头|眨眼|害羞|愤怒|...",
  "duration_ms": 1500,              // 动作持续时间
  "intensity": 0.8,                 // 动作强度 (0.0-1.0)
  "easing": "linear|ease-in|ease-out|ease-in-out",
  "delay_ms": 0,                    // 延迟时间 (可选)
  "loop": false                     // 是否循环 (可选)
}
```

### ✅ 成功响应示例

```json
{
  "message_id": "msg_20260403_001",
  "response_id": "resp_20260403_001",
  "status": "success",
  "data": {
    "text": "我很高兴看到你！希望你今天过得愉快。",
    "emotion_prompts": ["开心", "友善"],
    "emotion_confidence": {
      "开心": 0.92,
      "友善": 0.85
    },
    "actions": [
      {"name": "微笑", "duration_ms": 2000, "intensity": 0.9},
      {"name": "挥手", "duration_ms": 1500, "intensity": 0.7}
    ],
    "processing_time_ms": 2145
  },
  "error": null
}
```

### ❌ 错误响应示例

```json
{
  "message_id": "msg_20260403_001",
  "response_id": null,
  "status": "error",
  "data": null,
  "error": {
    "code": "OPENCLAW_SERVICE_ERROR",
    "message": "OpenClaw 服务暂时不可用",
    "details": "Connection timeout after 10s",
    "suggestion": "请使用本地模型降级"
  }
}
```

---

## 集成示例

### C++ 后端实现

```cpp
// backend/include/services/OpenClawGateway.hpp

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class OpenClawGateway {
private:
    std::string gatewayUrl;      // "http://localhost:18789"
    std::string apiKey;          // OpenClaw API Key
    CURL* curl;
    
    // HTTP 请求回调
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);
    
public:
    OpenClawGateway(const std::string& url, const std::string& key);
    ~OpenClawGateway();
    
    /**
     * 发送消息到 OpenClaw
     * @param request JSON 请求对象
     * @return 解析后的 JSON 响应
     */
    json processMessage(const json& request) {
        // 1. 构建 HTTP POST 请求
        std::string jsonStr = request.dump();
        
        curl_easy_setopt(curl, CURLOPT_URL, 
                        (gatewayUrl + "/api/process").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        
        // 2. 设置请求头
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, 
                                   ("Authorization: Bearer " + apiKey).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        // 3. 发送请求
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        
        // 4. 解析响应
        if (res != CURLE_OK) {
            return buildErrorResponse("CURL_ERROR", curl_easy_strerror(res));
        }
        
        try {
            return json::parse(response);
        } catch (const std::exception& e) {
            return buildErrorResponse("PARSE_ERROR", e.what());
        }
    }
    
    /**
     * 异步发送消息
     */
    std::future<json> processMessageAsync(const json& request) {
        return std::async(std::launch::async, [this, request]() {
            return processMessage(request);
        });
    }

private:
    json buildErrorResponse(const std::string& code, const std::string& msg) {
        return json{
            {"message_id", "unknown"},
            {"response_id", nullptr},
            {"status", "error"},
            {"data", nullptr},
            {"error", json{
                {"code", code},
                {"message", msg},
                {"details", ""},
                {"suggestion", "请重试或使用本地模型"}
            }}
        };
    }
};
```

### 使用示例

```cpp
// 在消息服务中调用
#include "services/OpenClawGateway.hpp"

class MessageService {
private:
    std::unique_ptr<OpenClawGateway> openClawGateway;
    
public:
    void processUserMessage(const std::string& userMessage, int64_t userId) {
        // 1. 构建请求
        json request = {
            {"message_id", "msg_" + generateId()},
            {"user_message", userMessage},
            {"user_name", getUserName(userId)},
            {"avatar_name", "八千代辉夜姬"},
            {"platform", "web"},
            {"language", "zh"},
            {"context", {
                {"mood", "neutral"},
                {"energy_level", 0.5},
                {"previous_messages", getPreviousMessages(userId, 5)}
            }},
            {"settings", {
                {"tone", "friendly"},
                {"max_length", 200},
                {"include_emotions", true},
                {"include_actions", true}
            }}
        };
        
        // 2. 发送请求
        auto response = openClawGateway->processMessage(request);
        
        // 3. 检查状态
        if (response["status"] != "success") {
            handleError(response["error"]);
            return;
        }
        
        // 4. 提取数据
        auto data = response["data"];
        std::string responseText = data["text"];
        std::vector<std::string> emotions = data["emotion_prompts"];
        std::vector<json> actions = data["actions"];
        
        // 5. 后续处理 (翻译、语音、动画等)
        processResponse(responseText, emotions, actions);
    }
};
```

---

## 错误处理

### 常见错误码

| 错误码 | HTTP状态 | 含义 | 处理方案 |
|--------|---------|------|---------|
| `OPENCLAW_UNAVAILABLE` | 503 | OpenClaw 服务不可用 | 使用本地模型降级 |
| `INVALID_REQUEST` | 400 | 请求格式错误 | 检查 JSON 格式 |
| `AUTH_FAILED` | 401 | 认证失败 | 检查 API Key |
| `TIMEOUT` | 504 | 请求超时 | 重试或降级 |
| `RATE_LIMIT` | 429 | 速率限制 | 缓存或队列 |
| `PARSE_ERROR` | 500 | 解析失败 | 记录日志并重试 |

### 错误处理代码示例

```cpp
void handleOpenClawResponse(const json& response) {
    if (response["status"] == "error") {
        auto error = response["error"];
        std::string code = error["code"];
        std::string message = error["message"];
        
        if (code == "OPENCLAW_UNAVAILABLE") {
            // 降级到本地模型
            LOG_WARN("OpenClaw 不可用，使用本地模型");
            useLocalModel();
        } 
        else if (code == "TIMEOUT") {
            // 重试一次
            LOG_WARN("OpenClaw 超时，准备重试");
            retry();
        }
        else if (code == "RATE_LIMIT") {
            // 加入队列
            LOG_WARN("触发速率限制，消息入队");
            addToQueue();
        }
        else {
            LOG_ERROR("OpenClaw 错误: {} - {}", code, message);
        }
    }
}
```

---

## 缓存策略

### 缓存机制

```cpp
class OpenClawCache {
private:
    std::map<std::string, CachedResponse> cache;  // message_id → response
    std::mutex cacheMutex;
    static const int CACHE_TTL_SECONDS = 3600;
    
public:
    /**
     * 获取缓存
     */
    std::optional<json> getFromCache(const std::string& messageId) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        auto it = cache.find(messageId);
        if (it == cache.end()) {
            return std::nullopt;  // 缓存未命中
        }
        
        // 检查是否过期
        auto now = std::time(nullptr);
        if (now - it->second.timestamp > CACHE_TTL_SECONDS) {
            cache.erase(it);
            return std::nullopt;  // 缓存已过期
        }
        
        return it->second.response;
    }
    
    /**
     * 保存到缓存
     */
    void saveToCache(const std::string& messageId, const json& response) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        cache[messageId] = {
            .response = response,
            .timestamp = std::time(nullptr)
        };
    }
};

// 在网关中使用缓存
json OpenClawGateway::processMessage(const json& request) {
    std::string messageId = request["message_id"];
    
    // 1. 检查缓存
    auto cached = cache->getFromCache(messageId);
    if (cached) {
        LOG_DEBUG("从缓存返回响应: {}", messageId);
        return cached.value();
    }
    
    // 2. 调用 OpenClaw API
    auto response = callOpenClawAPI(request);
    
    // 3. 保存缓存
    if (response["status"] == "success") {
        cache->saveToCache(messageId, response);
    }
    
    return response;
}
```

---

## 配置示例

```yaml
# backend/config/config.yaml

openclaw:
  enabled: true
  gateway_url: "http://localhost:18789"
  api_key: "${OPENCLAW_API_KEY}"
  
  # 连接配置
  connection:
    timeout_seconds: 10
    max_retries: 3
    retry_delay_ms: 1000
  
  # 缓存配置
  cache:
    enabled: true
    ttl_seconds: 3600
    max_entries: 10000
  
  # 降级配置
  fallback:
    enabled: true
    model: "local_ollama"  # 本地模型名称
    endpoint: "http://localhost:11434"
```

---

## 部署检查清单

- [ ] OpenClaw 本地网关已启动 (端口 18789)
- [ ] API Key 已配置
- [ ] 网络连接正常
- [ ] 请求格式正确
- [ ] 超时时间设置合理 (推荐 10 秒)
- [ ] 缓存策略已实现
- [ ] 错误处理已完善
- [ ] 日志记录充分
- [ ] 降级方案已准备

---

**参考链接**: https://openclaw.io/  
**更新日期**: 2026年4月3日
