# OpenClaw 对接说明

> 适用于 Yachiyo 虚拟直播平台

---

## 定位

本项目与 OpenClaw 的关联非常有限：**仅通过 Bridge 端口进行 JSON 交换**。

- 本项目 **不负责对话生成**。对话逻辑（prompt、模型选择、上下文管理等）全部由 OpenClaw 完成。
- 本项目的职责：弹幕审核 → 打包 JSON → 发给 OpenClaw → 接收回答 → 后处理（翻译、TTS、Live2D）。
- 没有直接代码耦合，没有 SDK 依赖，没有共享状态。

## 架构

> ⚠️ **重要说明**: 当前架构为 **纯同步模式**。不存在回调端口、不存在会话管理、不存在异步轮询。
> 如果你看到旧版设计图中标注了 `8766 回调端口` 或 `session` 管理，那是已废弃的旧方案。

桥接服务是一个极简的 Node.js HTTP 转发层，唯一职责就是**收 JSON → 转发 → 回 JSON**：

```
┌───────────┐   POST JSON    ┌──────────────┐   POST JSON    ┌──────────┐
│ C++ 后端  │ ──────────────→ │ 桥接服务     │ ──────────────→ │ OpenClaw │
│           │                 │ (:8765)      │                 │ (:8000)  │
│           │ ←────────────── │              │ ←────────────── │          │
└───────────┘   JSON 响应     └──────────────┘   JSON 响应     └──────────┘
```

**只有一个端口 `8765`**，只有一个方向（后端主动请求 → 桥接 → OpenClaw → 同步返回）。

没有会话管理，没有异步回调，没有轮询，没有 8766 端口。就是一个 HTTP 代理。

### 与旧方案的区别

| 项目 | 旧方案（已废弃） | 当前方案 |
|------|-----------------|---------|
| 通信模式 | 异步回调（POST 8765 + 回调 8766） | 纯同步（POST 8765，同步等待响应） |
| 端口占用 | 8765 + 8766 两个端口 | 仅 8765 一个端口 |
| 会话管理 | bridge 维护 session_id + TTL | 无状态，不维护会话 |
| 上下文历史 | bridge 维护 max_context_history | OpenClaw 侧管理（如有需要） |
| 复杂度 | 高（回调注册、超时轮询、状态清理） | 极低（同步 HTTP 代理） |

---

## 数据流

### 1. C++ 后端发送请求

```
POST http://bridge:8765/process
Content-Type: application/json

{
  "request_id": "req_user123_1234567890",
  "user_id": "user123",
  "text": "你好呀!",
  "context": "",
  "emotion_hints": ["开心"],
  "max_tokens": 1000,
  "temperature": 0.7
}
```

### 2. 桥接服务原样转发到 OpenClaw

桥接服务收到后，直接 `POST` 到 `OPENCLAW_ENDPOINT/process`。

### 3. OpenClaw 返回结果

```json
{
  "text": "你好~欢迎来到我的直播间!",
  "emotions": ["happy"],
  "actions": ["wave"]
}
```

### 4. 桥接服务组装标准响应返回给 C++

```json
{
  "request_id": "req_user123_1234567890",
  "success": true,
  "text": "你好~欢迎来到我的直播间!",
  "emotions": ["happy"],
  "actions": ["wave"],
  "processing_time_ms": 1200
}
```

---

## 部署

桥接服务随 `docker-compose up -d` 自动启动。

独立运行：

```bash
cd bridge
cp .env.example .env
# 编辑 .env，设置 OPENCLAW_ENDPOINT
npm install
node src/index.js
```

环境变量：

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `BRIDGE_PORT` | 监听端口 | `8765` |
| `OPENCLAW_ENDPOINT` | OpenClaw 地址 | `http://localhost:8000` |
| `OPENCLAW_API_KEY` | OpenClaw 认证密钥 | 空 |
| `REQUEST_TIMEOUT_MS` | 请求超时 | `30000` |

健康检查：`GET http://localhost:8765/health`

---

## 如果不用 OpenClaw

如果 OpenClaw 没有启动，桥接服务的 `/process` 端点会返回 `503 OpenClaw 服务不可达`。这不会影响 Yachiyo 其他功能的正常运行——AI 对话会走 DeepSeek API 直连通道。
