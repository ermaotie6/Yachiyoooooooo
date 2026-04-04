# Openclaw 集成 - 5 分钟快速指南

## 🎯 一句话总结

Yachiyo 已经完全集成了 Openclaw 框架，**只需要 3 步配置 API 密钥，就能启用 AI 内容审查**。

---

## ⚡ 3 步启动 Openclaw

### 第 1 步: 获取 API 密钥 (3 分钟)

```
前往 Openclaw 官网
    ↓
注册账户 (或使用现有账户)
    ↓
创建 API 密钥
    ↓
复制以下信息:
- API_KEY: oc_sk_live_xxxx...
- API_URL: https://api.openclaw.io
```

### 第 2 步: 配置环境变量 (1 分钟)

**方案 A: 编辑 `.env` 文件** (推荐)

```bash
# Yachiyo/.env
OPENCLAW_API_KEY=oc_sk_live_12345678901234567890
OPENCLAW_API_URL=https://api.openclaw.io
```

**方案 B: 编辑 `config.yaml` 文件**

```yaml
# YachiyoCPP/config/config.yaml
openclaw:
  enabled: true
  api_key: "oc_sk_live_12345678901234567890"
  api_url: "https://api.openclaw.io"
  timeout_seconds: 10
  max_retries: 3
```

### 第 3 步: 重启服务 (1 分钟)

```bash
# 如果用 Docker
docker-compose restart yachiyo-cpp

# 如果本地运行
pkill -f Application
./src/Application
```

**完成！** 🎉

---

## ✅ 验证集成成功

### 方法 1: 查看日志

```bash
# 查看是否成功初始化
tail -f logs/app.log | grep openclaw

# 预期输出:
# INFO 初始化OpencalwClient: https://api.openclaw.io
```

### 方法 2: 发送测试消息

```bash
# 登录获取 Token
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser","password":"Pwd123!"}'

# 复制返回的 token

# 发送消息 (会自动经过 Openclaw 审查)
curl -X POST http://localhost:8080/api/v1/messages/send \
  -H "Authorization: Bearer <your-token>" \
  -H "Content-Type: application/json" \
  -d '{"content":"这是一条测试消息"}'

# 检查响应中的 openclaw_response 字段
```

### 方法 3: 查看数据库

```bash
# 检查消息表中是否有 openclaw_response
psql yachiyo_cpp -c "
  SELECT id, content, status, openclaw_response 
  FROM messages 
  WHERE openclaw_response IS NOT NULL 
  LIMIT 1;
"

# 如果有结果，说明集成成功！
```

---

## 🏗️ 架构 (图示)

```
┌─────────────────────────────────────────────────────────┐
│                    用户发送消息                          │
└─────────────┬───────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│            MessageService::sendMessage()                │
│                                                         │
│  6 层审查系统                                            │
│  Layer 1: 速率限制 ✅                                    │
│  Layer 2: IP 黑名单 ✅                                   │
│  Layer 3: 敏感词 ✅                                      │
│  Layer 4: AI 审查 🤖 ← Openclaw 在这里！                │
│  Layer 5: 行为异常 ✅                                    │
│  Layer 6: 人工标记 ✅                                    │
└─────────────┬───────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│              OpencalwClient (Layer 4)                    │
│                                                         │
│  reviewContent(message) 或 batchReview(messages)        │
└─────────────┬───────────────────────────────────────────┘
              │
              ▼
    ┌─────────────────┐
    │  Openclaw API   │
    │                 │
    │ POST /review/   │
    │ text or batch   │
    └────────┬────────┘
             │
             ▼
    ┌─────────────────┐
    │ {allowed, score │
    │  categories}    │
    └────────┬────────┘
             │
              ▼
┌─────────────────────────────────────────────────────────┐
│  审查决策                                               │
│                                                         │
│  if score < 0.3:      APPROVED ✅                       │
│  if 0.3 < score < 0.6: MANUAL_REVIEW 📋               │
│  if score >= 0.6:      REJECTED ❌                      │
└─────────────┬───────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  保存到数据库                                           │
│  - messages.status = "approved/pending/rejected"       │
│  - messages.openclaw_response = { ... }                │
└─────────────────────────────────────────────────────────┘
```

---

## 📊 工作流程详解

### 当 Openclaw 正常工作时 ✅

```
消息 → 6层审查 → Openclaw API 调用 → 返回风险评分
                    ↓
                (< 0.3)      (0.3-0.6)      (> 0.6)
                  ↓            ↓              ↓
              批准 ✅     人工审查📋       拒绝 ❌
```

### 当 Openclaw 不可用时 ⚠️ (自动降级)

```
消息 → 6层审查 → Openclaw 不可用
                    ↓
            降级到启发式检查
                    ↓
            (简化风险评分)
                    ↓
            继续处理 (其他5层)
```

**关键**: 即使 Openclaw 故障，消息仍然会被处理！

---

## 📁 核心文件位置

