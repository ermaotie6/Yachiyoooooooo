# 🚀 OpenClaw 和 GPT-SoVITS 快速参考

## 核心概念 (30 秒理解)

**OpenClaw** = 统一的 AI 大脑  
**GPT-SoVITS** = OpenClaw 的语音合成工具  

```
用户输入 → OpenClaw (处理) → GPT-SoVITS (合成) → 语音 + 动画
```

---

## 常见问题

### Q1: GPT-SoVITS 是独立的吗？
**否**。GPT-SoVITS 接收 OpenClaw 生成的参数，不能独立使用。

### Q2: OpenClaw 做什么？
- 理解用户消息 (via GPT-3.5)
- 生成回复文本
- 分析情感
- 生成语音参数 (速度/音调/能量)
- 生成动画参数 (表情/姿态)

### Q3: GPT-SoVITS 做什么？
- 接收 OpenClaw 的参数
- 合成日语语音音频
- 返回音频给前端播放

### Q4: 数据流是什么样的？
```
用户: "こんにちは"
  ↓
OpenClaw 处理
  ├─ text = "おはようございます"
  ├─ speed = 1.2
  ├─ pitch = 0.8
  ├─ energy = 0.9
  └─ emotion = "happy"
  ↓
GPT-SoVITS 合成
  ├─ 使用上述参数
  ├─ 生成音频
  └─ 返回 audio.mp3
  ↓
前端播放并触发 Live2D 动画
```

---

## 集成快速清单

### 后端 (C++)

```cpp
// 1. 调用 OpenClaw
auto openclaw_result = openClaw.process(user_message);

// 2. 从结果获取参数
auto voice_params = openclaw_result.voice_params;  // {speed, pitch, energy}
auto text = openclaw_result.text;

// 3. 调用 GPT-SoVITS
auto audio = gptsovits_client.synthesize({
    .text = text,
    .speed = voice_params.speed,
    .pitch = voice_params.pitch,
    .energy = voice_params.energy,
    .emotion = openclaw_result.emotion
});

// 4. 返回给前端
return {.text = text, .audio = audio};
```

### 前端 (TypeScript)

```typescript
// 1. 调用后端 API
const response = await api.post('/api/chat', {message});

// 2. 获取音频
const {text, audio} = response.data;

// 3. 播放音频
const audioContext = new AudioContext();
const buffer = await audioContext.decodeAudioData(audio);
const source = audioContext.createBufferSource();
source.buffer = buffer;
source.connect(audioContext.destination);
source.start(0);

// 4. 触发 Live2D 动画
window.dispatchEvent(new CustomEvent('playAnimation', {
    detail: {emotion: 'happy'}
}));
```

---

## API 端点速查

| 端点 | 方法 | 输入 | 输出 |
|------|------|------|------|
| `/api/chat` | POST | `{message}` | `{text, audio, anim_params, emotion}` |
| `/api/voice/synthesize` | POST | `{text, speed, pitch}` | `{audio, duration_ms}` |
| `/api/voice/clone` | POST | `FormData(audio_file)` | `{speaker_id}` |

---

## 性能参数

| 参数 | 值 | 说明 |
|------|-----|------|
| **缓存大小** | 5000 条 | OpenClaw 响应缓存 |
| **缓存 TTL** | 24 小时 | 缓存过期时间 |
| **并发限制** | 100 | 同时处理的请求数 |
| **GPU 显存** | 4GB+ | GPT-SoVITS 最低要求 |
| **平均延迟** | 500-1000ms | 包括网络+处理时间 |
| **音频质量** | 44.1kHz | 采样率 |

---

## 配置文件位置

- **OpenClaw 配置**: `backend/config/openclaw.yaml`
- **GPT-SoVITS 配置**: `backend/config/gptsovits.yaml`
- **参数模板**: `backend/config/voice_params.json`

---

## 故障排查速查表

| 问题 | 原因 | 解决 |
|------|------|------|
| 没有音频输出 | GPT-SoVITS 未启动 | 检查: `curl localhost:9000/status` |
| 音频低质 | 参数不合适 | 调整 speed/pitch/energy 值 |
| 超时错误 | GPU 过载 | 减少并发，增加 batch 处理时间 |
| 参数不匹配 | OpenClaw 版本差异 | 同步官方最新版本 |

---

## 相关文档链接

- 📖 **完整指南**: `docs/GPT_SOVITS_INTEGRATION_GUIDE.md`
- 📋 **架构详解**: `docs/CODE_LOGIC_REVIEW.md` (第 416 行)
- 🔗 **API 文档**: `docs/API_INTEGRATION_GUIDE.md`
- ⚙️ **配置说明**: `backend/README.md`

---

## 关键代码文件

```
backend/
├── src/
│   ├── services/
│   │   ├── OpenClawIntegration.cpp       ← OpenClaw 集成
│   │   ├── ChatService.cpp               ← 聊天服务
│   │   └── VoiceSynthesisService.cpp    ← 语音合成服务
│   └── utils/
│       ├── GPTSoVITSClient.cpp          ← GPT-SoVITS 客户端
│       └── HttpServer.cpp                ← HTTP 服务器
├── include/
│   ├── services/
│   │   └── ChatService.hpp
│   └── utils/
│       └── GPTSoVITSClient.hpp
└── config/
    ├── gptsovits.yaml
    └── openclaw.yaml
```

---

## 快速启动 (3 步)

```bash
# 1. 启动后端服务
cd backend && ./start.sh

# 2. 启动前端应用
cd frontend && npm run dev

# 3. 打开浏览器
# http://localhost:5173
```

---

## 常用命令

```bash
# 检查 OpenClaw 健康状态
curl http://localhost:8080/health

# 检查 GPT-SoVITS 服务
curl http://localhost:9000/status

# 测试文本合成
curl -X POST http://localhost:9000/tts/synthesize \
  -H "Content-Type: application/json" \
  -d '{"text": "こんにちは", "speed": 1.0}'

# 查看日志
docker logs yachiyo-backend
docker logs yachiyo-gptsovits
```

---

## 思维导图

```
Yachiyo 系统
├─ OpenClaw (统一 AI 管理)
│  ├─ 语言理解 (GPT-3.5)
│  ├─ 参数生成
│  │  ├─ 文本
│  │  ├─ 声音参数 (→ GPT-SoVITS)
│  │  └─ 动画参数 (→ Live2D)
│  └─ 情感管理
│
├─ GPT-SoVITS (语音合成)
│  ├─ 接收参数
│  ├─ GPU 处理
│  └─ 返回音频
│
├─ Live2D (2D 动画)
│  ├─ 接收动画参数
│  └─ 播放表情/姿态
│
└─ 前端 (Vue 3)
   ├─ 显示虚拱形象
   ├─ 播放音频
   └─ 触发动画
```

---

## 下一步

- ✅ 已完成: 理解 OpenClaw 和 GPT-SoVITS 的关系
- 📖 建议: 阅读完整集成指南 (`GPT_SOVITS_INTEGRATION_GUIDE.md`)
- 💻 实践: 在本地部署和测试
- 🐛 遇到问题: 查看故障排查指南

---

*最后更新: 2024年*  
*快速参考版本: 1.0*
