# OpenClaw 集成修正指南 🦞

> **更新日期**: 2026年4月1日  
> **修正内容**: OpenClaw 不是内容审查工具，而是自主 AI 虚拟助理框架
> **架构变更**: 从单向 API 调用改为双向集成

---

## 🔴 原有错误理解

之前的实现将 OpenClaw 错误地用作**内容审查层 (Layer 4)**，这是不对的。

```
❌ 错误的架构：
用户消息 → Yachiyo 后端 → OpencalwClient (审查内容) → 返回风险分数 → 显示结果
```

---

## ✅ 正确的 OpenClaw 定位

OpenClaw 是**自主 AI 虚拟助理框架**，核心功能：

| 功能 | 说明 | 应用场景 |
|-----|------|--------|
| **自主任务执行** | 无需等待用户指令，主动执行操作 | 虚拟主播主动管理日程、回复粉丝 |
| **多渠道集成** | WhatsApp、Telegram、Discord、iMessage | 虚拟主播在多个平台同时活动 |
| **本地存储** | 配置和交互历史存储在本地设备 | 虚拟主播个性化数据、记忆 |
| **AI 模型调用** | 可调用 Claude、GPT、DeepSeek 等 | 提升虚拟主播的 AI 能力 |
| **自动化技能** | Skills 系统用于扩展功能 | 虚拟主播执行复杂工作流 |
| **持久化记忆** | 24/7 记忆上下文 | 虚拟主播持久性记忆用户信息 |

---

## 🏗️ 修正后的架构设计

### 当前 Yachiyo 架构（内容审查应保持）

```
┌─────────────────────────────────────────────────────┐
│                 Yachiyo 聊天系统                      │
└─────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────┐
│              6层内容审查系统 (保留)                   │
│  ┌─────────────────────────────────────────────┐   │
│  │ Layer 1: 速率限制 (Redis)                   │   │
│  │ Layer 2: IP 黑名单 (PostgreSQL)             │   │
│  │ Layer 3: 敏感词过滤 (内存)                  │   │
│  │ Layer 4: ??? (需要修正)                     │   │
│  │ Layer 5: 行为异常检测                       │   │
│  │ Layer 6: 人工审核标记                       │   │
│  └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

### 新增 OpenClaw 集成层（异步、独立）

```
┌──────────────────────────────────────────────────────┐
│              OpenClaw 虚拟助理框架                    │
│         (独立部署在用户本地或服务器)                 │
└──────────────────────────────────────────────────────┘
       ↑                                      ↓
    数据同步                              自主执行
       ↑                                      ↓
┌──────────────────────────────────────────────────────┐
│           Yachiyo 后端 <→ OpenClaw Gateway           │
│                                                      │
│  • 用户交互数据同步                                 │
│  • 虚拟主播状态更新                                 │
│  • 自动化任务触发                                   │
│  • 多平台消息转发                                   │
└──────────────────────────────────────────────────────┘
         ↑                                  ↓
         │                                  │
    用户消息                        OpenClaw 自主行为
    用户交互
    粉丝互动
```

---

## 🔄 三层集成模式

### 模式 1: 虚拟主播个性化管理

**场景**: 虚拟主播在 Yachiyo 平台收到粉丝消息后，OpenClaw 自动处理回复

```
1. 粉丝在 Yachiyo 发送消息
   └─→ Yachiyo 存储消息到 PostgreSQL

2. OpenClaw 定期同步新消息
   └─→ 从 Yachiyo API 获取未处理的粉丝消息

3. OpenClaw 自主处理
   └─→ 使用 Claude/GPT 生成个性化回复
   └─→ 考虑虚拟主播的设定和风格
   └─→ 可选：转发到 Telegram/Discord 粉丝群

4. 回复发送回 Yachiyo
   └─→ 通过 API 更新聊天记录
   └─→ 显示在虚拟主播的"已回复"消息中
```

### 模式 2: 跨平台虚拟主播活动

**场景**: 虚拟主播在多个平台同时进行营销活动

```
1. 在 Yachiyo 创建新活动
   └─→ 设置时间、目标、内容

2. OpenClaw 自动发布到多渠道
   └─→ WhatsApp: 向粉丝发送活动通知
   └─→ Telegram: 发布活动详情
   └─→ Discord: 创建活动讨论频道
   └─→ iMessage: 发送私人邀请

3. 收集反馈并同步回 Yachiyo
   └─→ 反应统计
   └─→ 参与度数据
   └─→ 用户评论
