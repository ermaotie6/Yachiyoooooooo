# 🤖 Yachiyo 与 Openclaw 集成指南

**集成状态**: ✅ 框架完成，等待 API 密钥配置  
**当前模式**: 降级方案 (启发式检查) + Openclaw 框架  
**所需工作**: 仅需配置 API 密钥，代码已完成  

---

## 🎯 快速概览

### 什么是 Openclaw？

**Openclaw** 是一个 AI 内容审查服务，用于自动检测并标记不适合的内容。

在 Yachiyo 项目中，Openclaw 是 **6 层内容审查系统的第 4 层**。

```
6 层内容审查系统
├─ Layer 1: 速率限制 (Redis)          ✅ 已实现
├─ Layer 2: IP 黑名单 (数据库)       ✅ 已实现
├─ Layer 3: 敏感词过滤 (内存)        ✅ 已实现
├─ Layer 4: AI 审查 (Openclaw) 🤖     ⏳ 等待配置
├─ Layer 5: 行为异常检测 (Redis)     ✅ 已实现
└─ Layer 6: 人工审查标记              ✅ 已实现
```

### 现在的状态

```
当前 (无 Openclaw API 密钥)       配置后 (有 Openclaw API 密钥)
┌─────────────────────────┐     ┌─────────────────────────┐
│  消息审查流程            │     │  消息审查流程            │
├─────────────────────────┤     ├─────────────────────────┤
│ Layer 1-3: 快速检查 ✅   │     │ Layer 1-3: 快速检查 ✅   │
│ Layer 4: 启发式降级 ⚠️   │ --> │ Layer 4: Openclaw AI 🤖  │
│ Layer 5-6: 继续检查 ✅   │     │ Layer 5-6: 继续检查 ✅   │
│                          │     │                          │
│ 结果: 中等准确度         │     │ 结果: 高准确度          │
└─────────────────────────┘     └─────────────────────────┘
```

---

## 📋 3 步集成 Openclaw

### 步骤 1️⃣: 获取 Openclaw API 密钥

