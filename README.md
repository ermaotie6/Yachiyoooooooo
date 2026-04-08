# Yachiyo — AI 虚拟形象直播平台

<p align="center">
  <img src="docs/assets/logo.png" alt="Yachiyo Logo" width="200" />
</p>

<p align="center">
  <b>AI驱动的虚拟形象直播互动平台</b><br/>
  C++20 后端 · Vue 3 前端 · Node.js 桥接 · Live2D · Docker 一键部署
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C++-20-blue?logo=cplusplus" alt="C++20" />
  <img src="https://img.shields.io/badge/Vue-3-green?logo=vue.js" alt="Vue 3" />
  <img src="https://img.shields.io/badge/Node.js-18-green?logo=node.js" alt="Node.js 18" />
  <img src="https://img.shields.io/badge/PostgreSQL-15-blue?logo=postgresql" alt="PostgreSQL 15" />
  <img src="https://img.shields.io/badge/Redis-7-red?logo=redis" alt="Redis 7" />
  <img src="https://img.shields.io/badge/Docker-Compose-blue?logo=docker" alt="Docker" />
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="License" />
</p>

---

## 目录

- [项目简介](#项目简介)
- [系统架构](#系统架构)
- [核心业务流程](#核心业务流程)
- [后端启动流程](#后端启动流程)
- [6 层内容审核系统](#6-层内容审核系统)
- [WebSocket 通信协议](#websocket-通信协议)
- [技术栈](#技术栈)
- [项目结构](#项目结构)
- [快速开始](#快速开始)
- [配置系统](#配置系统)
- [API 参考](#api-参考)
- [数据库设计](#数据库设计)
- [Docker 服务编排](#docker-服务编排)
- [前端页面说明](#前端页面说明)
- [Live2D 集成](#live2d-集成)
- [外部服务对接](#外部服务对接)
- [本地开发指南](#本地开发指南)
- [故障排查](#故障排查)
- [项目文档索引](#项目文档索引)
- [许可证](#许可证)

---

## 项目简介

Yachiyo（八千代辉夜姬）是一个 AI 虚拟形象直播互动平台。用户通过弹幕与 AI 驱动的 Live2D 虚拟形象实时交互——用户发送弹幕消息后，后端会进行内容审核、AI 对话生成、文本翻译（日语）、语音合成（GPT-SoVITS）、Live2D 表情/动作控制，最终通过 WebSocket 将文本、音频和动画指令实时推送到前端，驱动虚拟形象"说话"并做出表情与动作。

### 核心定位

对话/AI 推理由外部服务 **OpenClaw** 完成（通过 Node.js 桥接服务中转）。本项目（Yachiyo）的职责是：

1. **接收用户弹幕** → 6 层内容安全审核
2. **转发到 OpenClaw** → 将审核通过的弹幕打包为 JSON，通过桥接服务发给 OpenClaw
3. **接收 AI 回复** → 回答文本 + 情感标签 + 动作指令
4. **后处理管线** → 翻译为日语 → GPT-SoVITS 语音合成 → Live2D 表情/动作/口型同步
5. **实时推送** → 通过 WebSocket 将文本、音频、动画指令推送到前端

### 功能模块一览

| 功能模块 | 说明 | 实现状态 |
|---------|------|---------|
| 用户认证 | JWT 注册/登录/Token 刷新，密码 SHA-256 + Salt 加密 | ✅ 已完成 |
| 弹幕审核 | 6 层内容安全审核（速率限制 + IP检查 + 关键词 + AI语义 + 行为分析 + 综合评分） | ✅ 已完成 |
| OpenClaw 对接 | 将审核后的弹幕 JSON 发给 OpenClaw，接收回答文本 + 情感 + 动作 | ✅ 框架就绪 |
| 翻译服务 | 百度翻译 API（主引擎） + DeepSeek LLM 翻译（备选），自动降级 | ✅ 已完成 |
| TTS 语音合成 | GPT-SoVITS 将日语文本转为八千代声线语音，支持情感参考音频 | ✅ 框架就绪 |
| Live2D 驱动 | 情感→表情映射、动作→Motion 映射、Web Audio 口型同步 | ✅ 已完成 |
| WebSocket 推送 | 文本/音频/Live2D 指令全链路实时推送，用户消息+AI回复广播到所有观众 | ✅ 已完成 |
| 消息持久化 | PostgreSQL 存储用户消息、审核状态、对话上下文 | ✅ 已完成 |
| Redis 缓存 | 速率限制、Token 黑名单、翻译缓存、TTS 缓存 | ✅ 已完成 |
| 监控系统 | Prometheus 指标采集 + Grafana 可视化仪表盘 | ✅ 已完成 |

---

## 系统架构

### 总体架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Nginx (:80/:443)                              │
│                        反向代理 + SSL + gzip + WebSocket                    │
└──────────────┬──────────────────────────┬──────────────────┬────────────────┘
               │ /                        │ /api/*           │ /ws/
               ▼                          ▼                  ▼
┌──────────────────────────┐   ┌──────────────────────────────────────────────┐
│   Vue 3 Frontend (:3000) │   │         C++ Backend (Crow Framework)         │
│                          │   │                                              │
│  ┌─ Pages ─────────────┐ │   │  :8080  HTTP API (REST)                      │
│  │ Home.vue            │ │   │  :9001  WebSocket Server                     │
│  │ LiveStream.vue ⭐   │ │   │                                              │
│  │ Chat.vue            │ │   │  ┌─ Controllers ──────────────────────────┐  │
│  │ Profile.vue         │ │   │  │ HealthCtrl   → /api/v1/health         │  │
│  │ Admin.vue           │ │   │  │ AuthCtrl     → /api/v1/auth           │  │
│  └─────────────────────┘ │   │  │ UserCtrl     → /api/v1/users          │  │
│                          │   │  │ MessageCtrl  → /api/v1/messages       │  │
│  ┌─ Components ────────┐ │   │  │ AICtrl       → /api/v2/ai             │  │
│  │ Live2DComponent.vue │ │   │  │ WebSocketCtrl→ ws://:9001             │  │
│  │ LoginDialog.vue     │ │   │  └──────────────────────────────────────┘  │
│  │ Navbar.vue          │ │   │                                              │
│  └─────────────────────┘ │   │  ┌─ Services ────────────────────────────┐  │
│                          │   │  │ AuthServiceImpl      (认证)            │  │
│  ┌─ Composables ───────┐ │   │  │ MessageServiceImpl   (消息+6层审核)    │  │
│  │ useWebSocket.ts     │ │   │  │ AvatarResponseService(核心编排) ⭐    │  │
│  │ useAudioPlayer.ts   │ │   │  │ OpenClawGateway      (AI网关)         │  │
│  │ useAuth.ts          │ │   │  │ TranslationService   (翻译)           │  │
│  └─────────────────────┘ │   │  │ GPTSoVITSService     (语音合成)       │  │
│                          │   │  │ Live2DAnimationService(动画生成)       │  │
│  ┌─ Stores ────────────┐ │   │  │ DeepSeekModerationSvc(内容审核)       │  │
│  │ auth.ts (Pinia)     │ │   │  │ DatabaseService      (数据访问)       │  │
│  └─────────────────────┘ │   │  │ WebSocketService     (实时推送)       │  │
│                          │   │  └──────────────────────────────────────┘  │
│  Live2D: pixi.js +       │   │                                              │
│  pixi-live2d-display     │   │                                              │
│  Audio: Web Audio API    │   │                                              │
└──────────────────────────┘   └──────────┬───────────┬───────────┬──────────┘
                                          │           │           │
                                          ▼           ▼           ▼
                                    ┌──────────┐ ┌──────────┐ ┌──────────────┐
                                    │PostgreSQL│ │  Redis   │ │Bridge (Node) │
                                    │  (:5432) │ │ (:6379)  │ │  (:8765)     │
                                    │          │ │          │ │  JSON 转发    │
                                    │ users    │ │ 速率限制  │ │      │       │
                                    │ messages │ │ Token黑名 │ │      ▼       │
                                    │ sessions │ │ 翻译缓存  │ │  OpenClaw    │
                                    │ contexts │ │ TTS缓存  │ │  (:8000)     │
                                    └──────────┘ └──────────┘ └──────────────┘
```

### 桥接通信流程

```
用户弹幕 → WebSocket → C++ Backend → POST http://bridge:8765/process
                                              │
                                              ▼
                                     Node.js Bridge
                                     (纯 JSON 转发, 无状态)
                                              │
                                              ▼
                                     OpenClaw AI Agent (:8000)
                                     (对话生成, 情感分析, 动作指令)
                                              │
                                              ▼
                                     Bridge 同步返回 JSON 给 C++ 后端
                                              │
                                              ▼
                              C++ Backend 处理响应 (AvatarResponseService)
                              → 百度翻译 API 翻译为日语
                              → GPT-SoVITS 语音合成
                              → Live2D 表情/动作指令生成
                              → 口型同步参数计算
                                              │
                                              ▼
                              WebSocket 广播到所有在线观众
                              → 前端播放语音 + Live2D 动画 + 字幕显示
```

---

## 核心业务流程

### 完整交互链路（端到端）

以用户在直播间发送 "你好，八千代！" 为例：

```
T+0ms     用户在 LiveStream.vue 输入框按下"发送"（50字限制）
T+1ms     useWebSocket.sendUserMessage("你好，八千代！")
T+2ms     WebSocket 消息发送到 C++ 后端 :9001
T+3ms     WebSocketController.processUserMessage() 开始
T+5ms     ── AvatarResponseService.processUserMessage() ──
T+10ms    [步骤1] DeepSeekModerationService.moderate() → "pass"（6层审核）
T+50ms    [步骤2] OpenClawGateway → Bridge :8765 → OpenClaw :8000
T+800ms   OpenClaw 返回: text="你好呀！见到你很高兴~" emotions=["happy"] actions=["wave"]
T+810ms   [步骤3] TranslationService → 百度翻译 API → 日语翻译
T+820ms   [步骤4] GPTSoVITSService.synthesizeWithEmotion("こんにちは！...", "happy")
T+2500ms  TTS 返回: audioUrl, durationMs=2800
T+2510ms  [步骤5] Live2DAnimationService.generateAnimationSequence(["happy"], ["wave"])
T+2520ms  [步骤6] generateMouthSyncCommands(audioUrl, 2800ms)
T+2530ms  [步骤7] 组装 AvatarResponse JSON
T+2535ms  ── WebSocketService 广播到所有在线观众 ──
T+2540ms  前端收到 AvatarResponse
T+2542ms  ① 显示文本回复 + 情感标签
T+2543ms  ② 字幕覆盖层显示翻译后文本
T+2545ms  ③ Live2DComponent 执行: 表情→smile + 动作→wave
T+2550ms  ④ audioPlayer.play(audioUrl) 开始播放语音
T+2555ms  ⑤ AudioAnalyser 实时计算 mouthOpenY → 驱动嘴部动画
T+5350ms  ⑥ 音频播放结束, 嘴巴闭合, 恢复待机表情, 字幕清除
```

**端到端延迟**: ~2.5s（主要瓶颈: TTS 语音合成 ~1.7s）

### AvatarResponseService 编排管线

`AvatarResponseService` 是系统核心编排服务，将 7 个步骤串联为一条完整管线，各步骤独立容错——任一外部服务不可用时，管线仍能返回部分结果（例如只有文本没有语音）：

```
用户消息 ──→ [1. 内容审核] ──→ 违规 → 拒绝并返回 "内容不当"
                 │ 通过
                 ▼
             [2. OpenClaw 网关] ──→ 获取 AI 回复文本 + 情感标签 + 动作指令
                 │
                 ▼
             [3. 翻译服务] ──→ 百度翻译/DeepSeek LLM 翻译为日语（自动降级）
                 │
                 ▼
             [4. GPT-SoVITS TTS] ──→ 日语文本 + 情感参考音频 → 语音合成
                 │
                 ▼
             [5. 动画生成] ──→ 情感→表情映射 + 动作→Motion映射 → Live2DCommand[]
                 │
                 ▼
             [6. 口型同步] ──→ 根据音频时长生成 ParamMouthOpenY 动画帧
                 │
                 ▼
             [7. 组装响应] ──→ AvatarResponse JSON → WebSocket 广播
```

### AvatarResponse JSON 结构

```json
{
  "type": "avatar_response",
  "text": "こんにちは！お会いできて嬉しいです~",
  "originalText": "你好呀！见到你很高兴~",
  "emotions": ["happy"],
  "actions": ["wave"],
  "audioUrl": "http://localhost:5000/audio/xxx.wav",
  "audioDurationMs": 2800,
  "animationCommands": [
    { "type": "expression", "name": "f_smile", "duration": 3000 },
    { "type": "motion", "group": "m_wave", "priority": 2 }
  ],
  "mouthSync": [
    { "time": 0, "value": 0.0 },
    { "time": 100, "value": 0.6 },
    { "time": 200, "value": 0.8 }
  ],
  "processingTimeMs": 2525
}
```

---

## 后端启动流程

后端入口为 `backend/src/main.cpp`，通过 **Application 单例模式** 管控全生命周期：

```
main()
  │
  ├── globalInit()                        // __attribute__((constructor))
  │   ├── setupGlobalExceptionHandler()   // std::set_terminate
  │   └── setupSignalHandlers()           // SIGINT, SIGTERM, SIGSEGV, SIGABRT
  │
  ├── Application::getInstance()          // 获取全局单例
  │
  ├── app->initialize(argc, argv)         // 初始化（严格按依赖顺序）
  │   │
  │   ├── 1. parseArguments()             // 解析命令行参数
  │   │       --config-dir   配置文件目录 (默认: config)
  │   │       --env          运行环境 (默认: dev)
  │   │       --port         HTTP 端口 (默认: 8080)
  │   │       --host         监听地址 (默认: 0.0.0.0)
  │   │       --workers      工作线程数 (默认: CPU核心数)
  │   │
  │   ├── 2. ConfigManager::initialize()  // 加载 YAML 配置
  │   │       读取 config/ 目录下的 config.yaml
  │   │       支持 ${ENV_VAR:default} 环境变量语法
  │   │       支持 dot-separated key path 和 YAML 数组索引
  │   │
  │   ├── 3. LogUtils::initialize()       // 初始化日志系统 (spdlog)
  │   │       日志级别、文件路径、控制台输出
  │   │
  │   ├── 4. initializeHttpServer()       // 创建 Crow HTTP 服务器
  │   │       ├── 绑定 host:port
  │   │       ├── 设置工作线程数
  │   │       ├── 配置 CORS 跨域
  │   │       ├── 配置 gzip 压缩
  │   │       ├── 配置请求速率限制
  │   │       └── (可选) 配置 SSL/TLS
  │   │
  │   ├── 5. initializeDatabase()         // 创建 PostgreSQL 连接池
  │   │       通过 pqxx 建立连接池
  │   │       连接字符串: host + port + dbname + user + password
  │   │       默认连接池大小: 10 (可配置 database.poolSize)
  │   │
  │   ├── 6. initializeAIServices()       // 初始化 AI 提供商配置
  │   │       支持 openai / deepseek / qwen 等多种 AI 后端
  │   │       配置 apiKey、baseUrl、model
  │   │
  │   ├── 7. initializeServices()         // 初始化 WebSocket + Redis
  │   │       ├── WebSocket 服务器 (默认 0.0.0.0:9001)
  │   │       │   在独立后台线程中启动 (std::thread + detach)
  │   │       └── Redis 连接池 (如启用)
  │   │           hiredis 连接到 Redis 服务器
  │   │
  │   └── 8. initializeControllers()      // 注册路由控制器
  │           最后执行，确保所有依赖服务就绪
  │           ├── /api/v1/health   ← HealthController
  │           ├── /api/v1/auth     ← AuthController (JWT)
  │           ├── /api/v2/ai       ← AIController
  │           ├── /api/v1/users    ← UserController
  │           └── /api/v1/messages ← MessageController
  │               (仅当 Database + WebSocket 均初始化成功时注册)
  │
  ├── app->start()                        // 启动 HTTP 服务器
  │   ├── httpServer->start()             // 启动 Crow HTTP 监听
  │   ├── registerSignalHandlers()        // 注册 SIGINT/SIGTERM
  │   └── running = true                  // 标记运行状态
  │
  └── app->wait()                         // 阻塞等待
      └── 收到 SIGINT/SIGTERM → stop()
          └── 依次关闭: HTTP服务器 → 数据库连接池 → Redis连接池
```

---

## 6 层内容审核系统

用户在直播间发送的每条消息都经过 6 层安全审查：

```
用户发送消息
     │
     ▼
┌─── 第1层: 速率限制 ──────────────────────────────────┐
│  Redis 计数器: rate_limit:{userId}                    │
│  限制: 10条/分钟, 100条/小时                          │
│  超限 → 返回 "消息过于频繁，请稍后再试"              │
└───────────────────────┬──────────────────────────────┘
                        │ 通过
                        ▼
┌─── 第2层: IP 黑名单检查 ─────────────────────────────┐
│  检查发送者 IP 是否在黑名单中                         │
│  命中 → 返回 "您的IP已被限制"                        │
└───────────────────────┬──────────────────────────────┘
                        │ 通过
                        ▼
┌─── 第3层: 关键词过滤 ────────────────────────────────┐
│  多级敏感词库匹配（暴力/色情/仇恨/广告）              │
│  命中高危词 → 标记 isBlockedKeyword=true              │
│  计算 keywordScore (0.0-1.0)                         │
└───────────────────────┬──────────────────────────────┘
                        │ 通过
                        ▼
┌─── 第4层: AI 语义审核 ───────────────────────────────┐
│  调用 DeepSeek Chat API 进行语义分析                  │
│  检测变体词、谐音、隐含含义                           │
│  命中 → 标记 isAbusive=true, 计算 aiScore            │
│  API 不可用时自动回退到规则审核                       │
└───────────────────────┬──────────────────────────────┘
                        │ 通过
                        ▼
┌─── 第5层: 行为分析 ──────────────────────────────────┐
│  ① 5分钟消息频率 (Redis user_activity:{userId})      │
│  ② 24小时内多IP检测 (SQL查询)                        │
│  ③ 同IP被拒消息数检测                                │
│  异常 → 标记 hasAbnormalBehavior=true                │
└───────────────────────┬──────────────────────────────┘
                        │ 通过
                        ▼
┌─── 第6层: 综合评分 → 决定审查级别 ───────────────────┐
│  spamScore > 0.6 → ReviewStatus::REJECTED            │
│  spamScore > 0.3 → ReviewStatus::PENDING_REVIEW      │
│  spamScore ≤ 0.3 → ReviewStatus::APPROVED            │
│                                                       │
│  写入 PostgreSQL messages 表                          │
│  返回消息对象 (含 review_status, spam_score)          │
└──────────────────────────────────────────────────────┘
```

---

## WebSocket 通信协议

### 连接建立

前端通过 `useWebSocket.ts` composable 连接到 `ws://host:9001`，连接后发送 `identify` 消息关联 `userId`。

### 消息类型

| 类型 | 方向 | 说明 |
|------|------|------|
| `identify` | 前端→后端 | 连接后发送，关联 userId |
| `ping` / `pong` | 双向 | 30 秒心跳保活 |
| `user_message` | 前端→后端 | 用户弹幕消息（50字限制） |
| `user_broadcast` | 后端→前端 | 用户消息广播到所有观众 |
| `avatar_response` | 后端→前端 | AI 回复 + 音频 + 动画指令（广播到所有观众） |
| `status` | 后端→前端 | 系统状态通知 |
| `error` | 后端→前端 | 错误信息 |

### 消息格式示例

**用户发送弹幕**:
```json
{
  "type": "user_message",
  "data": {
    "text": "你好，八千代！",
    "userId": "user123"
  }
}
```

**AI 回复广播**:
```json
{
  "type": "avatar_response",
  "data": {
    "text": "こんにちは！お会いできて嬉しいです~",
    "originalText": "你好呀！见到你很高兴~",
    "emotions": ["happy"],
    "actions": ["wave"],
    "audioUrl": "http://host:5000/audio/xxx.wav",
    "audioDurationMs": 2800,
    "animationCommands": [...],
    "processingTimeMs": 2525
  }
}
```

---

## 技术栈

### 后端 (C++20)

| 组件 | 技术 | 版本 | 说明 |
|------|------|------|------|
| HTTP 框架 | [Crow](https://github.com/CrowCpp/Crow) | latest | 轻量级 C++ HTTP/WebSocket 框架 |
| 数据库驱动 | [libpqxx](https://github.com/jtv/libpqxx) | 7.x | PostgreSQL C++ 客户端 |
| Redis 驱动 | [hiredis](https://github.com/redis/hiredis) | 1.x | Redis C 客户端 |
| JSON | [nlohmann/json](https://github.com/nlohmann/json) | 3.x | Header-only JSON 库 |
| 配置解析 | [yaml-cpp](https://github.com/jbeder/yaml-cpp) | 0.7+ | YAML 配置文件解析 |
| 日志 | [spdlog](https://github.com/gabime/spdlog) | 1.x | 高性能异步日志 |
| HTTP 客户端 | libcurl | 7.x | 调用外部 API (翻译/TTS/审核) |
| 加密 | OpenSSL | 3.x | JWT 签名/验证、SHA-256 密码哈希 |
| 构建系统 | CMake | 3.20+ | 跨平台构建 |
| 测试框架 | Google Test | latest | 单元测试 + 集成测试 |

### 前端 (Vue 3 + TypeScript)

| 组件 | 技术 | 版本 | 说明 |
|------|------|------|------|
| 框架 | Vue 3 (Composition API) | ^3.3.0 | 响应式 UI 框架 |
| 构建工具 | Vite | ^4.3.0 | 开发服务器 + 打包 |
| 类型系统 | TypeScript | ^5.0.0 | 静态类型检查 |
| UI 组件库 | Element Plus | ^2.3.0 | 企业级 Vue 3 UI |
| 状态管理 | Pinia | ^2.1.0 | Vue 3 官方状态管理 |
| 路由 | Vue Router | ^4.2.0 | 客户端路由 |
| HTTP 客户端 | Axios | ^1.4.0 | API 调用、Token 刷新拦截 |
| Live2D 渲染 | pixi.js + pixi-live2d-display | ^7.3.0 / ^0.4.0 | 基于 PixiJS 的 Live2D 渲染 |
| 语音播放 | Web Audio API | 原生 | 音频播放 + 频率分析 (口型同步) |

### 桥接服务 (Node.js)

| 组件 | 技术 | 说明 |
|------|------|------|
| 运行时 | Node.js 18 | LTS 版本 |
| HTTP 框架 | Express | 无状态 HTTP 代理 |
| HTTP 客户端 | Axios | 转发请求到 OpenClaw |
| 日志 | Winston | 结构化日志 |
| 配置 | dotenv | 环境变量管理 |

### 基础设施

| 组件 | 技术 | 说明 |
|------|------|------|
| 容器化 | Docker + Docker Compose 3.8 | 全栈容器编排 |
| 反向代理 | Nginx | HTTP/HTTPS/WebSocket 代理 |
| 数据库 | PostgreSQL 15 Alpine | 关系型数据存储 |
| 缓存 | Redis 7 Alpine | 速率限制/Token黑名单/缓存 |
| 监控采集 | Prometheus | 指标采集 |
| 监控可视化 | Grafana | 数据可视化仪表盘 |
| 数据库管理 | pgAdmin 4 | Web PostgreSQL 管理 |
| 缓存管理 | Redis Commander | Web Redis 管理 |

---

## 项目结构

```
Yachiyo/
├── backend/                              # C++20 后端服务
│   ├── CMakeLists.txt                   # 顶层 CMake 构建配置
│   ├── Dockerfile                       # 多阶段 Docker 构建
│   ├── config/                          # Docker 部署用配置文件
│   │   ├── config.yaml                  # 后端配置 (环境变量注入)
│   │   ├── init_database.sql            # 数据库初始化 (容器内)
│   │   └── prometheus.yml               # Prometheus 采集配置
│   ├── include/                         # 头文件 (接口声明)
│   │   ├── Application.hpp              # 应用程序主类 (单例)
│   │   ├── config/
│   │   │   └── ConfigManager.hpp        # YAML 配置管理器 (支持环境变量、数组索引)
│   │   ├── controllers/                 # 路由控制器
│   │   │   ├── AuthController.hpp       # 认证 (/auth/register, /auth/login, /auth/me)
│   │   │   ├── AIController.hpp         # AI 对话 (/ai/chat, /ai/tts)
│   │   │   ├── HealthController.hpp     # 健康检查 (/health)
│   │   │   ├── MessageController.hpp    # 消息管理 (/messages)
│   │   │   ├── UserController.hpp       # 用户管理 (/users)
│   │   │   └── WebSocketController.hpp  # WebSocket 消息处理
│   │   ├── dto/                         # 数据传输对象
│   │   │   ├── AuthDTO.hpp              # 登录/注册请求响应
│   │   │   ├── ChatMessageDTO.hpp       # 聊天消息
│   │   │   ├── ChatRequest.hpp          # AI 对话请求
│   │   │   ├── CommonDTO.hpp            # 通用 API 响应包装
│   │   │   ├── Live2DDTO.hpp            # Live2D 动画指令
│   │   │   ├── ModerationDTO.hpp        # 审核结果
│   │   │   ├── OpenClawDTO.hpp          # OpenClaw 请求/响应
│   │   │   ├── TranslationDTO.hpp       # 翻译请求/响应
│   │   │   ├── TTSServiceDTO.hpp        # TTS 请求/响应
│   │   │   └── UserDTO.hpp              # 用户数据
│   │   ├── models/                      # 数据模型
│   │   │   ├── BaseModel.hpp            # 基础模型 (id, createdAt, updatedAt)
│   │   │   ├── User.hpp                 # 用户模型 (toJson/fromJson)
│   │   │   ├── Message.hpp              # 消息模型
│   │   │   └── DatabaseModels.hpp       # 数据库实体映射
│   │   ├── services/                    # 业务服务接口
│   │   │   ├── AIService.hpp            # AI 对话接口
│   │   │   ├── AuthService.hpp          # 认证接口
│   │   │   ├── AuthServiceImpl.hpp      # 认证实现
│   │   │   ├── AvatarResponseService.hpp# 核心编排服务 ⭐
│   │   │   ├── ChatService.hpp          # 聊天服务
│   │   │   ├── DatabaseService.hpp      # 数据库服务 (DAO)
│   │   │   ├── DeepSeekModerationService.hpp # DeepSeek 内容审核
│   │   │   ├── GPTSoVITSService.hpp     # GPT-SoVITS TTS
│   │   │   ├── Live2DAnimationService.hpp # Live2D 动画生成
│   │   │   ├── MessageService.hpp       # 消息服务接口
│   │   │   ├── MessageServiceImpl.hpp   # 消息服务实现 (6层审核)
│   │   │   ├── OpenClawGateway.hpp      # OpenClaw 网关
│   │   │   ├── TranslationService.hpp   # 翻译服务 (百度+DeepSeek)
│   │   │   └── WebSocketService.hpp     # WebSocket 服务
│   │   └── utils/                       # 工具类
│   │       ├── DatabaseUtil.hpp         # 数据库工具
│   │       ├── HashUtil.hpp             # SHA-256 + Salt 哈希
│   │       ├── JwtUtil.hpp              # JWT 生成/验证
│   │       ├── LogUtils.hpp             # spdlog 日志工具
│   │       └── ...
│   ├── src/                             # 源文件 (实现)
│   │   ├── main.cpp                     # 入口点
│   │   ├── Application.cpp              # 应用程序实现 (741 行)
│   │   ├── CMakeLists.txt               # 源码 CMake 配置
│   │   ├── config/
│   │   │   └── ConfigManager.cpp        # 配置管理器实现
│   │   ├── controllers/                 # 控制器实现
│   │   ├── services/                    # 服务实现
│   │   ├── dto/                         # DTO 实现
│   │   ├── filters/                     # 请求过滤器
│   │   ├── mappers/                     # 数据映射器
│   │   ├── models/                      # 模型实现
│   │   └── utils/                       # 工具实现
│   ├── tests/                           # 测试
│   │   ├── CMakeLists.txt               # 测试 CMake 配置
│   │   ├── main_test.cpp                # 测试入口
│   │   ├── unit/                        # 单元测试 (13 个文件)
│   │   └── integration/                 # 集成测试 (4 个文件)
│   └── scripts/
│       └── build.sh                     # 构建脚本
│
├── frontend/                            # Vue 3 前端
│   ├── Dockerfile                       # 前端 Docker 构建
│   ├── package.json                     # npm 依赖
│   ├── vite.config.ts                   # Vite 配置 (含 Live2D 资源代理插件)
│   ├── tsconfig.json                    # TypeScript 配置
│   ├── index.html                       # 入口 HTML
│   ├── public/
│   │   └── images/                      # 可替换的静态图片 (logo.svg, bg.jpg)
│   └── src/
│       ├── App.vue                      # 根组件 (导航栏 + 路由视图)
│       ├── main.ts                      # 入口 (createApp + 插件注册)
│       ├── api/
│       │   └── client.ts                # Axios 实例 (Token 拦截器 + 自动刷新)
│       ├── components/
│       │   ├── Live2DComponent.vue      # Live2D 渲染组件 (pixi-live2d-display)
│       │   ├── LoginDialog.vue          # 登录/注册对话框
│       │   └── ...
│       ├── composables/
│       │   ├── useWebSocket.ts          # WebSocket 连接管理 + 消息处理
│       │   ├── useAudioPlayer.ts        # Web Audio API 音频播放
│       │   └── ...
│       ├── router/                      # Vue Router 路由配置
│       ├── stores/
│       │   └── auth.ts                  # Pinia 认证状态 (token 管理)
│       ├── styles/                      # 全局样式
│       ├── types/
│       │   └── index.ts                 # TypeScript 类型定义
│       └── views/
│           ├── Home.vue                 # 首页
│           ├── LiveStream.vue           # 直播间 ⭐ (核心页面)
│           ├── Chat.vue                 # AI 文字聊天
│           ├── Profile.vue              # 个人资料
│           └── Admin.vue                # 管理后台
│
├── bridge/                              # Node.js 桥接服务
│   ├── Dockerfile                       # 桥接服务 Docker 构建
│   ├── package.json                     # npm 依赖
│   └── src/
│       ├── index.js                     # Express 入口 (JSON 转发代理)
│       └── logger.js                    # Winston 日志配置
│
├── resources/                           # 静态资源
│   ├── live2d/                          # Live2D 模型文件
│   │   ├── yachiyo.model3.json          # 主模型描述文件
│   │   ├── yachiyo.moc3                 # 模型二进制
│   │   ├── yachiyo.physics3.json        # 物理演算
│   │   ├── yachiyo.cdi3.json            # 显示信息
│   │   ├── f_smile.exp3.json            # 表情: 微笑
│   │   ├── f_sad.exp3.json              # 表情: 悲伤
│   │   ├── f_cry.exp3.json              # 表情: 哭泣
│   │   ├── f_squint.exp3.json           # 表情: 眯眼
│   │   └── yachiyo.8192/                # 纹理文件夹
│   └── config/
│       └── avatar_config.yaml           # 虚拟形象配置
│
├── database/
│   └── init.sql                         # 数据库初始化脚本 (表结构 + 索引 + 触发器)
│
├── config/
│   └── config.yaml                      # 全局配置 (本地开发用)
│
├── docs/                                # 项目文档
│   ├── 要求.md                          # 原始项目需求
│   ├── 部署指南.md                      # 完整部署指南 (Docker + 裸机)
│   ├── 项目工作流程分析.md              # 源码级工作流程分析
│   ├── 外部服务对接指南.md              # OpenClaw/GPT-SoVITS/翻译/审核对接
│   ├── 待实现功能清单.md                # 功能状态跟踪
│   ├── GPT-SoVITS部署指南.md            # TTS 训练+部署完整指南
│   └── OpenClaw对接说明.md              # OpenClaw 桥接架构说明
│
├── scripts/                             # 运维脚本
│   ├── deploy.sh                        # 部署脚本
│   ├── deploy-server.sh                 # 服务器一键部署
│   ├── start.sh                         # 启动脚本
│   └── stop.sh                          # 停止脚本
│
├── nginx.conf                           # Nginx 反向代理配置
└── docker-compose.yml                   # Docker Compose 编排 (10 个服务)
```

---

## 快速开始

### 前置要求

| 要求 | 最低版本 | 说明 |
|------|---------|------|
| Docker | 20.10+ | 容器运行时 |
| Docker Compose | v2 | 服务编排 |
| Git | 2.x | 代码克隆 |
| (可选) OpenClaw | - | AI 对话服务，运行在 `http://localhost:8000` |

### 方式一：Docker Compose 一键启动（推荐）

```bash
# 1. 克隆仓库
git clone https://github.com/ermaotie6/yachiyoooooooo.git
cd yachiyoooooooo

# 2. 创建 .env 文件，填入你的 API Key
cat > .env << 'EOF'
JWT_SECRET_KEY=your-random-secret-at-least-32-chars
DEEPSEEK_API_KEY=sk-your-deepseek-key
BAIDU_TRANSLATE_APPID=20150630000000001
BAIDU_TRANSLATE_API_KEY=your-baidu-translate-key
DB_PASSWORD=postgres
EOF

# 3. 生成随机 JWT 密钥（推荐）
# Linux/Mac:
openssl rand -hex 32
# 将生成的值替换 .env 中的 JWT_SECRET_KEY

# 4. 启动所有服务
docker compose up -d

# 5. 查看构建进度（首次构建 C++ 后端可能需要 5-15 分钟）
docker compose logs -f backend

# 6. 验证
curl http://localhost:8080/api/v1/health
# 期望: {"status":"ok","version":"3.0.0",...}

# 7. 访问
#   前端:     http://localhost       (通过 Nginx)
#   API:      http://localhost:8080/api/v1/health
#   pgAdmin:  http://localhost:5050
#   Grafana:  http://localhost:3001
#   Redis UI: http://localhost:8082
```

### 方式二：分步启动（便于排查问题）

```bash
# 1. 先启动基础设施
docker compose up -d postgres redis

# 2. 等待数据库就绪后启动后端
docker compose up -d backend

# 3. 启动前端
docker compose up -d frontend

# 4. 启动反向代理
docker compose up -d nginx

# 5. (可选) 启动桥接服务（需要 OpenClaw 时）
docker compose up -d bridge

# 6. (可选) 启动监控和管理工具
docker compose up -d prometheus grafana pgadmin redis-commander
```

### 方式三：本地开发（不使用 Docker）

```bash
# ===== 后端 =====
cd backend
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20
make -j$(nproc)
./yachiyo_cpp --config-dir ../config --env dev --port 8080

# ===== 前端 =====
cd frontend
npm install
npm run dev
# 开发服务器启动在 http://localhost:5173
# Vite 插件自动代理 /resources/live2d/* 到项目根目录

# ===== 桥接服务 =====
cd bridge
npm install
node src/index.js
# 监听在 http://localhost:8765
```

> **后端本地编译依赖**: CMake 3.20+, GCC/Clang (C++20), libpqxx, hiredis, libcurl, OpenSSL, yaml-cpp, spdlog, nlohmann_json, Crow。详细安装命令见 [docs/部署指南.md](docs/部署指南.md#11-手动裸机部署备选)。

---

## 配置系统

项目有两个配置文件，分别用于本地开发和 Docker 部署：

| 文件 | 用途 | 加载方式 |
|------|------|---------|
| `config/config.yaml` | 本地开发 | 后端默认读取 `config/` 目录 |
| `backend/config/config.yaml` | Docker 部署 | 通过 docker-compose volume 挂载到 `/app/config/` |

### 配置文件结构

```yaml
# ============ 服务器配置 ============
server:
  host: "0.0.0.0"           # 监听地址
  port: 8080                 # HTTP 端口
  debug: false               # 调试模式
  log_level: "info"          # 日志级别 (debug/info/warn/error)
  threads: 4                 # 工作线程数

# ============ WebSocket 配置 ============
websocket:
  enabled: true
  host: "0.0.0.0"
  port: 9001                 # WebSocket 端口
  heartbeat_interval_ms: 30000  # 心跳间隔 30s
  idle_timeout_ms: 300000       # 空闲超时 5min

# ============ 数据库配置 ============
database:
  host: "${DB_HOST:localhost}"      # 支持环境变量，默认 localhost
  port: 5432
  name: "${DB_NAME:yachiyo}"
  username: "${DB_USER:postgres}"
  password: "${DB_PASSWORD}"        # 必须从环境变量注入
  poolSize: 10

# ============ Redis 缓存配置 ============
redis:
  enabled: true
  host: "${REDIS_HOST:localhost}"
  port: 6379

# ============ GPT-SoVITS 语音合成 ============
gpt_sovits:
  enabled: true
  api_endpoint: "http://localhost:5000"
  mode: "cpu"                 # "gpu" (timeout=15s) 或 "cpu" (timeout=60s)

# ============ 内容审核 ============
moderation:
  enabled: true
  api_key: "${DEEPSEEK_API_KEY}"
  model: "deepseek-chat"

# ============ OpenClaw 对话服务 ============
openclaw:
  bridge_endpoint: "${OPENCLAW_BRIDGE_ENDPOINT:http://localhost:8765}"
  timeout_seconds: 30

# ============ 翻译服务 ============
translation:
  target_language: "ja"       # 默认翻译目标语言（日语）
  services:
    - name: "baidu"           # 百度翻译（主引擎）
      appid: "${BAIDU_TRANSLATE_APPID}"
      api_key: "${BAIDU_TRANSLATE_API_KEY}"
    - name: "deepseek"        # DeepSeek LLM 翻译（备选）
      api_key: "${DEEPSEEK_API_KEY}"

# ============ JWT 认证 ============
jwt:
  secret_key: "${JWT_SECRET_KEY}"
  expiration_hours: 24
  refresh_expiration_days: 7
```

### 环境变量语法

配置文件支持 `${ENV_VAR:default}` 语法从环境变量读取值：

```yaml
database:
  host: "${DB_HOST:localhost}"   # 优先读取 DB_HOST 环境变量，不存在则用 localhost
  password: "${DB_PASSWORD}"     # 必须设置环境变量，无默认值
```

### 关键环境变量一览

| 变量 | 说明 | 必填 | 默认值 |
|------|------|------|--------|
| `JWT_SECRET_KEY` | JWT 签名密钥 (≥32字符随机字符串) | ✅ | - |
| `DEEPSEEK_API_KEY` | DeepSeek API Key (对话+审核) | 选填 | - |
| `BAIDU_TRANSLATE_APPID` | 百度翻译 APPID（纯数字） | 选填 | - |
| `BAIDU_TRANSLATE_API_KEY` | 百度翻译密钥 | 选填 | - |
| `DB_HOST` | PostgreSQL 地址 | 选填 | `localhost` |
| `DB_PASSWORD` | PostgreSQL 密码 | ✅ | - |
| `DB_NAME` | 数据库名 | 选填 | `yachiyo` |
| `DB_USER` | 数据库用户 | 选填 | `postgres` |
| `REDIS_HOST` | Redis 地址 | 选填 | `localhost` |
| `OPENCLAW_BRIDGE_ENDPOINT` | 桥接服务地址 | 选填 | `http://localhost:8765` |
| `OPENCLAW_ENDPOINT` | OpenClaw 服务地址 | 选填 | `http://localhost:8000` |

---

## API 参考

### 认证 API (`/api/v1/auth`)

| 方法 | 路径 | 说明 | 认证 | 请求体 |
|------|------|------|------|--------|
| POST | `/api/v1/auth/register` | 用户注册 | ❌ | `{ username, email, password }` |
| POST | `/api/v1/auth/login` | 用户登录 | ❌ | `{ username, password }` |
| POST | `/api/v1/auth/refresh` | 刷新 Token | ❌ | `{ refresh_token }` |
| POST | `/api/v1/auth/logout` | 注销登录 | ✅ | - |
| GET | `/api/v1/auth/me` | 获取当前用户信息 | ✅ | - |
| GET | `/api/v1/auth/profile` | 获取当前用户信息 (别名) | ✅ | - |

**注册请求示例**:
```json
POST /api/v1/auth/register
{
  "username": "testuser",
  "email": "test@example.com",
  "password": "SecureP@ss123"
}
```

**登录响应示例**:
```json
{
  "code": 200,
  "data": {
    "token": "eyJhbGciOi...",
    "refresh_token": "rt_abc123...",
    "user": {
      "id": 1,
      "username": "testuser",
      "email": "test@example.com",
      "role": 1
    }
  }
}
```

### 用户 API (`/api/v1/users`)

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/v1/users` | 用户列表 | ✅ |
| GET | `/api/v1/users/:id` | 用户详情 | ✅ |
| PUT | `/api/v1/users/:id` | 更新用户信息 | ✅ |

### AI 对话 API (`/api/v2/ai`)

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/v2/ai/chat` | AI 文本对话 | ✅ |
| POST | `/api/v2/ai/tts` | TTS 语音合成 | ✅ |
| POST | `/api/v2/ai/stt` | 语音识别 | ✅ |
| GET | `/api/v2/ai/models` | 可用模型列表 | ✅ |
| GET | `/api/v2/ai/history` | 聊天历史 | ✅ |
| DELETE | `/api/v2/ai/history/:id` | 删除聊天记录 | ✅ |

### 消息 API (`/api/v1/messages`)

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/v1/messages` | 消息列表 | ✅ |

### 其他 API

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/v1/health` | 健康检查 | ❌ |
| WS | `ws://host:9001` | WebSocket 实时通信 | ✅ (identify) |

---

## 数据库设计

使用 PostgreSQL 15，数据库名为 `yachiyo`，初始化脚本在 `database/init.sql`。

### 核心数据表

#### users 表

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | BIGSERIAL PK | 自增主键 |
| `username` | VARCHAR(50) UNIQUE | 用户名 |
| `email` | VARCHAR(100) UNIQUE | 邮箱 |
| `password_hash` | VARCHAR(255) | SHA-256 密码哈希 |
| `salt` | VARCHAR(32) | 密码盐值 |
| `nickname` | VARCHAR(50) | 昵称 |
| `role` | SMALLINT | 角色 (1=普通用户, 99=管理员) |
| `status` | SMALLINT | 状态 (1=活跃, 2=禁用, 3=封禁) |
| `messages_sent` | BIGINT | 发送消息数 |
| `messages_rejected` | BIGINT | 被拒消息数 |
| `warnings_count` | SMALLINT | 警告次数 |
| `is_banned` | BOOLEAN | 是否封禁 |
| `created_at` | TIMESTAMP | 创建时间 |
| `last_login_at` | TIMESTAMP | 最后登录时间 |
| `last_login_ip` | VARCHAR(45) | 最后登录 IP |

#### messages 表

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | BIGSERIAL PK | 自增主键 |
| `user_id` | BIGINT FK | 关联用户 |
| `original_message` | TEXT | 原始消息内容 |
| `review_status` | VARCHAR(20) | 审核状态 (approved/pending_review/rejected) |
| `spam_score` | DECIMAL(5,4) | 垃圾评分 (0.0-1.0) |
| `created_at` | TIMESTAMP | 创建时间 |

#### sessions 表

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | BIGSERIAL PK | 自增主键 |
| `user_id` | BIGINT FK | 关联用户 |
| `session_id` | VARCHAR(255) UNIQUE | 会话标识 |
| `access_token` | VARCHAR(1024) | JWT Token |
| `refresh_token` | VARCHAR(1024) | 刷新 Token |

#### conversation_contexts 表

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | BIGSERIAL PK | 自增主键 |
| `user_id` | BIGINT FK | 关联用户 |
| `context_data` | JSONB | 对话上下文数据 |
| `message_history` | JSONB | 消息历史 |

### Redis 数据结构

| Key Pattern | 类型 | TTL | 说明 |
|-------------|------|-----|------|
| `rate_limit:{userId}` | Counter | 60s | 速率限制计数器 |
| `user_activity:{userId}` | Counter | 300s | 用户活跃度计数 |
| `token_blacklist:{token}` | String | Token有效期 | JWT 黑名单 |
| `cache:translation:*` | String | 3600s | 翻译结果缓存 |
| `cache:tts:*` | String | 3600s | TTS 音频缓存 |

---

## Docker 服务编排

### 服务一览

| 服务 | 容器名 | 端口 | 镜像 | 说明 |
|------|--------|------|------|------|
| postgres | yachiyo-postgres | 5432 | postgres:15-alpine | 数据库，带健康检查 |
| redis | yachiyo-redis | 6379 | redis:7-alpine | 缓存，带健康检查 |
| backend | yachiyo-backend | 8080, 9001 | 自建 (Dockerfile) | C++ 后端 (HTTP + WebSocket) |
| frontend | yachiyo-frontend | 3000 | 自建 (Dockerfile) | Vue 3 前端 |
| nginx | yachiyo-nginx | 80, 443 | nginx:alpine | 反向代理 |
| bridge | yachiyo-bridge | 8765 | 自建 (Dockerfile) | Node.js 桥接服务 |
| pgadmin | yachiyo-pgadmin | 5050 | dpage/pgadmin4 | PostgreSQL Web 管理 |
| redis-commander | yachiyo-redis-commander | 8082 | rediscommander/redis-commander | Redis Web 管理 |
| prometheus | yachiyo-prometheus | 9091 | prom/prometheus | 监控指标采集 |
| grafana | yachiyo-grafana | 3001 | grafana/grafana | 监控可视化仪表盘 |

### 启动依赖关系

```
postgres (健康检查) ─┐
                     ├── backend (:8080/9001) ── frontend (:3000) ── nginx (:80/443)
redis    (健康检查) ─┘

bridge (:8765) ── 独立启动，无依赖
pgadmin / redis-commander / prometheus / grafana ── 独立启动
```

### 网络

所有服务在同一个 Docker 网络 `yachiyo-network` 中通信，使用容器名作为主机名（如 `postgres`, `redis`, `backend`, `bridge`）。

### 常用运维命令

```bash
# 查看所有容器状态
docker compose ps

# 查看指定服务日志
docker compose logs -f backend
docker compose logs -f bridge

# 停止所有服务
docker compose down

# 重建并启动后端（代码更新后）
docker compose up -d --build backend

# 备份数据库
docker compose exec postgres pg_dump -U postgres yachiyo > backup_$(date +%Y%m%d).sql

# 恢复数据库
docker compose exec -T postgres psql -U postgres yachiyo < backup.sql
```

---

## 前端页面说明

### 路由表

| 路由 | 组件 | 认证 | 说明 |
|------|------|------|------|
| `/` | Home.vue | ❌ | 首页，项目介绍 |
| `/posts` | Posts.vue | ❌ | 内容列表浏览 |
| `/posts/:id` | PostDetail.vue | ❌ | 内容详情 |
| `/chat` | Chat.vue | ✅ | AI 文字聊天 (HTTP API) |
| `/livestream` | LiveStream.vue | ✅ | **核心页面** — Live2D 虚拟直播间 (WebSocket) |
| `/profile` | Profile.vue | ✅ | 个人资料管理 |
| `/admin` | Admin.vue | ✅ Admin | 管理后台仪表盘 |
| `/admin/moderation` | Moderation.vue | ✅ Admin | 内容审核管理 |
| `/admin/messages` | MessageManagement.vue | ✅ Admin | 消息管理 |
| `/admin/users` | UserManagement.vue | ✅ Admin | 用户管理 |

### 认证流程

- `accessToken` + `refreshToken` 存储在 `localStorage` + Pinia store
- Axios 请求拦截器自动附加 `Authorization: Bearer <token>`
- 响应拦截器检测 401 → 自动调用 `/auth/refresh` 获取新 Token → 重试原始请求
- 访问需认证页面时未登录 → 路由守卫重定向到首页并自动弹出登录对话框 (`showLogin=true`)

### LiveStream.vue（核心直播页面）

直播间页面包含以下组件：

- **Live2DComponent** — 渲染 Live2D 虚拟形象 (pixi-live2d-display)
- **字幕覆盖层** — 与音频同步显示翻译后文本
- **消息输入框** — 50 字限制，发送弹幕消息
- **实时消息框** — 显示所有用户的弹幕消息（包含昵称+文本）
- **音频播放器** — useAudioPlayer composable，Web Audio API 播放 TTS 音频

### 可自定义资源

| 资源 | 路径 | 说明 |
|------|------|------|
| 网站 Logo | `frontend/public/images/logo.svg` | 替换为自定义 Logo |
| 页面背景 | `frontend/public/images/bg.jpg` | 放置背景图片即可生效 |
| Live2D 模型 | `resources/live2d/` | 替换模型文件 |

---

## Live2D 集成

### 模型文件

Live2D 模型文件位于 `resources/live2d/` 目录，开发时 Vite 自定义插件 (`serve-live2d-resources`) 自动代理 `/resources/live2d/*` 到项目根目录，无需手动复制到 `frontend/public/`。

### 当前可用表情

| 表情文件 | 映射情感 | 说明 |
|---------|---------|------|
| `f_smile.exp3.json` | happy, excited | 微笑 |
| `f_sad.exp3.json` | sad | 悲伤 |
| `f_cry.exp3.json` | cry | 哭泣 |
| `f_squint.exp3.json` | neutral | 眯眼 |

### 情感→表情映射规则

由 `Live2DAnimationService` 实现：

| OpenClaw 返回的情感标签 | Live2D 表情 | 说明 |
|------------------------|-------------|------|
| `happy` / `excited` | `f_smile` | 微笑 |
| `sad` | `f_sad` | 悲伤 |
| `angry` | `f_cry` | 哭泣（复用） |
| `surprised` | `f_squint` | 眯眼 |
| 其他/默认 | 无表情切换 | 保持当前表情 |

### 口型同步

前端通过 Web Audio API 实现实时口型同步：

1. 音频通过 `AudioContext` 播放
2. 创建 `AnalyserNode` 进行频率分析
3. `getByteFrequencyData()` 获取频率数据 → 计算 `mouthOpenY` (0~1)
4. 通过 Live2D SDK 设置 `ParamMouthOpenY` 参数
5. 音频播放结束 → `mouthOpenY` 归零 → 嘴巴闭合

### Live2DComponent 组件 API

| 方法 | 参数 | 说明 |
|------|------|------|
| `setExpression(name)` | `'f_smile'` 等 | 切换表情 |
| `playMotion(group, priority)` | `'m_greet'`, 2 | 播放动作 |
| `setSyncMouthOpenY(value)` | 0~1 | 口型同步 |
| `setEyeTrackingTarget(x, y)` | 坐标 | 眼球追踪 |
| `setEyeOpen(left, right)` | 0~1 | 眼睑控制 |
| `syncMouthFromAudio(analyserNode)` | AnalyserNode | 从 Web Audio 自动同步口型 |

---

## 外部服务对接

### 对接概览

| 外部服务 | 通信方式 | 端口 | 是否必须 | 文档 |
|---------|---------|------|---------|------|
| OpenClaw | HTTP POST (via Bridge) | 8765→8000 | 选填 | [OpenClaw对接说明.md](docs/OpenClaw对接说明.md) |
| GPT-SoVITS | HTTP POST | 5000 | 选填 | [GPT-SoVITS部署指南.md](docs/GPT-SoVITS部署指南.md) |
| 百度翻译 API | HTTP GET | - | 选填 | [外部服务对接指南.md](docs/外部服务对接指南.md) |
| DeepSeek API | HTTP POST | - | 选填 | [外部服务对接指南.md](docs/外部服务对接指南.md) |

### 服务降级策略

所有外部服务均非必须，系统支持优雅降级：

| 场景 | 降级行为 |
|------|---------|
| OpenClaw 不可用 | AI 对话走 DeepSeek API 直连通道 |
| GPT-SoVITS 不可用 | 跳过语音合成，只返回文本 |
| 百度翻译不可用 | 自动切换到 DeepSeek LLM 翻译 |
| 所有翻译引擎不可用 | 返回原文（不翻译） |
| DeepSeek 审核不可用 | 回退到本地敏感词规则审核 |
| Redis 不可用 | 跳过缓存和速率限制，直接处理 |

---

## 本地开发指南

### 后端开发

**系统依赖安装** (Arch Linux):
```bash
sudo pacman -S base-devel cmake ninja gcc git \
    openssl yaml-cpp postgresql-libs hiredis curl \
    nlohmann-json postgresql redis
```

**系统依赖安装** (Debian/Ubuntu):
```bash
sudo apt-get install -y build-essential cmake ninja-build gcc g++ git \
    libssl-dev libyaml-cpp-dev libpq-dev libhiredis-dev libcurl4-openssl-dev \
    nlohmann-json3-dev pkg-config
```

**编译和运行**:
```bash
cd backend
mkdir -p build && cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20
ninja
./yachiyo_cpp --config-dir ../config --env dev --port 8080
```

**运行测试**:
```bash
cd backend/build
ctest --output-on-failure
```

### 前端开发

```bash
cd frontend
npm install
npm run dev          # 开发服务器 (http://localhost:5173)
npm run build        # 生产构建
npm run preview      # 预览生产构建
```

### 桥接服务开发

```bash
cd bridge
npm install
# 设置环境变量
export OPENCLAW_ENDPOINT=http://localhost:8000
export BRIDGE_PORT=8765
node src/index.js
```

---

## 故障排查

### 后端连不上数据库

1. 确认 PostgreSQL 正在运行: `docker compose ps postgres`
2. 检查数据库密码一致性: `.env` 中的 `DB_PASSWORD` 与 `config.yaml` 中 `database.password` 一致
3. Docker 模式下 host 为 `postgres`（容器名），本地开发为 `localhost`

### 前端无法调用 API

1. 检查 Nginx 是否正常运行: `docker compose ps nginx`
2. 检查 CORS 配置: 后端 `config.yaml` 中 `server.cors` 是否允许前端域名

### WebSocket 连接失败

1. 检查 Nginx `/ws` 路径是否配置了 WebSocket 升级头
2. 如使用 Cloudflare，确认 Network → WebSockets 已开启
3. 前端使用 `wss://` 协议时需要 HTTPS

### Docker 构建后端 OOM

C++ 编译消耗大量内存，如果 OOM 可限制并行编译数:
```bash
docker compose build --build-arg CMAKE_BUILD_PARALLEL_LEVEL=2 backend
```

### 百度翻译报 54001 签名错误

检查 APPID（纯数字）和密钥是否正确。两个值在 [百度翻译开放平台](https://fanyi-api.baidu.com/manage/developer) → 开发者信息页面获取。

### Cloudflare 522/524 错误

- 522 (服务器不可达): 检查防火墙是否开放 80/443 端口
- 524 (请求超时): 检查 Nginx 和后端是否在运行

---

## 项目文档索引

| 文档 | 说明 |
|------|------|
| [README.md](README.md) | 项目总览（本文件） |
| [docs/要求.md](docs/要求.md) | 原始项目需求文档 |
| [docs/部署指南.md](docs/部署指南.md) | 完整部署指南 (Docker Compose + 裸机 + HTTPS + 安全加固) |
| [docs/项目工作流程分析.md](docs/项目工作流程分析.md) | 源码级工作流程分析 (启动流程/认证/审核/直播/桥接) |
| [docs/外部服务对接指南.md](docs/外部服务对接指南.md) | OpenClaw / GPT-SoVITS / 翻译 / 审核 对接详解 |
| [docs/待实现功能清单.md](docs/待实现功能清单.md) | 功能模块状态跟踪 |
| [docs/GPT-SoVITS部署指南.md](docs/GPT-SoVITS部署指南.md) | TTS 训练 (Windows) + 部署 (Linux) 完整指南 |
| [docs/OpenClaw对接说明.md](docs/OpenClaw对接说明.md) | OpenClaw 桥接架构 + 数据流 + 部署说明 |

---

## 许可证

MIT License

---