```

### 模式 3: 持久化虚拟主播记忆

**场景**: 虚拟主播记住粉丝信息，提供个性化服务

```
1. 粉丝交互数据存储
   ├─ Yachiyo: 聊天历史
   ├─ PostgreSQL: 用户档案
   └─ OpenClaw: 本地持久化记忆

2. OpenClaw 学习和更新
   └─→ 分析粉丝偏好
   └─→ 记录重要信息
   └─→ 个性化推荐

3. 下次交互时使用记忆
   └─→ "嗨，我记得你喜欢 K-pop!"
   └─→ "上次你提到要看新剧..."
   └─→ 提升粉丝满意度
```

---

## 📋 修正后的内容审查方案

### Layer 4 替代方案

现有的错误层级应该改为以下之一：

#### 方案 A: 使用第三方内容审查 API（推荐）

```cpp
// 使用真正的内容审查服务，如：
// - Perspective API (Google)
// - Azure Content Moderator
// - OpenAI Moderation
// - 阿里云内容审查

struct ContentModerationResult {
    bool isAllowed;           // 是否通过审查
    double toxicityScore;     // 毒性分数 (0.0-1.0)
    double spamScore;         // 垃圾内容分数
    std::vector<std::string> categories;  // 违规类别
    std::string service;      // "perspective" / "azure" / etc
};

class ContentModerationClient {
public:
    Result<ContentModerationResult> moderate(const std::string& text);
    Result<std::vector<ContentModerationResult>> batchModerate(
        const std::vector<std::string>& texts
    );
};
```

#### 方案 B: 本地开源模型

```cpp
// 使用本地托管的开源模型
// - detoxify (Python 模型)
// - transformers (Hugging Face)
// - BERT 微调模型

struct LocalModerationClient {
    // 调用本地 Python 服务
    Result<ContentModerationResult> moderate(const std::string& text);
};
```

#### 方案 C: 组合方案（多层防御）

```
Layer 3: 敏感词库 (本地快速) ✅
    └─→ 如果触发，进入 Layer 4

Layer 4: 本地 AI 模型 (快速、准确) ✅
    └─→ 使用 detoxify 或 BERT
    └─→ 如果不确定，进入 Layer 5

Layer 5: 远程 API (准确度最高) ✅
    └─→ 使用 Perspective API
    └─→ 结果缓存到 Redis

Layer 6: 人工审核 (最终审核) ✅
    └─→ 高风险内容标记
    └─→ 人工审核团队处理
```

---

## 🔧 实施步骤

### 第一阶段：移除错误的 OpencalwClient

```bash
# 删除不正确的文件
rm -f YachiyoCPP/include/utils/OpencalwClient.hpp
rm -f YachiyoCPP/src/utils/OpencalwClient.cpp
rm -f YachiyoCPP/OPENCLAW_INTEGRATION.md
rm -f YachiyoCPP/WEB_OPENCLAW_INTEGRATION_QUICK_REFERENCE.md
```

### 第二阶段：选择正确的内容审查方案

```
推荐顺序：
1️⃣ 先用本地敏感词库 + BERT (成本最低、速度快)
2️⃣ 再添加 Perspective API (精准度最高)
3️⃣ 最后人工审核 (精细化处理)
```

### 第三阶段：集成 OpenClaw Gateway

```bash
# 单独部署 OpenClaw
npm install -g openclaw@latest
openclaw onboard --install-daemon

# 配置 Yachiyo ↔ OpenClaw 连接
# 在 config.yaml 中添加：
openclaw:
  gateway_url: "http://localhost:18789"
  api_key: "your_key"
  enabled: true
  channels:
    - whatsapp
    - telegram
    - discord
```

---

## 📡 Yachiyo ↔ OpenClaw 集成 API

### 1. 同步用户交互数据

```cpp
// POST /api/openclaw/sync-chats
// 发送聊天历史给 OpenClaw
struct SyncRequest {
    std::string userId;           // 虚拟主播 ID
    std::vector<Message> messages; // 最近消息
    std::string timeRange;         // "1h" / "1d" / "1w"
};

Response sync_chats(const SyncRequest& req) {
    // 通过 REST API 将消息发送到 OpenClaw
    POST /gateway/sync {
        "profile_id": "yachiyo_avatar_123",
        "messages": [...],
        "metadata": {...}
    }
    return success;
}
```

### 2. 接收 OpenClaw 的自动化操作

```cpp
// POST /api/openclaw/webhook
// OpenClaw 推送的事件
struct OpenClawEvent {
    std::string eventType;  // "message_sent" / "action_completed"
    std::string profile;    // 虚拟主播
    json metadata;          // 事件详情
};

