# OpenClaw 对接说明

> 适用于 Yachiyo 虚拟直播平台  
> OpenClaw 是 AI Agent 编排框架，负责协调 AI 对话、内容审核、语音合成、Live2D 动作等模块。

---

## 1. OpenClaw 在 Yachiyo 中的角色

OpenClaw 是整个 AI 直播流水线的 **中枢调度器**，负责将用户的一条消息转换为完整的虚拟主播响应（包括文字、语音、表情、动作）。

### 消息处理流水线

```
用户发送弹幕/消息
  │
  ▼
┌─────────────────┐
│  Yachiyo 后端   │
│  (Crow HTTP)    │
└────────┬────────┘
         │ POST /process
         ▼
┌─────────────────┐
│   OpenClaw      │  ← 独立部署的 AI Agent 服务
│  (localhost:8000)│
│                 │
│  1. 内容审核    │  → DeepSeek Moderation (内置或回调)
│  2. 上下文管理  │  → 维护对话历史 + 角色记忆
│  3. AI 生成回复 │  → DeepSeek Chat / 自定义模型
│  4. 情感分析    │  → 从回复中提取情感标签
│  5. 动作规划    │  → 根据情感决定 Live2D 动作
│                 │
└────────┬────────┘
         │ 返回 OpenClawResponse
         ▼
┌─────────────────┐
│  Yachiyo 后端   │
│  后处理:        │
│  1. 语音合成    │  → GPT-SoVITS
│  2. WebSocket推送│ → 前端
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│    前端         │
│  1. 显示文字    │
│  2. 播放语音    │
│  3. Live2D动画  │
│  4. 口型同步    │
└─────────────────┘
```

---

## 2. 项目中的相关代码

### 2.1 后端 Gateway

**文件**: `backend/src/services/OpenClawGateway.cpp`

```cpp
// 已实现的功能:
// - processMessage(text, context, emotion_hints) → 发送到 OpenClaw
// - 健康检查: GET /health
// - 响应缓存 (TTL 可配置)
// - 自动重试
```

**请求格式** (POST `http://localhost:8000/process`):

```json
{
  "request_id": "uuid-string",
  "text": "用户发送的消息",
  "context": "当前对话上下文",
  "emotion_hints": ["开心", "好奇"],
  "max_tokens": 512,
  "temperature": 0.7
}
```

**响应格式**:

```json
{
  "request_id": "uuid-string",
  "success": true,
  "text": "八千代辉夜姬的回复文本",
  "emotions": ["happy", "curious"],
  "actions": ["nod", "smile"],
  "processing_time_ms": 350
}
```

### 2.2 DTO 定义

**文件**: `backend/include/dto/OpenClawDTO.hpp`

```cpp
struct OpenClawRequest {
    std::string requestId;
    std::string text;
    std::string context;
    std::string userId;
    std::string conversationId;
    int maxTokens = 512;
    float temperature = 0.7f;
    std::vector<std::string> emotionHints;
};

struct OpenClawResponse {
    std::string requestId;
    bool success;
    std::string text;
    std::vector<std::string> emotions;
    std::vector<std::string> actions;
    int processingTimeMs;
    std::string errorMessage;
};
```

### 2.3 配置

**文件**: `config/config.yaml`

```yaml
openclaw:
  enabled: true
  local_deployment: true
  local_endpoint: "http://localhost:8000"
  session_ttl_seconds: 3600
  max_context_history: 20
  memory_update_interval: 300
  enable_task_execution: true
  task_timeout_seconds: 30
  response_cache_enabled: true
  response_cache_ttl: 300
  batch_processing_enabled: true
  batch_size: 10
```

---

## 3. OpenClaw 部署

### 3.1 什么是 OpenClaw

OpenClaw 是一个 AI Agent 框架 / 工作流编排器。如果你不熟悉 OpenClaw，也可以用以下替代方案：

| 方案 | 复杂度 | 说明 |
| ---- | ---- | ---- |
| **OpenClaw** | 高 | 完整 Agent 框架，支持工具调用、记忆管理 |
| **LangChain + FastAPI** | 中 | Python，灵活但需自己搭 API 层 |
| **自定义 FastAPI 服务** | 低 | 最简方案，只需包装 DeepSeek API + 角色 prompt |

### 3.2 最简替代方案（推荐初期使用）

如果暂不需要完整的 Agent 框架，可以用一个简单的 Python FastAPI 服务代替 OpenClaw：