**你需要**:
1. 访问 https://openclaw.io (或你们的 Openclaw 部署地址)
2. 注册账户或使用现有账户
3. 创建 API 密钥
4. 获取以下信息:
   - ✅ `API_KEY`: 认证密钥
   - ✅ `API_URL`: API 端点 (例: https://api.openclaw.io)

**示例**:
```
API_KEY = "oc_sk_live_12345678901234567890"
API_URL = "https://api.openclaw.io"
```

### 步骤 2️⃣: 配置环境变量

编辑 `.env` 文件 (或 `config/config.yaml`):

```bash
# .env 文件
OPENCLAW_API_KEY=oc_sk_live_12345678901234567890
OPENCLAW_API_URL=https://api.openclaw.io
```

或编辑 `config/config.yaml`:

```yaml
openclaw:
  enabled: true
  api_key: "oc_sk_live_12345678901234567890"
  api_url: "https://api.openclaw.io"
  timeout_seconds: 10
  max_retries: 3
```

### 步骤 3️⃣: 重启服务

```bash
# 如果用 Docker
docker-compose restart yachiyo-cpp

# 如果本地运行
./src/Application
```

**完成！** 🎉 Openclaw 现在会自动用于所有内容审查。

---

## 🔄 工作流程

### 消息从提交到审查的完整流程

```
用户发送消息
    ↓
┌─────────────────────────────────────────┐
│ MessageService::sendMessage()            │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ Layer 1: 速率限制检查                    │
│ - 检查用户是否超过 100 msg/分钟          │
└─────────────────────────────────────────┘
    ↓ (通过)
┌─────────────────────────────────────────┐
│ Layer 2: IP 黑名单检查                   │
│ - 检查用户 IP 是否在黑名单中             │
└─────────────────────────────────────────┘
    ↓ (通过)
┌─────────────────────────────────────────┐
│ Layer 3: 敏感词检查                      │
│ - 扫描 5 种敏感词库                      │
│ - 返回风险等级 (1-5)                    │
└─────────────────────────────────────────┘
    ↓ (如果敏感词等级 < 3)
┌─────────────────────────────────────────┐
│ Layer 4: AI 内容审查 🤖                  │
│                                          │
│ 如果配置了 Openclaw:                     │
│ ├─ 调用 OpencalwClient::reviewContent()  │
│ ├─ 发送 POST /api/review/text            │
│ ├─ 返回: { allowed, score, categories }  │
│ │                                        │
│ └─ score 映射:                           │
│    ├─ 0.0-0.3: APPROVED ✅              │
│    ├─ 0.3-0.6: MANUAL_REVIEW 📋         │
│    └─ 0.6-1.0: REJECTED ❌              │
│                                          │
│ 如果 Openclaw 不可用 (降级):             │
│ ├─ 使用启发式检查                       │
│ ├─ 检查: 全大写, 重复字符, 特殊字符     │
│ └─ 返回: 简化的风险评分                 │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ Layer 5: 行为异常检测                    │
│ - 检查用户发送频率是否异常               │
│ - 检查用户是否在黑名单                   │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ Layer 6: 人工审查标记                    │
│ - 标记是否需要人工审查                   │
│ - 设置审查优先级                         │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ 消息保存到数据库                         │
│ - status: approved / pending / rejected   │
│ - review_level: 0-6 (审查层级)           │
│ - openclaw_response: Openclaw 完整响应   │
└─────────────────────────────────────────┘
    ↓
返回响应给前端
```

---

## 💻 代码位置和实现

### 核心文件

```
YachiyoCPP/
├── include/utils/OpencalwClient.hpp      ← Openclaw 客户端接口
├── src/utils/OpencalwClient.cpp          ← Openclaw 客户端实现
├── include/services/MessageService.hpp   ← 消息服务接口
├── src/services/MessageService.cpp       ← 消息服务实现 (6层审查)
└── config/config.yaml                    ← 配置文件
```

### OpencalwClient 核心方法

```cpp
class OpencalwClient {
public:
    // 1. 审查单个消息
    Result<ReviewResult> reviewContent(
        const std::string& message,  // 要审查的消息
        int64_t userId = 0           // 用户 ID (用于日志)
    );
    
    // 2. 批量审查消息 (效率更高)
    Result<std::vector<ReviewResult>> batchReview(
        const std::vector<std::pair<int64_t, std::string>>& messages
    );
    
    // 3. 健康检查 (验证 API 连接)
    Result<bool> healthCheck();
    
    // 4. 获取 API 限流配额
    Result<json> getRateLimit();
};
```

### 审查结果结构

```cpp
struct ReviewResult {
    bool isAllowed;                    // 是否允许发送
    double riskScore;                  // 风险评分 (0.0-1.0)
    std::string mainCategory;          // 主违规类别 (如 "abuse")
    std::vector<std::string> categories; // 所有违规类别
    std::string rawResponse;           // 原始 JSON 响应
};
```

### 在 MessageService 中的使用

```cpp
// 在 MessageServiceImpl::sendMessage() 中
// Layer 4: AI 内容审查
auto reviewResult = aiContentReview(message.content, userId);
// 或者调用 Openclaw
auto opencalwResult = opencalwClient->reviewContent(message.content, userId);

if (!opencalwResult.isSuccess()) {
    LOG_WARN("Openclaw 审查失败，降级到启发式检查");
    reviewResult = aiContentReview(message.content, userId);
}

// 记录审查结果
message.setReviewStatus(
    opencalwResult.getData().isAllowed ? "APPROVED" : "REJECTED"
);
```

---

## 🧪 测试 Openclaw 集成

### 测试 1: 验证 API 连接

```bash
# 在应用启动时检查
curl -X POST https://api.openclaw.io/api/health \
  -H "Authorization: Bearer your-api-key" \
  -H "Content-Type: application/json"

# 预期响应
{
  "status": "ok",
  "message": "API is available"
}
```

### 测试 2: 手动测试单个消息审查

```bash
curl -X POST http://localhost:8080/api/v1/messages/send \
  -H "Authorization: Bearer your-token" \
  -H "Content-Type: application/json" \
  -d '{
    "content": "这是一条测试消息"
  }'

# 预期响应
{
  "success": true,
  "data": {
    "messageId": 12345,
    "status": "approved",
    "reviewLevel": 4,
    "openclaw_response": {
      "allowed": true,
      "score": 0.1,
      "mainCategory": "none",
      "categories": []
    }
  }
}
```

### 测试 3: 查看应用日志

```bash
# 查看 Openclaw 相关日志
tail -f logs/app.log | grep -i openclaw

# 预期日志
INFO 初始化OpencalwClient: https://api.openclaw.io
INFO 内容审查完成: userId=123, allowed=true, score=0.1
```

### 测试 4: 模拟 Openclaw 故障

```bash
# 临时禁用 Openclaw (改配置)
# 在 config.yaml 中设置
openclaw:
  enabled: false

# 重启应用
docker-compose restart yachiyo-cpp

# 测试发送消息 - 应该使用降级的启发式检查
curl -X POST http://localhost:8080/api/v1/messages/send ...

# 预期日志
WARN openclaw审查失败: 连接超时，降级到启发式检查
INFO 消息已使用启发式检查: message_id=12345, score=0.35
```

---

## 🚨 错误处理和降级

### 如果 Openclaw 不可用，会发生什么？

```
Openclaw API 超时/离线
         ↓
捕捉异常，记录 WARN 日志
         ↓
自动降级到启发式检查
         ↓
使用内置的轻量级检查算法
         ↓
消息照常处理和发送
         ↓
其他 5 层审查继续运行
```

**关键点**: 即使 Openclaw 不可用，系统仍然能继续运行！

### 常见错误和解决方案

| 错误 | 原因 | 日志中的信息 | 解决方案 |
|------|------|-----------|---------|
| **401 Unauthorized** | API 密钥无效 | `HTTP 401 Unauthorized` | 检查 `OPENCLAW_API_KEY` 是否正确 |
| **429 Too Many Requests** | 超过 API 限流 | `HTTP 429 Too Many Requests` | 减少请求频率或升级 Openclaw 计划 |
| **503 Service Unavailable** | Openclaw 服务宕机 | `HTTP 503 Service Unavailable` | 等待 Openclaw 服务恢复 |
| **Connection Timeout** | 网络连接太慢 | `Timeout` | 增加超时时间或检查网络 |
| **Invalid JSON Response** | API 返回格式错误 | `Failed to parse JSON` | 联系 Openclaw 支持 |

### 如何查看是否使用了降级方案

```bash
# 在日志中搜索
grep -i "启发式检查" logs/app.log

# 如果有输出，说明正在使用降级方案
# 示例:
# WARN openclaw审查失败: 连接超时，降级到启发式检查
# INFO 消息已使用启发式检查: message_id=12345, score=0.35
```

---

## 📊 监控 Openclaw 性能

### 关键指标

```
1. API 响应时间
   - 目标: < 500ms
   - 告警: > 2000ms

2. 失败率
   - 目标: < 0.1%
   - 告警: > 1%

3. 限流触发率
   - 目标: 0%
   - 告警: > 0.01%

4. 降级使用频率
   - 目标: 0%
   - 告警: > 5%
```

### Prometheus 指标

```yaml
# 可以在 Prometheus 中查看
openclaw_request_duration_seconds    # 请求耗时
openclaw_request_failures_total      # 失败次数
openclaw_requests_total              # 总请求数
openclaw_fallback_uses_total         # 降级使用次数
```

### 在 Grafana 中添加仪表板

```json
{
  "title": "Openclaw 性能监控",
  "panels": [
    {
      "title": "API 响应时间",
      "targets": [
        {
          "expr": "openclaw_request_duration_seconds"
        }
      ]
    },
    {
      "title": "失败率",
      "targets": [
        {
          "expr": "rate(openclaw_request_failures_total[5m])"
        }
      ]
    }
  ]
}
```

---

## 📝 API 响应格式

### Openclaw API 返回的审查结果

```json
{
  "allowed": false,
  "score": 0.85,
  "mainCategory": "abuse",
  "categories": ["abuse", "harassment"],
  "details": {
    "text": "original message",
    "confidence": 0.92,
    "flags": ["contains_profanity", "aggressive_tone"]
  }
}
```

### 消息表中保存的数据

```sql
-- messages 表
SELECT 
    id,
    content,
    status,                 -- "approved" / "pending" / "rejected"
    review_level,           -- 0-6 (哪一层拒绝了)
    openclaw_response,      -- Openclaw 的完整 JSON 响应
    created_at
FROM messages
WHERE user_id = 123;
```

**示例输出**:

```
id   | content        | status   | review_level | openclaw_response
-----|----------------|----------|--------------|-------------------------------------
101  | 你好世界       | approved | 4            | {"allowed":true,"score":0.1,...}
102  | [不当言论]    | rejected | 4            | {"allowed":false,"score":0.92,...}
```

---

## 🔐 安全考虑

### API 密钥管理

✅ **好的做法**:
```bash
# 使用环境变量
export OPENCLAW_API_KEY="your-secret-key"

# 或使用 .env 文件 (添加到 .gitignore)
echo "OPENCLAW_API_KEY=..." >> .env
echo ".env" >> .gitignore
```

❌ **不好的做法**:
```cpp
// 硬编码密钥 (危险!)
const char* API_KEY = "oc_sk_live_...";  // 不要这样做!
```

### 日志中的敏感信息

✅ **正确处理**:
```cpp
LOG_INFO("Openclaw API 已初始化 (隐藏密钥)");
// 不要在日志中打印完整的 API 密钥
```

❌ **错误处理**:
```cpp
LOG_DEBUG("API Key: " + apiKey);  // 危险! 会泄露密钥
```

---

## 🎯 常见问题 (FAQ)

### Q1: Openclaw 完全不工作，消息无法发送

**A**: 检查以下项:
1. API 密钥是否正确设置在 `.env` 或 `config.yaml`
2. 应用是否重启后生效
3. 查看日志是否有错误: `grep -i "openclaw" logs/app.log`
4. 临时禁用 Openclaw，确认降级方案能否工作

### Q2: Openclaw 响应太慢

**A**: 
1. 调整超时时间 (在 `config.yaml` 中)
   ```yaml
   openclaw:
     timeout_seconds: 15  # 增加到 15 秒
   ```
2. 使用批量 API 而不是单个审查
3. 考虑缓存审查结果

### Q3: 如何测试 Openclaw 没有真实的 API 密钥？

**A**: 使用 Mock 或降级模式:
```cpp
// 在 MessageService 中
if (!opencalwClient || !opencalwClient->isAvailable()) {
    // 使用启发式检查
    return aiContentReview(message, userId);
}
```

### Q4: 如何验证 Openclaw 是否正在被使用？

**A**: 查看日志和消息表:
```bash
# 方法 1: 查看应用日志
tail -f logs/app.log | grep "内容审查完成"

# 方法 2: 查询数据库
psql yachiyo_cpp -c "SELECT * FROM messages WHERE openclaw_response IS NOT NULL LIMIT 5;"
```

### Q5: 能否同时使用多个 AI 服务？

**A**: 当前架构支持一个，但可以扩展:
```cpp
// 未来改进: 支持多个提供商
struct AIReviewConfig {
    std::string primary = "openclaw";
    std::string fallback = "internal_heuristic";
};
```

---

## 🚀 下一步

### 现在就能做

1. ✅ 获取 Openclaw API 密钥
2. ✅ 在 `.env` 或 `config.yaml` 中配置
3. ✅ 重启后端服务
4. ✅ 测试新消息是否使用 Openclaw

### 短期改进 (v2.1)

- [ ] 实现审查结果缓存 (Redis)
- [ ] 添加 Openclaw 速率限制监控
- [ ] 实现指数退避重试策略
- [ ] 添加更详细的监控和告警

### 长期规划 (v2.2+)

- [ ] 支持多个 AI 提供商
- [ ] 本地模型作为高级降级方案
- [ ] 人工审查反馈循环
- [ ] 多语言支持

---

## 📞 联系和支持

- **Openclaw 文档**: https://docs.openclaw.io
- **Openclaw API 参考**: https://api.openclaw.io/docs
- **项目文档**: 参见 `OPENCLAW_INTEGRATION.md` 和 `API_INTEGRATION_GUIDE.md`

---

## 📋 集成检查清单

```
□ 获得 Openclaw API 密钥和 URL
□ 在 .env 或 config.yaml 中配置密钥
□ 验证后端日志中是否初始化成功
□ 发送测试消息验证审查工作
□ 检查数据库中的 openclaw_response 字段
□ 配置监控和告警
□ 在生产环境前完成压力测试
□ 文档化 API 密钥管理流程
```

---

**总结**: Yachiyo 已经完全准备好集成 Openclaw！只需要 API 密钥，其余代码都已完成。✅

