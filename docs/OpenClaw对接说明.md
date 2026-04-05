# OpenClaw 对接说明 — 桥接服务架构

> 适用于 Yachiyo 虚拟直播平台  
> 更新时间: 2025-07

---

## 1. 架构总览

Yachiyo C++ 后端 **不直接连接 OpenClaw**，而是通过一个 Node.js 桥接服务中转：

```
┌──────────────┐   HTTP POST    ┌──────────────┐    session     ┌──────────────┐
│              │  ──────────→   │              │  ──────────→   │              │
│  C++ 后端    │    :8765       │  桥接服务     │               │   OpenClaw   │
│  (Crow)      │                │  (Node.js)   │               │  (:8000)     │
│              │   HTTP POST    │              │    result      │              │
│              │  ←──────────   │              │  ←──────────   │              │
└──────────────┘    :8766       └──────────────┘               └──────────────┘
```

### 为什么要加桥接层？

| 原因 | 说明 |
|------|------|
| **会话管理** | Node.js 桥接维护用户对话 session，自动管理上下文历史，C++ 端无需关心 |
| **协议适配** | OpenClaw 可能有 Python SDK / WebSocket / 特殊认证，桥接层统一转为简单 HTTP JSON |
| **解耦** | C++ 后端只关心「发送消息→拿到回复」，OpenClaw 的部署/替换对后端完全透明 |
| **异步支持** | 桥接服务支持同步返回 + 异步回调两种模式 |
| **易于调试** | Node.js 侧可以方便地加日志、mock、限流等中间件 |

---

## 2. 端口分配

| 端口 | 服务 | 方向 | 说明 |
|------|------|------|------|
| **8765** | 桥接服务 (接收端) | C++ → Bridge | 后端发送用户消息到这里 |
| **8766** | 桥接服务 (回调端) | Bridge → C++ | 异步结果推回 / C++ 轮询 |
| **8000** | OpenClaw | Bridge → OpenClaw | 桥接服务内部连接 |

---

## 3. 数据流详解

### 3.1 同步模式（默认）

```
1. C++ 后端 POST http://bridge:8765/process
   {
     "request_id": "req_user123_1234567890",
     "user_id": "user123",
     "text": "你好呀!",
     "context": "",
     "emotion_hints": ["开心"],
     "max_tokens": 1000,
     "temperature": 0.7
   }

2. 桥接服务:
   - 查找/创建 user123 的 session
   - 追加用户消息到 session 历史
   - 转发完整上下文到 OpenClaw POST :8000/process
   - 等待 OpenClaw 返回

3. OpenClaw 返回:
   {
     "text": "你好~欢迎来到我的直播间!",
     "emotions": ["happy"],
     "actions": ["wave"]
   }

4. 桥接服务:
   - 追加助手回复到 session
   - 组装标准化响应返回给 C++

5. C++ 后端收到:
   {
     "request_id": "req_user123_1234567890",
     "success": true,
     "text": "你好~欢迎来到我的直播间!",
     "emotions": ["happy"],
     "actions": ["wave"],
     "processing_time_ms": 1200,
     "session_id": "session_user123_...",
     "metadata": {
       "openclaw_processing_ms": 1100,
       "bridge_overhead_ms": 100,
       "message_count": 2
     }
   }
```

### 3.2 异步模式（可选）

```
1. C++ 后端 POST http://bridge:8765/process → 桥接立即返回 202
2. 桥接处理完成后 POST http://backend:8766/callback → 推送结果
3. 或者 C++ 后端 GET http://bridge:8766/result/{requestId} → 轮询结果
```

---

## 4. 代码文件清单

### 4.1 桥接服务 (Node.js)

```
bridge/
├── package.json           # 依赖: express, axios, uuid, winston, dotenv
├── Dockerfile             # Node.js 18-alpine
├── .env.example           # 环境变量模板
└── src/
    ├── index.js           # 主入口 (两个 Express 实例: 8765 + 8766)
    ├── session.js         # 会话管理器 (内存存储, TTL 过期)
    └── logger.js          # winston 日志
```

### 4.2 C++ 后端

| 文件 | 变更 |
|------|------|
| `backend/include/services/OpenClawGateway.hpp` | 改为桥接模式：`initialize(bridgeEndpoint, callbackPort)` |
| `backend/src/services/OpenClawGateway.cpp` | `sendToBridge()` 替代 `sendRequest()`，新增 `processMessageAsync()` |
| `backend/src/services/AvatarResponseService.cpp` | 初始化改为 `initialize("http://localhost:8765", 8766)` |
| `backend/include/dto/OpenClawDTO.hpp` | 无变更 (DTO 格式兼容) |

### 4.3 配置文件

| 文件 | 变更 |
|------|------|
| `config/config.yaml` | `openclaw.bridge_endpoint` / `openclaw.callback_port` |
| `backend/.env.example` | `OPENCLAW_BRIDGE_ENDPOINT` / `OPENCLAW_CALLBACK_PORT` |
| `docker-compose.yml` | 新增 `bridge` 服务 |

---

## 5. 部署方式

### 5.1 Docker Compose（推荐）

已集成到 `docker-compose.yml`，直接启动即可：

```bash
docker-compose up -d bridge
```

需要设置环境变量：

```bash
# .env
OPENCLAW_ENDPOINT=http://host.docker.internal:8000
OPENCLAW_API_KEY=your-key  # 如果 OpenClaw 需要
```