void handle_openclaw_event(const OpenClawEvent& event) {
    if (event.eventType == "message_sent") {
        // 保存自动回复的消息
        save_to_chat_history(event);
    } else if (event.eventType == "broadcast_completed") {
        // 更新活动状态
        update_campaign_status(event);
    }
}
```

### 3. 请求 OpenClaw 执行特定任务

```cpp
// POST /api/openclaw/execute-task
// 让 OpenClaw 执行虚拟主播任务
struct TaskRequest {
    std::string task;      // "send_greeting" / "broadcast_news"
    json parameters;       // 任务参数
};

Result<json> execute_task(const TaskRequest& req) {
    auto response = curl_post("http://localhost:18789/execute", {
        "task": req.task,
        "params": req.parameters,
        "profile": current_avatar_id
    });
    return parse_response(response);
}
```

---

## 📊 修正后的系统架构图

```
┌─────────────────────────────────────┐
│   用户 App (iOS/Android/Web)        │
│     与虚拟主播交互                  │
└──────────────┬──────────────────────┘
               │
               ↓
┌─────────────────────────────────────┐
│      Yachiyo 后端 (C++20)            │
│                                     │
│  ┌────────────────────────────────┐ │
│  │  6层内容审查系统(正确实现)     │ │
│  │  Layer 1-3: 本地快速检查       │ │
│  │  Layer 4: Perspective API      │ │
│  │  Layer 5-6: 人工审核           │ │
│  └────────────────────────────────┘ │
│                                     │
│  ┌────────────────────────────────┐ │
│  │  OpenClaw 网关接口             │ │
│  │  - 数据同步 API                │ │
│  │  - Webhook 接收                │ │
│  │  - 任务执行接口                │ │
│  └────────────────────────────────┘ │
└──────────────┬──────────────────────┘
               │
        ┌──────┴──────┐
        ↓             ↓
  PostgreSQL      Redis
  (聊天存储)     (缓存)
        
        ↓
┌─────────────────────────────────────┐
│   OpenClaw Gateway (独立部署)        │
│                                     │
│  ┌────────────────────────────────┐ │
│  │  虚拟主播代理 Profile          │ │
│  │  - 个性化设定                  │ │
│  │  - 本地持久化记忆              │ │
│  │  - 自动化 Skills               │ │
│  └────────────────────────────────┘ │
│                                     │
│  ┌────────────────────────────────┐ │
│  │  多渠道集成                     │ │
│  │  - WhatsApp                    │ │
│  │  - Telegram                    │ │
│  │  - Discord                     │ │
│  │  - iMessage                    │ │
│  └────────────────────────────────┘ │
│                                     │
│  ┌────────────────────────────────┐ │
│  │  AI 模型接口                   │ │
│  │  - Claude (Anthropic)          │ │
│  │  - GPT (OpenAI)                │ │
│  │  - DeepSeek                    │ │
│  └────────────────────────────────┘ │
└─────────────────────────────────────┘
```

---

## ✨ 完成检查清单

- [ ] 移除错误的 OpencalwClient 代码
- [ ] 选择实际的内容审查方案（Perspective API 或本地模型）
- [ ] 实现正确的 Layer 4 审查层
- [ ] 创建 OpenClaw 网关连接接口
- [ ] 测试虚拟主播 → OpenClaw 数据同步
- [ ] 测试 OpenClaw 自动化回复功能
- [ ] 验证多平台消息同步
- [ ] 更新文档和 API 文档

---

## 📚 参考资源

### OpenClaw 官方文档
- 网站: https://openclaw.ai/
- GitHub: https://github.com/openclaw/openclaw
- 文档: https://docs.openclaw.ai/

### 内容审查替代方案
- **Perspective API**: https://perspectiveapi.com/
- **Detoxify**: https://github.com/unitaryai/detoxify
- **阿里云内容审查**: https://www.aliyun.com/product/viapi

### 相关概念
- [AI Agent 框架](https://openclaw.ai/)
- [自主代理模式](https://en.wikipedia.org/wiki/Autonomous_agent)
- [内容审查 API 对比](https://perspectiveapi.com/)

---

## 🎯 总结

| 方面 | 之前（错误） | 现在（正确） |
|------|------------|----------|
| **定位** | 内容审查工具 | 自主虚拟助理框架 |
| **功能** | 返回风险分数 | 执行自动化任务 |
| **部署** | 后端集成 | 独立部署 |
| **用途** | 检查消息内容 | 管理虚拟主播行为 |
| **集成方式** | 直接 API 调用 | 网关 + 数据同步 |

**下一步**: 选择正确的内容审查方案，移除错误的 OpenClaw 代码，并重新设计 OpenClaw 集成！