```
YachiyoCPP/
├── include/utils/OpencalwClient.hpp       ← 接口定义
├── src/utils/OpencalwClient.cpp           ← 实现 (CURL HTTP 请求)
├── config/config.yaml                     ← 配置文件 (放 API Key 这里)
├── .env.example                           ← 环境变量示例
└── src/services/MessageService.cpp        ← 调用 Openclaw 的地方
```

**最重要的**:
1. 配置文件: `config/config.yaml`
2. 客户端类: `OpencalwClient.hpp` / `.cpp`
3. 使用处: `MessageService.cpp` 中的 `sendMessage()` 方法

---

## 🔌 Openclaw API 调用示例

### 单个消息审查

```cpp
auto client = std::make_shared<OpencalwClient>(
    "oc_sk_live_xxx...",                // API Key
    "https://api.openclaw.io"           // API URL
);

// 审查消息
auto result = client->reviewContent(
    "你好，这是一条消息",
    12345                               // 用户 ID
);

if (result.isSuccess()) {
    auto review = result.getData();
    std::cout << "允许发送: " << review.isAllowed << std::endl;
    std::cout << "风险评分: " << review.riskScore << std::endl;
    std::cout << "违规类别: " << review.mainCategory << std::endl;
}
```

### 批量审查 (效率更高)

```cpp
// 多条消息一起审查
std::vector<std::pair<int64_t, std::string>> messages = {
    {1, "消息1"},
    {2, "消息2"},
    {3, "消息3"}
};

auto result = client->batchReview(messages);

if (result.isSuccess()) {
    for (const auto& review : result.getData()) {
        std::cout << "结果: " << review.isAllowed << std::endl;
    }
}
```

### 健康检查

```cpp
auto healthResult = client->healthCheck();
if (healthResult.isSuccess()) {
    std::cout << "Openclaw API 可用!" << std::endl;
} else {
    std::cout << "Openclaw API 不可用，将使用降级方案" << std::endl;
}
```

---

## 🚨 常见问题

### ❓ 如果 API Key 配置错误会怎样？

```
❌ 错误的行为:
- Openclaw API 返回 401 Unauthorized
- 日志显示: "HTTP 401 Unauthorized"
- 系统自动降级到启发式检查
- 消息仍然可以发送，但准确度降低

✅ 解决: 检查 API Key 是否正确拷贝
```

### ❓ Openclaw API 超时了怎么办？

```
⚠️ 情况:
- 网络延迟导致请求超时
- Openclaw 服务响应慢
- 触发重试机制 (最多 3 次)

✅ 解决:
1. 增加超时时间: timeout_seconds: 15
2. 检查网络连接
3. 检查 Openclaw 服务状态
4. 系统会自动降级
```

### ❓ 如何验证 Openclaw 在生产环境中工作？

```bash
# 1. 检查日志
docker logs yachiyo-cpp | grep -i openclaw

# 2. 查询数据库
SELECT COUNT(*) FROM messages WHERE openclaw_response IS NOT NULL;

# 3. 监控 Openclaw API 请求数
# (在 Openclaw 的管理面板中查看)
```

---

## 📈 性能指标

### 预期性能

```
单个消息审查:
- 响应时间: 200-500ms
- 成功率: > 99.9%

批量审查 (10条):
- 响应时间: 500-1000ms
- 成功率: > 99.9%

降级启发式检查:
- 响应时间: < 10ms
- 精确度: 中等 (~70%)
```

### 监控

```yaml
# Prometheus 指标
openclaw_request_duration_seconds   # API 响应时间
openclaw_request_failures_total     # 失败次数
openclaw_requests_total             # 总请求数
openclaw_fallback_uses_total        # 降级使用次数
```

---

## 🎓 完整集成检查清单

```
□ 步骤 1: 获取 Openclaw API 密钥
  □ 访问 https://openclaw.io
  □ 创建 API 密钥
  □ 记下 API_KEY 和 API_URL

□ 步骤 2: 配置到应用
  □ 编辑 .env 或 config.yaml
  □ 设置 OPENCLAW_API_KEY
  □ 设置 OPENCLAW_API_URL

□ 步骤 3: 重启服务
  □ 重启后端应用
  □ 查看日志确认初始化

□ 步骤 4: 测试
  □ 发送测试消息
  □ 检查日志
  □ 查询数据库

□ 步骤 5: 监控
  □ 配置告警规则
  □ 添加 Grafana 仪表板
  □ 定期检查性能指标
```

---

## 🔗 参考文档

- **完整集成指南**: `OPENCLAW_QUICK_INTEGRATION.md`
- **API 参考**: `API_INTEGRATION_GUIDE.md`
- **Phase 3 文档**: `OPENCLAW_INTEGRATION.md`
- **Openclaw 官方**: https://docs.openclaw.io

---

## ✨ 最后

**你已经拥有一个生产级别的 AI 内容审查系统！** 🎉

- ✅ 6 层防护
- ✅ Openclaw AI 集成
- ✅ 自动降级方案
- ✅ 完整的代码实现
- ✅ 详细的文档

**现在只需要配置 API 密钥就能启用 AI 审查！**

---

**下一步**: 获取 Openclaw API 密钥 → 配置 → 重启 → 完成! 🚀