```python
# openclaw_simple/main.py
from fastapi import FastAPI
from pydantic import BaseModel
import httpx
import uuid

app = FastAPI()

DEEPSEEK_API_KEY = "your-key-here"
DEEPSEEK_ENDPOINT = "https://api.deepseek.com/v1/chat/completions"

SYSTEM_PROMPT = """你是八千代辉夜姬（やちよかぐやひめ），一个可爱的虚拟主播。
性格：温柔、偶尔傲娇、喜欢和观众互动。
说话风格：口语化、偶尔用日语词汇、会用颜文字。
回复时请同时输出：
1. 回复文字
2. 情感标签（如 happy, sad, surprised 等）
3. 建议动作（如 nod, wave, think 等）

请严格以 JSON 格式返回:
{"text": "回复内容", "emotions": ["emotion1"], "actions": ["action1"]}
"""

class ProcessRequest(BaseModel):
    request_id: str = ""
    text: str
    context: str = ""
    emotion_hints: list[str] = []
    max_tokens: int = 512
    temperature: float = 0.7

@app.post("/process")
async def process(req: ProcessRequest):
    request_id = req.request_id or str(uuid.uuid4())
    
    messages = [
        {"role": "system", "content": SYSTEM_PROMPT},
    ]
    if req.context:
        messages.append({"role": "assistant", "content": f"[上下文] {req.context}"})
    messages.append({"role": "user", "content": req.text})
    
    async with httpx.AsyncClient() as client:
        resp = await client.post(
            DEEPSEEK_ENDPOINT,
            headers={
                "Authorization": f"Bearer {DEEPSEEK_API_KEY}",
                "Content-Type": "application/json",
            },
            json={
                "model": "deepseek-chat",
                "messages": messages,
                "temperature": req.temperature,
                "max_tokens": req.max_tokens,
            },
            timeout=30.0,
        )
    
    data = resp.json()
    content = data["choices"][0]["message"]["content"]
    
    # 尝试解析 JSON
    import json
    try:
        parsed = json.loads(content)
        return {
            "request_id": request_id,
            "success": True,
            "text": parsed.get("text", content),
            "emotions": parsed.get("emotions", ["neutral"]),
            "actions": parsed.get("actions", ["idle"]),
            "processing_time_ms": 0,
        }
    except json.JSONDecodeError:
        return {
            "request_id": request_id,
            "success": True,
            "text": content,
            "emotions": ["neutral"],
            "actions": ["idle"],
            "processing_time_ms": 0,
        }

@app.get("/health")
async def health():
    return {"status": "ok"}
```

### 3.3 部署替代服务

```bash
# 安装依赖
pip install fastapi uvicorn httpx

# 启动
cd openclaw_simple
uvicorn main:app --host 0.0.0.0 --port 8000

# 测试
curl -X POST http://localhost:8000/process \
  -H "Content-Type: application/json" \
  -d '{"text": "你好呀!", "request_id": "test-001"}'
```

### 3.4 Docker 部署

```dockerfile
# openclaw_simple/Dockerfile
FROM python:3.11-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY . .
EXPOSE 8000
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000"]
```

添加到 `docker-compose.yml`:

```yaml
  openclaw:
    build: ./openclaw_simple
    ports:
      - "8000:8000"
    environment:
      - DEEPSEEK_API_KEY=${DEEPSEEK_API_KEY}
    restart: unless-stopped
```

---

## 4. 完整的前后端通信流程

### 4.1 直播间消息处理（端到端）

```
1. 前端 (LiveStream.vue)
   └── useWebSocket.ts 发送: {"type": "chat", "text": "你好", "user_id": "123"}

2. 后端 WebSocket 收到消息
   └── AIController / WebSocket handler

3. 内容审核
   └── DeepSeekModerationService.moderate(request)
   └── 如果 verdict == "block" → 返回拒绝提示

4. 调用 OpenClaw
   └── OpenClawGateway.processMessage(text, context, emotions)
   └── POST http://localhost:8000/process
   └── 返回: {text, emotions, actions}

5. 语音合成
   └── GPTSoVITSService.synthesize(response.text, emotion)
   └── POST http://localhost:5000/synthesize
   └── 返回: {audio_url, duration_ms}

6. WebSocket 推送
   └── 组装完整响应:
       {
         "request_id": "xxx",
         "text": "八千代的回复",
         "audio_url": "/audio/xxx.wav",
         "audio_duration_ms": 2500,
         "emotions": ["happy"],
         "actions": ["nod", "smile"],
         "animation_commands": [
           {"type": "expression", "value": "happy", "duration": 3000},
           {"type": "motion", "value": "nod", "duration": 1500}
         ]
       }

7. 前端处理
   └── 显示回复文字
   └── 播放音频
   └── Live2D 切换表情 + 播放动作
   └── 音频时长内同步口型动画
```

---

## 5. 注意事项

1. **延迟优化**: OpenClaw (AI 生成) 是延迟最高的环节（1~5秒），可以：
   - 使用流式响应（SSE），先返回文字再异步合成语音
   - 缓存常见问答

2. **对话记忆**: `max_context_history: 20` 表示保留最近 20 条对话，可在 config.yaml 调整。

3. **健康检查**: Yachiyo 后端会定期调用 `GET /health` 检查 OpenClaw 是否在线。

4. **错误处理**: `OpenClawGateway` 已实现自动重试和缓存回退，当 OpenClaw 不可用时会返回错误提示而非崩溃。