### 5.2 独立运行

```bash
cd bridge
npm install
cp .env.example .env
# 编辑 .env 配置 OpenClaw 地址
npm start
```

### 5.3 开发模式

```bash
cd bridge
npm run dev   # 自动热重载 (node --watch)
```

---

## 6. 桥接服务 API

### 6.1 接收端 (:8765)

| 方法 | 路径 | 说明 |
|------|------|------|
| `GET` | `/health` | 健康检查 (含活跃会话数) |
| `POST` | `/process` | 处理用户消息 → 转发到 OpenClaw |
| `GET` | `/session/:userId` | 获取用户会话信息 |
| `DELETE` | `/session/:userId` | 清除用户会话 |

### 6.2 回调端 (:8766)

| 方法 | 路径 | 说明 |
|------|------|------|
| `GET` | `/health` | 健康检查 |
| `POST` | `/callback` | 接收 OpenClaw 异步回调 |
| `GET` | `/result/:requestId` | 轮询异步结果 |

---

## 7. OpenClaw 替代方案

如果暂时不部署真正的 OpenClaw，可以用以下 Python 服务替代（桥接服务连接到它）：

```python
# openclaw_simple/main.py
from fastapi import FastAPI
from pydantic import BaseModel
import httpx, uuid, json

app = FastAPI()

DEEPSEEK_API_KEY = "your-key-here"
SYSTEM_PROMPT = """你是八千代辉夜姬（やちよかぐやひめ），一个可爱的虚拟主播。
性格：温柔、偶尔傲娇、喜欢和观众互动。
回复时请以 JSON 格式返回:
{"text": "回复内容", "emotions": ["emotion1"], "actions": ["action1"]}"""

class ProcessRequest(BaseModel):
    session_id: str = ""
    messages: list = []
    user_id: str = ""
    text: str
    context: str = ""
    emotion_hints: list[str] = []
    max_tokens: int = 1000
    temperature: float = 0.7
    metadata: dict = {}

@app.post("/process")
async def process(req: ProcessRequest):
    # 构建消息历史
    messages = [{"role": "system", "content": SYSTEM_PROMPT}]
    
    # 使用桥接服务传来的会话历史
    for msg in req.messages[-10:]:  # 最近 10 条
        messages.append({"role": msg["role"], "content": msg["content"]})
    
    # 如果 messages 里没有当前消息, 补上
    if not req.messages or req.messages[-1].get("content") != req.text:
        messages.append({"role": "user", "content": req.text})
    
    async with httpx.AsyncClient() as client:
        resp = await client.post(
            "https://api.deepseek.com/v1/chat/completions",
            headers={"Authorization": f"Bearer {DEEPSEEK_API_KEY}"},
            json={
                "model": "deepseek-chat",
                "messages": messages,
                "temperature": req.temperature,
                "max_tokens": req.max_tokens,
            },
            timeout=30.0,
        )
    
    content = resp.json()["choices"][0]["message"]["content"]
    
    try:
        parsed = json.loads(content)
        return {
            "text": parsed.get("text", content),
            "emotions": parsed.get("emotions", ["neutral"]),
            "actions": parsed.get("actions", ["idle"]),
        }
    except json.JSONDecodeError:
        return {"text": content, "emotions": ["neutral"], "actions": ["idle"]}

@app.get("/health")
async def health():
    return {"status": "ok"}
```

部署：

```bash
pip install fastapi uvicorn httpx
uvicorn main:app --host 0.0.0.0 --port 8000
```

---

## 8. 端到端消息流程

完整的一条用户消息处理链路：

```
用户发送弹幕 "你好呀!"
  │
  ▼
┌─ 前端 WebSocket ────────────────────────────────────┐
│  useWebSocket.ts → {"type":"chat","text":"你好呀!"} │
└──────────────────────────┬──────────────────────────┘
                           │ ws://backend:9001
                           ▼
┌─ C++ 后端 (AvatarResponseService) ──────────────────┐
│  ① 内容审核 → DeepSeekModerationService             │
│  ② POST http://bridge:8765/process                   │
│     └─→ 桥接服务 → OpenClaw → 回复文本 + 情感 + 动作│
│  ③ 翻译 (如需) → TranslationService                 │
│  ④ TTS 语音 → GPTSoVITSService                      │
│  ⑤ 动画命令 → Live2DAnimationService                │
│  ⑥ 组装完整响应                                      │
└──────────────────────────┬──────────────────────────┘
                           │ WebSocket push
                           ▼
┌─ 前端 ──────────────────────────────────────────────┐
│  显示文字 → 播放语音 → Live2D 表情/动作 → 口型同步  │
└─────────────────────────────────────────────────────┘
```

---

## 9. 注意事项

1. **会话持久化**: 当前桥接服务的 session 存储在内存中，重启会丢失。如需持久化，可接入 Redis。

2. **延迟**: AI 生成（OpenClaw/DeepSeek）是延迟最高的环节（1~5秒）。桥接层本身开销极低（<10ms）。

3. **OpenClaw 不可用时**: 桥接服务会返回 503，C++ 端 `OpenClawGateway` 会将错误透传给 `AvatarResponseService`。

4. **多用户并发**: 桥接服务使用 Express 异步处理，每个用户独立 session，天然支持并发。

5. **日志**: 桥接服务日志在 `bridge/logs/` 目录，包含每个请求的完整链路追踪。
