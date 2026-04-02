# OpenClaw 快速参考卡 (Quick Reference)

> 为 OpenClaw 代理提供的快速查询表  
> 打印或放在 OpenClaw 配置目录中

---

## 🎯 虚拟主播 Yachiyo 的 5 个主要任务

| 任务 | 触发方式 | 做什么 | 时间 |
|-----|--------|------|------|
| 1. **粉丝互动** | 新消息到达 | 生成个性化回复 | 即时 (< 5 秒) |
| 2. **定时公告** | 按计划 | 发送直播提醒、感谢消息 | 定时 |
| 3. **数据报告** | 每天/每周 | 生成互动统计报告 | 后台处理 |
| 4. **社区管理** | 持续监控 | 审核内容、处理投诉 | 实时 |
| 5. **平台同步** | 消息或事件 | 将消息发送到多个平台 | 同步 |

---

## 🔑 关键配置项

### 1. 虚拟主播身份 (Profile)

```
ID: yachiyo_avatar_001
名字: Yachiyo
性格: 友好、幽默、充满活力
语言: 简体中文 (主) + 英文 (次)
平台: Discord, Telegram, WhatsApp, Twitter, Instagram
```

### 2. API 密钥需求

```bash
必需:
  ✓ ANTHROPIC_API_KEY       # Claude AI
  ✓ YACHIYO_API_KEY         # Yachiyo 后端认证
  
可选:
  □ DISCORD_BOT_TOKEN       # Discord 集成
  □ TELEGRAM_BOT_TOKEN      # Telegram 集成
  □ WHATSAPP_API_KEY        # WhatsApp 集成
  □ TWITTER_API_KEY         # Twitter 集成
```

### 3. 网址和端口

```
OpenClaw 网关: http://localhost:18789
Yachiyo 后端: http://localhost:8080
配置目录: ~/.openclaw/
任务定义: ~/.openclaw/tasks/
日志文件: ~/.openclaw/openclaw.log
```

---

## ⚡ 快速命令

### 启动和停止

```bash
# 启动 OpenClaw
openclaw daemon start

# 后台启动（带日志）
nohup openclaw daemon start > ~/.openclaw/openclaw.log 2>&1 &

# 停止 OpenClaw
openclaw daemon stop

# 检查状态
openclaw status

# 通过 systemd
systemctl start openclaw
systemctl stop openclaw
systemctl restart openclaw
```

### 日志和调试

```bash
# 实时日志
openclaw logs -f

# 特定级别日志
openclaw logs -f --level=error

# 查看特定模块
openclaw logs -f --module=yachiyo

# 通过 systemd 日志
journalctl -u openclaw -f
```

### 验证和测试

```bash
# 健康检查
curl http://localhost:18789/health

# 检查配置
cat ~/.openclaw/openclaw.json | jq .

# 测试 Yachiyo 连接
curl -X POST http://localhost:18789/api/yachiyo/sync-messages \
  -H "Content-Type: application/json" \
  -d '{"profile_id":"yachiyo_avatar_001","messages":[]}'

# 测试消息回复
curl -X POST http://localhost:18789/api/test/reply \
  -H "Content-Type: application/json" \
  -d '{"message":"Hi Yachiyo!"}'
```

---

## 📝 常见任务的 API 调用

### 1. 发送粉丝消息到 OpenClaw

```bash
POST /api/yachiyo/sync-messages
Content-Type: application/json

{
  "profile_id": "yachiyo_avatar_001",
  "messages": [
    {
      "id": 123,
      "from": "fan_user_001",
      "content": "Hi Yachiyo!",
      "platform": "discord",
      "timestamp": 1704067200
    }
  ]
}
```

### 2. 请求 OpenClaw 执行任务

```bash
POST /api/yachiyo/execute-task
Content-Type: application/json

{
  "profile_id": "yachiyo_avatar_001",
  "task": "send_announcement",
  "params": {
    "message": "新直播即将开始！",
    "channels": ["discord", "telegram"],
    "schedule_time": 1704067800
  }
}
```

### 3. 接收 OpenClaw 的回复（Webhook）

