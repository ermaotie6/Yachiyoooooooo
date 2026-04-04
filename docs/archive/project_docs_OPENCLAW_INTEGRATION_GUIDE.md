# Yachiyo × OpenClaw 集成指南 🦞

> **目标**: 将 Yachiyo 虚拟主播平台与 OpenClaw 自主 AI 代理框架集成  
> **对象**: 在 Ubuntu 中部署 OpenClaw，让虚拟主播代理自动执行任务  
> **时间**: 30 分钟快速集成

---

## 📋 概览

### 什么是这个集成？

```
Yachiyo (虚拟主播平台)
    ↓ 用户交互数据
    ↓
OpenClaw Gateway (自主 AI 代理框架)
    ↓
✅ 自动回复粉丝消息
✅ 定时发送更新
✅ 跨平台消息转发
✅ 持久化虚拟主播人格
```

### 核心功能

| 功能 | 说明 | 流程 |
|-----|------|------|
| **虚拟主播自动回复** | 粉丝消息 → OpenClaw → 自动生成回复 | 异步处理 |
| **定时任务** | 安排公告、直播提醒 | 24/7 运行 |
| **多平台同步** | WhatsApp、Telegram、Discord | 一次配置 |
| **个性化记忆** | 虚拟主播记住粉丝信息 | 本地持久化 |

---

## 🚀 快速部署（Ubuntu）

### 前置条件

```bash
# 系统版本
Ubuntu 20.04 LTS 或更高版本

# Node.js 版本
node --version  # 需要 v24 或 v22.14+

# 检查 npm
npm --version
```

### 第一步：安装 OpenClaw

```bash
# 1. 安装 OpenClaw CLI
npm install -g openclaw@latest

# 2. 验证安装
openclaw --version
```

### 第二步：配置虚拟主播代理

```bash
# 1. 创建配置目录
mkdir -p ~/.openclaw
cd ~/.openclaw

# 2. 创建配置文件 (openclaw.json)
cat > ~/.openclaw/openclaw.json << 'EOF'
{
  "profile": {
    "name": "Yachiyo Avatar",
    "description": "Virtual streamer powered by AI",
    "personality": {
      "tone": "friendly",
      "language": "zh-CN",
      "interests": ["streaming", "gaming", "anime"]
    }
  },
  "channels": {
    "whatsapp": {
      "enabled": true,
      "allowFrom": []
    },
    "telegram": {
      "enabled": true,
      "botToken": "YOUR_TELEGRAM_BOT_TOKEN"
    },
    "discord": {
      "enabled": true,
      "token": "YOUR_DISCORD_BOT_TOKEN"
    }
  },
  "model": {
    "provider": "anthropic",
    "model": "claude-opus",
    "apiKey": "YOUR_ANTHROPIC_API_KEY"
  },
  "yachiyo": {
    "backend_url": "http://localhost:8080",
    "api_key": "YOUR_YACHIYO_API_KEY",
    "sync_interval": 300
  },
  "memory": {
    "type": "local",
    "maxSize": "10MB",
    "persistPath": "~/.openclaw/memory"
  }
}
EOF

# 3. 根据你的配置修改上面的参数
nano ~/.openclaw/openclaw.json
```

### 第三步：启动 OpenClaw

```bash
# 1. 后台启动（推荐）
openclaw daemon start

# 或前台启动（调试）
openclaw start

# 2. 检查状态
openclaw status

# 3. 查看日志
openclaw logs -f
```

### 第四步：验证连接

```bash
# 测试 OpenClaw 网关
curl http://localhost:18789/health

# 应该返回：
# {"status":"ok","version":"x.x.x"}
```

---

## 🔌 Yachiyo 后端集成

### API 端点配置

在 Yachiyo 后端的 `config.yaml` 中添加：

```yaml
openclaw:
  enabled: true
  gateway_url: "http://localhost:18789"
  api_key: "your_key"
  
  # 同步配置
  sync:
    enabled: true
    interval_seconds: 300  # 5分钟同步一次
    batch_size: 50
    
  # 回调配置
  webhook:
    enabled: true
    port: 8081
    path: "/api/openclaw/webhook"
```

### 创建同步接口

Yachiyo 需要以下 API 端点与 OpenClaw 通信：

#### 1. 发送聊天数据给 OpenClaw

```bash
# POST http://openclaw-gateway/api/yachiyo/sync-messages
{
  "profile_id": "yachiyo_avatar_001",
  "messages": [
    {
      "id": 12345,
      "from_user_id": 1001,
      "from_username": "fan_user",
      "content": "Hi! Love your streams!",
      "timestamp": 1704067200,
      "platform": "discord"
    }
  ],
  "metadata": {
    "total_count": 150,
    "unread_count": 5
  }
}
```

