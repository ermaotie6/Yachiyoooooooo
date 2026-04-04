# 🔑 OpenAI API Key 完整指南

## 📋 目录
1. [API Key 是什么](#1-api-key-是什么)
2. [获取 API Key](#2-获取-api-key)
3. [Yachiyo 中的使用场景](#3-yachiyo-中的使用场景)
4. [配置和安全](#4-配置和安全)
5. [成本估算](#5-成本估算)
6. [故障排查](#6-故障排查)

---

## 1. API Key 是什么

### 定义
**OpenAI API Key** 是访问 OpenAI 服务的身份凭证，类似于密码。它允许您的应用程序代表您调用 OpenAI 的 API（如 GPT-4、GPT-3.5 等）。

### 重要特性
- **唯一标识**: 每个 API Key 唯一对应一个 OpenAI 账户
- **权限控制**: 可以设置不同的权限和限制
- **用量追踪**: 所有 API 调用都会被追踪和计费
- **安全敏感**: 如同密码一样重要，**不要公开分享**

### 格式示例
```
sk-proj-abc123xyz...  (通常以 sk- 开头)
```

---

## 2. 获取 API Key

### 步骤 1: 创建 OpenAI 账户

1. 访问 https://platform.openai.com/signup
2. 使用邮箱注册（支持国内邮箱 QQ、126 等）
3. 验证邮箱

> ⚠️ **注意**: OpenAI 在某些地区可能有访问限制，建议使用 VPN

### 步骤 2: 设置支付方式

1. 登录 https://platform.openai.com/account/billing/overview
2. 点击 "Billing" → "Billing settings"
3. 添加支付方式（信用卡/借记卡）
4. 🇨🇳 **中国用户**: 可使用国际虚拟信用卡（如 Wise、Stripe）

### 步骤 3: 生成 API Key

1. 访问 https://platform.openai.com/api-keys
2. 点击 "Create new secret key"
3. 选择权限范围（推荐 "All" 权限）
4. **复制 Key 到安全位置**（只显示一次！）

### 步骤 4: 设置使用配额（可选）

1. 在账户设置中设置 "Usage limits"
2. 设置月度上限（防止意外高费用）
   - 示例: $20/月 防止滥用

---

## 3. Yachiyo 中的使用场景

### 场景 1️⃣: **翻译服务** (最常用)
```
中文输入 → OpenAI GPT → 日文输出
"你好，今天天气怎么样？" → "こんにちは、今日の天気はどうですか？"
```

**消耗成本**: 低 (中短文本)
- 单条消息: ~0.0001 - 0.001 USD

**代码位置**: `backend/src/services/TranslationService.cpp`

**相关配置**:
```cpp
const std::string TRANSLATION_ENGINE = "openai"; // 使用 OpenAI
const std::string OPENAI_MODEL = "gpt-3.5-turbo"; // 模型选择
```

### 场景 2️⃣: **AI 聊天补充** (可选)
如果不使用 OpenClaw 框架，可直接调用 GPT API 生成回复。

**消耗成本**: 中等 (完整对话)
- 单条对话: ~0.001 - 0.01 USD

**代码位置**: `backend/src/services/AIServiceImpl.cpp`

```cpp
// 发送 AI 请求的伪代码
std::string response = openaiClient.createChatCompletion({
    model: "gpt-3.5-turbo",
    messages: [{role: "user", content: user_message}],
    temperature: 0.7,
    max_tokens: 150
});
```

### 场景 3️⃣: **实时语义分析** (高级)
对用户消息进行语义理解，增强虚拱的智能度。

**消耗成本**: 中等
- 单条分析: ~0.0005 - 0.005 USD

---

## 4. 配置和安全

### 4.1 环境变量配置

**推荐方式**: 使用环境变量

#### 后端配置 (`backend/.env`)
```bash
# OpenAI API Key
OPENAI_API_KEY=sk-proj-your_actual_key_here

# 可选: 设置 API 代理（中国用户需要）
OPENAI_API_PROXY=https://proxy.example.com

# 可选: 设置 API 超时（秒）
OPENAI_API_TIMEOUT=30

# 模型选择
OPENAI_MODEL=gpt-3.5-turbo  # 或 gpt-4

# 翻译相关配置
TRANSLATION_ENGINE=openai
TRANSLATION_CACHE_SIZE=5000
TRANSLATION_CACHE_TTL=86400
```

#### 前端配置 (`frontend/.env`)
```bash
# 注意: 前端不应该直接存储 API Key！
# 所有 API 调用都应该通过后端代理

VITE_API_BASE_URL=http://localhost:8080/api
VITE_USE_TRANSLATION=true
```

### 4.2 安全最佳实践

#### ✅ 必做

1. **永远不要提交 API Key 到版本控制**
```bash
# 在 .gitignore 中添加
.env
.env.local
.env.*.local
```

2. **使用环境变量隐藏 API Key**
```cpp
// 正确的做法
const char* api_key = std::getenv("OPENAI_API_KEY");

// 错误的做法 (硬编码)
const std::string api_key = "sk-proj-xxx"; // ❌ 不要这样做
```

3. **定期轮换 API Key**
   - 每 3 个月更换一次
   - 发现泄露后立即删除

4. **使用 API Key 限制**
   - 在 OpenAI 面板设置使用配额
   - 绑定 IP 白名单（如果可能）

#### 🚫 禁止事项

- ❌ 不要在客户端代码中存储 API Key
- ❌ 不要将 API Key 提交到 GitHub
- ❌ 不要在日志中打印 API Key
- ❌ 不要通过不安全的连接发送 API Key
- ❌ 不要与他人共享 API Key

### 4.3 代码实现示例

#### 后端调用 (C++)
```cpp
#include <curl/curl.h>
#include <nlohmann/json.hpp>

class OpenAIClient {
private:
    std::string api_key;
    std::string api_url = "https://api.openai.com/v1";

public:
    OpenAIClient() {
        // 从环境变量读取 API Key
        const char* key = std::getenv("OPENAI_API_KEY");
        if (key) {
            api_key = key;
        } else {
            throw std::runtime_error("OPENAI_API_KEY not set");
        }
    }

    // 调用翻译 API
    std::string translateText(const std::string& text) {
        std::string url = api_url + "/chat/completions";
        
        // 构建请求
        nlohmann::json request = {
            {"model", "gpt-3.5-turbo"},
            {"messages", nlohmann::json::array({
                {{"role", "system"}, {"content", "You are a translator. Translate Chinese to Japanese."}},
                {{"role", "user"}, {"content", text}}
            })},
            {"temperature", 0.3},
            {"max_tokens", 200}
        };

        // 设置 CURL
        CURL* curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        
        // 设置认证头
        std::string auth_header = "Authorization: Bearer " + api_key;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, auth_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 发送请求
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        std::string post_data = request.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        
        curl_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        // 解析响应
        auto resp_json = nlohmann::json::parse(response);
        return resp_json["choices"][0]["message"]["content"];
    }

private:
    static size_t writeCallback(void* contents, size_t size, 
                                size_t nmemb, std::string* s) {
        s->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
};
```

#### 错误处理
```cpp
try {
    std::string translation = openai_client.translateText(user_message);
    return AvatarResponse{
        .text_response = translation,
        .status = "success"
    };
} catch (const std::exception& e) {
    logger.error("OpenAI API error: {}", e.what());
    
    // 返回备用响应
    return AvatarResponse{
        .text_response = "申し訳ございません、翻訳に失敗しました。", // 对不起，翻译失败
        .status = "error",
        .fallback = true
    };
}
```

---

## 5. 成本估算

### 当前模型的价格（2026年）

| 模型 | 输入价格 | 输出价格 | 推荐用途 |
|-----|---------|---------|---------|
| **gpt-3.5-turbo** | $0.50/1M tokens | $1.50/1M tokens | ✅ 翻译、聊天 |
| **gpt-4** | $30/1M tokens | $60/1M tokens | ❌ 成本太高 |
| **gpt-4-turbo** | $10/1M tokens | $30/1M tokens | 🟡 高精度任务 |

> 💡 **Token 说明**: 大约 4 个字符 = 1 token（中文通常 2-3 字 = 1 token）

### 使用场景成本

#### 🎯 翻译场景（最常见）
- 平均消息长度: 50 字 ≈ 25 tokens
- 请求 + 响应: ~50 tokens 总计
- **单次成本**: 50 × ($0.50 + $1.50) / 1M = $0.0001 USD

#### 📊 日均成本估算

| 日活用户 | 平均消息数 | 日成本 | 月成本 |
|---------|----------|--------|--------|
| 10 | 10 | $0.01 | $0.30 |
| 100 | 20 | $0.20 | $6.00 |
| 1,000 | 50 | $2.50 | $75 |
| 10,000 | 100 | $50 | $1,500 |

### 💰 成本控制建议

1. **使用缓存** ✅
   - 相同查询缓存 24 小时，减少重复调用
   - Yachiyo 实现了 5000 项翻译缓存

2. **选择合适的模型**
   - 翻译任务用 gpt-3.5-turbo （足够且便宜）
   - 不用 gpt-4 除非必要

3. **设置请求限制**
```cpp
// 单个用户每天最多翻译 1000 次
const int DAILY_TRANSLATION_LIMIT = 1000;

// 响应返回长度限制（减少输出 tokens）
const int MAX_RESPONSE_LENGTH = 200;
```

4. **使用批处理**
   - 合并多个请求为一个
   - 使用 Batch API （成本便宜 50%）

---

## 6. 故障排查

### 问题 1: `401 Unauthorized`
**原因**: API Key 不正确或过期

**解决方案**:
```bash
# 检查 API Key 是否已设置
echo $OPENAI_API_KEY

# 检查 API Key 是否有效
curl https://api.openai.com/v1/models \
  -H "Authorization: Bearer $OPENAI_API_KEY"

# 如果返回 401，重新生成 API Key
```

### 问题 2: `429 Too Many Requests`
**原因**: 请求过于频繁，超过速率限制

**解决方案**:
```cpp
// 实现退避策略
int retry_count = 0;
while (retry_count < 3) {
    try {
        return openai_client.translate(text);
    } catch (const RateLimitException& e) {
        retry_count++;
        int wait_time = (1 << retry_count) * 1000; // 指数级退避
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
    }
}
```

### 问题 3: `Context Length Exceeded`
**原因**: 消息过长，超过模型的 token 限制

**解决方案**:
```cpp
// 检查长度并截断
const int MAX_TOKENS = 2000;
if (text.length() > MAX_TOKENS) {
    text = text.substr(0, MAX_TOKENS);
    logger.warn("Message truncated to MAX_TOKENS");
}
```

### 问题 4: 翻译质量不佳
**原因**: 模型不是专业翻译模型，可能出错

**解决方案**:
```cpp
// 实现翻译质量评分
auto [translation, quality_score] = openai_client.translateWithQuality(text);

if (quality_score < 0.7) {
    // 尝试使用其他翻译引擎或返回原文
    logger.warn("Low translation quality: {}", quality_score);
    return original_text;
}
```

---

## 🎓 最佳实践总结

| 实践 | 为什么重要 | 实现方式 |
|------|----------|---------|
| 使用环境变量 | 隐藏敏感信息 | `.env` 文件 + `getenv()` |
| 实现缓存 | 减少 API 调用 | Redis + 24h TTL |
| 错误处理 | 提高可用性 | try-catch + fallback |
| 请求限制 | 控制成本 | 用户限额 + 配额检查 |
| 定期轮换 | 提高安全性 | 每 3 个月更换 |
| 监控使用 | 及时发现问题 | 记录所有 API 调用 |

---

## 📞 技术支持

- **OpenAI 官方文档**: https://platform.openai.com/docs/api-reference
- **API 状态**: https://status.openai.com
- **支持邮箱**: support@openai.com

---

**最后更新**: 2026-04-02  
**下一步**: 配置完成后，运行 `backend/tests/test_openai_integration.cpp` 进行集成测试。