```bash
POST /api/openclaw/webhook
Content-Type: application/json

{
  "event_type": "message_sent",
  "profile_id": "yachiyo_avatar_001",
  "to_user": "fan_user_001",
  "message": "谢谢你的消息！😊",
  "platform": "discord"
}
```

---

## 🔧 配置快速修改

### 启用新平台

编辑 `~/.openclaw/openclaw.json`:

```json
{
  "channels": {
    "telegram": {
      "enabled": true,
      "botToken": "YOUR_TELEGRAM_BOT_TOKEN"
    }
  }
}
```

然后重启:
```bash
systemctl restart openclaw
```

### 修改回复风格

编辑 `~/.openclaw/tasks/yachiyo_profile.yaml`:

```yaml
personality:
  tone: "friendly, humorous"  # 修改这里
  language: "Simplified Chinese"
```

### 调整速率限制

编辑 `~/.openclaw/openclaw.json`:

```json
{
  "security": {
    "rate_limit": {
      "requests_per_minute": 100  # 增加这个数字
    }
  }
}
```

---

## ⚠️ 常见问题快速解决

| 问题 | 原因 | 解决方案 |
|-----|------|--------|
| Connection refused | OpenClaw 未启动 | `openclaw daemon start` |
| 401 Unauthorized | API 密钥错误 | 检查 `ANTHROPIC_API_KEY` |
| 503 Service Unavailable | 模型不可用 | 检查 API 配额、网络连接 |
| Webhook timeout | Yachiyo 后端无响应 | 检查防火墙、后端日志 |
| 消息重复 | 消息不幂等 | 检查消息 ID 实现 |
| 内存持续增长 | 内存泄漏 | 重启 OpenClaw |

---

## 📊 监控检查清单 (每日)

```
□ OpenClaw 运行正常: openclaw status
□ 没有错误日志: openclaw logs | grep ERROR
□ Yachiyo 后端可连接: curl http://localhost:8080/health
□ API 密钥有效: 检查最近的请求日志
□ 内存使用正常: openclaw stats | grep memory
□ 粉丝互动正常: 查看最近的消息日志
□ 没有待处理错误: 检查错误队列
□ 备份任务配置: cp ~/.openclaw/tasks/*.yaml backup/
```

---

## 🚀 性能优化提示

```
优化前: 平均回复时间 8 秒
优化后: 平均回复时间 2 秒

方法:
1. 使用 claude-haiku 替代 claude-opus (更快，稍微低精度)
2. 启用消息缓存: cache_responses: true
3. 增加并发限制: concurrency: 5
4. 预加载用户档案: preload_profiles: true
5. 启用本地模型: use_local_model: true
```

---

## 📞 支持和文档链接

| 资源 | 链接 |
|------|------|
| OpenClaw 文档 | https://docs.openclaw.ai/ |
| Claude API 指南 | https://docs.anthropic.com/ |
| Yachiyo 项目 | 本项目根目录 |
| 完整集成指南 | OPENCLAW_INTEGRATION_GUIDE.md |
| 任务定义 | OPENCLAW_VIRTUAL_AVATAR_TASKS.md |

---

## 🎯 快速部署清单

新服务器部署: ✅ 使用脚本

```bash
# 下载脚本
curl -O https://your-repo/deploy_openclaw_ubuntu.sh

# 运行脚本
sudo bash deploy_openclaw_ubuntu.sh

# 配置你的 API 密钥
nano ~/.openclaw/openclaw.json

# 启动
openclaw daemon start

# 验证
curl http://localhost:18789/health
```

---

## 💾 备份和恢复

```bash
# 备份所有配置
tar -czf openclaw_backup.tar.gz ~/.openclaw/

# 恢复配置
tar -xzf openclaw_backup.tar.gz

# 备份日志
cp ~/.openclaw/openclaw.log openclaw_$(date +%Y%m%d).log

# 导出任务定义
openclaw export-tasks tasks_backup.yaml
```

---

**打印此卡或保存到 `~/.openclaw/QUICK_REFERENCE.md` 以便快速查询！**