#### 2. 接收 OpenClaw 的自动回复

```bash
# 在 Yachiyo 后端实现 webhook
POST /api/openclaw/webhook
Content-Type: application/json

{
  "event_type": "message_sent",
  "profile_id": "yachiyo_avatar_001",
  "to_user_id": 1001,
  "message": "Thanks for watching! See you in the next stream!",
  "platform": "discord",
  "timestamp": 1704067300
}
```

#### 3. 请求 OpenClaw 执行特定任务

```bash
# POST http://openclaw-gateway/api/yachiyo/execute-task
{
  "profile_id": "yachiyo_avatar_001",
  "task": "broadcast_announcement",
  "params": {
    "message": "New stream starting in 1 hour!",
    "channels": ["discord", "telegram"],
    "schedule_time": 1704067500
  }
}
```

---

## 🎯 常见任务配置

### 任务 1: 虚拟主播自动回复

**目标**: 粉丝发消息 → 虚拟主播自动回复

**配置**:

```yaml
openclaw:
  skills:
    - name: "auto_reply"
      description: "自动回复粉丝消息"
      trigger: "on_new_message"
      handler: |
        {
          "model": "claude-opus",
          "system_prompt": "你是虚拟主播Yachiyo，以友好、幽默的风格回复粉丝。",
          "user_message": "{{message_content}}",
          "context": "{{user_history}}"
        }
      rate_limit: "1 reply per user per 5 minutes"
```

**工作流**:
```
1. 粉丝在Discord发消息
2. OpenClaw检测新消息
3. 调用Claude生成虚拟主播风格的回复
4. 发送回复给粉丝
5. 记录到Yachiyo数据库
```

### 任务 2: 定时公告

**目标**: 虚拟主播定时发送直播提醒

**配置**:

```yaml
openclaw:
  skills:
    - name: "schedule_reminder"
      description: "定时发送直播提醒"
      trigger: "cron:0 22 * * *"  # 每天晚上10点
      handler: |
        {
          "action": "broadcast",
          "channels": ["discord", "telegram", "whatsapp"],
          "message": "🎬 直播即将开始！感谢大家的支持！",
          "include_link": true
        }
```

### 任务 3: 粉丝互动统计

**目标**: 定期总结粉丝互动数据

**配置**:

```yaml
openclaw:
  skills:
    - name: "engagement_report"
      description: "生成粉丝互动报告"
      trigger: "cron:0 23 * * 0"  # 每周日晚上11点
      handler: |
        {
          "action": "generate_report",
          "data_source": "yachiyo_api",
          "metrics": [
            "total_messages",
            "unique_fans",
            "engagement_rate",
            "top_topics"
          ],
          "output": "send_to_admin"
        }
```

---

## 📊 监控和日志

### 查看 OpenClaw 日志

```bash
# 实时日志
openclaw logs -f

# 特定级别
openclaw logs -f --level=error

# 特定模块
openclaw logs -f --module=yachiyo
```

### 常见错误

| 错误 | 原因 | 解决方案 |
|-----|------|--------|
| `Connection refused` | OpenClaw 未启动 | 运行 `openclaw daemon start` |
| `API key invalid` | 密钥错误 | 检查 `.openclaw/openclaw.json` 中的密钥 |
| `Model not available` | AI 模型不可用 | 检查 API 配额和网络连接 |
| `Webhook timeout` | Yachiyo 后端未响应 | 检查防火墙和网络配置 |

### 性能监控

```bash
# 查看网关状态
openclaw status

# 查看内存使用
openclaw stats

# 查看连接数
openclaw connections
```

---

## 🔐 安全配置

### API 密钥管理

```bash
# 生成新的 API 密钥
openclaw generate-key

# 轮换密钥
openclaw rotate-key --profile=yachiyo_avatar_001

# 吊销密钥
openclaw revoke-key --key-id=xxx
```

### 网络安全

```yaml
# openclaw.json 中的安全配置
{
  "security": {
    "tls": {
      "enabled": true,
      "cert_path": "/etc/ssl/certs/openclaw.crt",
      "key_path": "/etc/ssl/private/openclaw.key"
    },
    "auth": {
      "type": "api_key",
      "validate_source_ip": true,
      "allowed_ips": [
        "127.0.0.1",
        "your_yachiyo_server_ip"
      ]
    },
    "rate_limit": {
      "requests_per_minute": 100,
      "burst_size": 200
    }
  }
}
```

---

## 📈 扩展功能

### 添加自定义 Skill

创建 `~/.openclaw/skills/yachiyo_assistant.skill`:

```javascript
// yachiyo_assistant.skill
module.exports = {
  name: "yachiyo_assistant",
  version: "1.0.0",
  
  // 触发条件
  triggers: {
    onMessage: async (context) => {
      const { message, user, platform } = context;
      
      // 自定义逻辑
      const response = await generateResponse(message, user);
      
      return {
        action: "send_message",
        target: user.id,
        content: response,
        platform: platform
      };
    },
    
    onCommand: async (context, command, args) => {
      if (command === "stats") {
        return {
          action: "send_message",
          content: await getEngagementStats(args)
        };
      }
    }
  },
  
  // 辅助函数
  async function generateResponse(message, user) {
    // 调用 Claude
    const response = await claudeAPI.generate({
      prompt: `用虚拟主播Yachiyo的风格回复: ${message}`,
      user_context: user.profile
    });
    return response.text;
  }
};
```

### 集成第三方服务

```yaml
openclaw:
  integrations:
    - name: "youtube"
      config:
        api_key: "YOUR_YOUTUBE_API_KEY"
        channel_id: "UC..."
        
    - name: "twitch"
      config:
        client_id: "YOUR_TWITCH_CLIENT_ID"
        oauth_token: "xxx"
        
    - name: "tiktok"
      config:
        access_token: "xxx"
```

---

## 📝 部署检查清单

在生产环境中部署前，验证以下内容：

### 基础设置
- [ ] Node.js v24 或 v22.14+ 已安装
- [ ] OpenClaw 已安装 (`npm install -g openclaw@latest`)
- [ ] 配置文件 `~/.openclaw/openclaw.json` 已创建
- [ ] 所有 API 密钥已配置

### Yachiyo 集成
- [ ] 后端已启动并运行在 `localhost:8080`
- [ ] Yachiyo API 密钥已配置
- [ ] Webhook 端点已实现
- [ ] 网络防火墙已配置允许端口 `18789`

### AI 模型
- [ ] Claude API 密钥已设置
- [ ] API 配额充足
- [ ] 模型选择正确（推荐 `claude-opus`）

### 多平台配置（可选）
- [ ] Telegram Bot Token 已获取
- [ ] Discord Bot Token 已获取
- [ ] WhatsApp Business API 已配置

### 监控和日志
- [ ] 日志输出正常
- [ ] 没有错误消息
- [ ] 性能指标正常
- [ ] Webhook 能成功调用

### 安全
- [ ] TLS/SSL 已启用
- [ ] API 密钥已加密存储
- [ ] IP 白名单已配置
- [ ] 速率限制已启用

---

## 🆘 故障排查

### 检查清单

```bash
# 1. 检查 OpenClaw 是否运行
openclaw status

# 2. 测试网关连通性
curl http://localhost:18789/health

# 3. 验证配置文件
cat ~/.openclaw/openclaw.json | jq .

# 4. 检查日志
openclaw logs -f --level=error

# 5. 测试 API 密钥
curl -H "Authorization: Bearer YOUR_API_KEY" \
  http://localhost:18789/api/profile

# 6. 检查 Yachiyo 连接
curl -X POST http://localhost:18789/api/yachiyo/sync-messages \
  -H "Content-Type: application/json" \
  -d '{"profile_id":"test","messages":[]}'
```

### 常见问题

**Q: OpenClaw 启动失败**
```bash
# 检查日志
openclaw logs

# 尝试重新安装
npm uninstall -g openclaw
npm install -g openclaw@latest

# 重新配置
openclaw onboard --reset
```

**Q: 无法连接到 Yachiyo**
```bash
# 检查网络
ping your_yachiyo_server

# 检查防火墙
sudo ufw allow 18789

# 检查配置中的 URL
grep "backend_url" ~/.openclaw/openclaw.json
```

**Q: AI 回复太慢**
```bash
# 检查模型选择
# 建议：claude-opus > claude-sonnet > claude-haiku

# 增加超时时间
# 在 openclaw.json 中添加：
"model": {
  "timeout_seconds": 30
}
```

---

## 🎓 学习资源

- **OpenClaw 官方文档**: https://docs.openclaw.ai/
- **Yachiyo 后端 API**: 本项目的 `/docs/api`
- **Claude API 指南**: https://docs.anthropic.com/

---

## 📞 支持

遇到问题？

1. **查看日志**: `openclaw logs -f`
2. **检查配置**: `cat ~/.openclaw/openclaw.json`
3. **查询文档**: `openclaw --help`
4. **提交问题**: 项目 GitHub Issues

---

## ✅ 下一步

完成部署后，你可以：

1. ✅ 测试虚拟主播自动回复
2. ✅ 配置定时任务
3. ✅ 添加多平台支持
4. ✅ 构建自定义 Skills
5. ✅ 监控和优化性能

**祝部署顺利！** 🚀
