# 🌸 Yachiyo — AI 虚拟形象直播平台

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
  <img src="https://img.shields.io/badge/Docker-Compose-blue?logo=docker" alt="Docker" />
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="License" />
</p>

---

## 📖 目录

- [项目简介](#-项目简介)
- [系统架构](#-系统架构)
- [运行逻辑](#-运行逻辑)
- [技术栈](#-技术栈)
- [项目结构](#-项目结构)
- [快速开始](#-快速开始)
- [API 端点](#-api-端点)
- [Docker 服务](#-docker-服务)
- [配置说明](#-配置说明)
- [许可证](#-许可证)

---

## 🎯 项目简介

Yachiyo 是一个 AI 虚拟形象直播互动平台，用户可以通过弹幕与 AI 驱动的 Live2D 虚拟形象进行实时交互。系统基于 **OpenClaw** AI 编排框架处理对话逻辑，通过 **Node.js 桥接服务** 实现 C++ 后端与 Python AI Agent 的跨语言异步通信。

### 核心功能

| 功能模块 | 说明 |
|---------|------|
| 🤖 AI 对话 | 基于 DeepSeek/OpenAI 的多轮对话，支持情感分析与表情映射 |
| 🎭 Live2D 驱动 | 根据 AI 回复的情感标签实时切换虚拟形象表情与动作 |
| 🔊 TTS 语音合成 | AI 回复自动转语音，实现虚拟形象"说话" |
| 💬 WebSocket 实时推送 | 弹幕消息、AI 回复、Live2D 指令全链路实时传输 |
| 🔐 JWT 认证 | 用户注册/登录，Token 鉴权 |
| 📝 社区功能 | 帖子发布、用户管理 |
| 🔗 OpenClaw 桥接 | Node.js 桥接服务实现 C++ ↔ Python AI Agent 异步通信 |
| 📊 监控系统 | Prometheus + Grafana 指标采集与可视化 |

---

## 🏗 系统架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Nginx (:80/:443)                              │
│                            反向代理 & 负载均衡                               │
└──────────────┬────────────────────────────────────┬─────────────────────────┘
               │                                    │
               ▼                                    ▼
┌──────────────────────────┐          ┌──────────────────────────┐
│   Vue 3 Frontend (:3000) │          │  C++ Backend (:8080)     │
│                          │          │  Crow HTTP Framework     │
│  Element Plus + Pinia    │◄────────►│                          │
│  Live2D Cubism SDK       │  REST    │  WebSocket Server (:9001)│
│  Web Audio API (TTS)     │◄────────►│  实时消息推送              │
└──────────────────────────┘  WS      └──────────┬───────────────┘
                                                  │
                                    ┌─────────────┼─────────────┐
                                    │             │             │
                                    ▼             ▼             ▼
                              ┌──────────┐ ┌──────────┐ ┌──────────────────┐
                              │PostgreSQL│ │  Redis   │ │ Bridge (Node.js) │
                              │  (:5432) │ │ (:6379)  │ │ :8765 ← 接收     │
                              │  用户数据 │ │  会话缓存 │ │ :8766 → 回调     │
                              └──────────┘ └──────────┘ └────────┬─────────┘
                                                                  │
                                                                  ▼
                                                          ┌──────────────┐
                                                          │   OpenClaw   │
                                                          │   (:8000)    │
                                                          │ AI Agent 编排 │
                                                          └──────────────┘
```

### 桥接通信流程

```
用户弹幕 → WebSocket → C++ Backend → POST http://bridge:8765/process
                                              │
                                              ▼
                                     Node.js Bridge
                                     (会话管理, 请求转发)
                                              │
                                              ▼
                                     OpenClaw AI Agent (:8000)
                                     (对话生成, 情感分析, 动作指令)
                                              │
                                              ▼
                                     Bridge 回调 → POST http://backend:8766/callback
                                              │
                                              ▼
                              C++ Backend 处理回调结果
                              → TTS 语音合成
                              → Live2D 表情/动作指令
                              → WebSocket 推送到前端
                                              │
                                              ▼
                              前端播放语音 + Live2D 动画
```

---

## ⚙ 运行逻辑

### 1. 后端启动流程

后端入口为 `backend/src/main.cpp`，通过 **Application 单例模式** 管控全生命周期：

```
main()
  │
  ├── Application::getInstance()          // 获取全局单例
  │
  ├── app->initialize(argc, argv)         // 初始化（详见下方）
  │
  ├── app->start()                        // 启动 HTTP 服务器，注册信号处理
  │
  └── app->wait()                         // 阻塞等待，直到收到终止信号
```

#### `initialize()` 初始化顺序

初始化严格按照依赖顺序执行，每一步都必须在下一步之前完成：

```
initialize()
  │
  ├── 1. parseArguments()                 // 解析命令行参数
  │       --config-dir   配置文件目录 (默认: config)
  │       --env          运行环境 (默认: dev)
  │       --port         HTTP 端口 (默认: 8080)
  │       --host         监听地址 (默认: 0.0.0.0)
  │       --workers      工作线程数 (默认: CPU核心数)
  │
  ├── 2. ConfigManager::initialize()      // 加载 YAML 配置
  │       读取 config/ 目录下的 config.yaml 等配置文件
  │
  ├── 3. LogUtils::initialize()           // 初始化日志系统 (spdlog)
  │       日志级别、文件路径、控制台输出
  │
  ├── 4. initializeHttpServer()           // 创建 Crow HTTP 服务器
  │       ├── 绑定 host:port
  │       ├── 设置工作线程数
  │       ├── 配置 CORS 跨域
  │       ├── 配置 gzip 压缩
  │       ├── 配置请求速率限制
  │       └── (可选) 配置 SSL/TLS
  │
  ├── 5. initializeDatabase()             // 创建 PostgreSQL 连接池
  │       通过 pqxx 建立到 PostgreSQL 的连接池
  │       连接字符串: host + port + dbname + user + password
  │       默认连接池大小: 10
  │
  ├── 6. initializeAIServices()           // 初始化 AI 提供商配置
  │       支持 openai / deepseek / qwen 等多种 AI 后端
  │       配置 apiKey、baseUrl、model
  │
  ├── 7. initializeServices()             // 初始化 WebSocket + Redis
  │       ├── WebSocket 服务器 (默认 0.0.0.0:9001)
  │       │   在独立后台线程中启动 (std::thread + detach)
  │       └── Redis 连接池 (如启用)
  │           hiredis 连接到 Redis 服务器
  │
  └── 8. initializeControllers()          // 注册路由控制器
          最后执行，确保所有依赖服务就绪
          ├── /api/v1/health   ← HealthController
          ├── /api/v1/auth     ← AuthController (JWT)
          ├── /api/v2/ai       ← AIController
          ├── /api/v1/users    ← UserController
          ├── /api/v1/posts    ← PostController
          └── /api/v1/messages ← MessageController
              (仅当 Database + WebSocket 均初始化成功时注册)
```

#### `start()` 启动逻辑

```
start()
  ├── httpServer->start()                 // 启动 Crow HTTP 监听
  ├── registerSignalHandlers()            // 注册 SIGINT/SIGTERM 信号处理
  └── running = true                      // 标记运行状态
```

#### `wait()` → `stop()` 生命周期

- `wait()` 阻塞主线程，等待 HTTP 服务器退出
- 收到 `SIGINT`/`SIGTERM` 时触发 `stop()`
- `stop()` 依次关闭 HTTP 服务器 → 数据库连接池 → Redis 连接池

### 2. Docker Compose 启动顺序

`docker-compose up` 时，服务按依赖关系启动：

```
                    ┌──────────┐   ┌──────────┐
                    │ postgres │   │  redis   │
                    │ (健康检查) │   │ (健康检查) │
                    └─────┬────┘   └────┬─────┘
                          │  healthy    │ healthy
                          └──────┬──────┘
                                 ▼
                    ┌────────────────────────┐     ┌──────────┐
                    │   backend (:8080/9001) │     │  bridge  │
                    │   等待 PG+Redis 健康    │     │(:8765/66)│
                    └────────────┬───────────┘     └──────────┘
                                 │
                                 ▼
                    ┌────────────────────────┐
                    │   frontend (:3000)     │
                    │   等待 backend 就绪     │
                    └────────────┬───────────┘
                                 │
                                 ▼
                    ┌────────────────────────┐
                    │   nginx (:80/:443)     │
                    │   等待 backend+frontend │
                    └────────────────────────┘

    独立启动（无依赖）：pgadmin, redis-commander, prometheus, grafana
```

### 3. 数据流完整链路

**用户发送弹幕到 AI 回复的完整流程：**

```
1. 用户在前端输入弹幕消息
2. 前端通过 WebSocket (:9001) 发送消息到后端
3. 后端接收消息，进行内容审核
4. 后端将消息 POST 到 Bridge (:8765/process)
   └── 携带: userId, sessionId, message, context
5. Bridge 管理用户会话 (TTL 3600s)，转发到 OpenClaw (:8000)
6. OpenClaw AI Agent 处理对话：
   ├── 生成回复文本
   ├── 情感分析 (happy/sad/angry/surprise/...)
   └── 动作指令 (wave/nod/dance/...)
7. OpenClaw 返回结果给 Bridge
8. Bridge 将结果回调到后端 (:8766/callback)
9. 后端处理回调：
   ├── TTS 语音合成 → 生成音频数据
   ├── 映射情感标签 → Live2D 表情参数
   └── 映射动作指令 → Live2D 动作参数
10. 后端通过 WebSocket 推送到前端：
    ├── AI 回复文本
    ├── 音频数据 (Base64)
    ├── Live2D 表情指令
    └── Live2D 动作指令
11. 前端接收并执行：
    ├── 显示弹幕回复
    ├── Web Audio API 播放语音
    └── Live2D Cubism SDK 播放表情+动作
```

---

## 🛠 技术栈

### 后端 (C++20)

| 组件 | 技术 |
|------|------|
| HTTP 框架 | [Crow](https://github.com/CrowCpp/Crow) |
| 数据库 | PostgreSQL 15 + [pqxx](https://github.com/jtv/libpqxx) |
| 缓存 | Redis 7 + [hiredis](https://github.com/redis/hiredis) |
| JSON | [nlohmann/json](https://github.com/nlohmann/json) |
| 配置 | [yaml-cpp](https://github.com/jbeder/yaml-cpp) |
| 日志 | [spdlog](https://github.com/gabime/spdlog) |
| HTTP 客户端 | libcurl |
| 加密 | OpenSSL (JWT, Hash) |
| 构建 | CMake 3.20+ |

### 前端 (Vue 3)

| 组件 | 技术 |
|------|------|
| 框架 | Vue 3 + TypeScript |
| 构建工具 | Vite |
| UI 组件库 | Element Plus |
| 状态管理 | Pinia |
| 路由 | Vue Router 4 |
| HTTP | Axios |
| 虚拟形象 | Live2D Cubism SDK |
| 语音 | Web Audio API |

### 桥接服务 (Node.js)

| 组件 | 技术 |
|------|------|
| 运行时 | Node.js 18 |
| HTTP 框架 | Express |
| HTTP 客户端 | Axios |
| 会话管理 | 内置 Map + TTL 自动清理 |
| 日志 | Winston |
| 配置 | dotenv |

### 基础设施

| 组件 | 技术 |
|------|------|
| 容器化 | Docker + Docker Compose |
| 反向代理 | Nginx |
| 数据库 | PostgreSQL 15 |
| 缓存 | Redis 7 |
| 监控 | Prometheus + Grafana |
| AI 编排 | OpenClaw |

---

## 📁 项目结构

```
Yachiyo/
├── backend/                          # C++20 后端服务
│   ├── CMakeLists.txt               # CMake 构建配置
│   ├── Dockerfile                   # 后端容器镜像
│   ├── config/                      # 配置文件 (YAML)
│   ├── include/                     # 头文件
│   │   ├── Application.hpp          # 应用程序主类
│   │   ├── config/                  # 配置管理
│   │   ├── controllers/             # 路由控制器
│   │   ├── dto/                     # 数据传输对象
│   │   ├── mappers/                 # 数据映射
│   │   ├── models/                  # 数据模型
│   │   ├── services/                # 业务服务
│   │   └── utils/                   # 工具类
│   ├── src/                         # 源文件
│   │   ├── main.cpp                 # 入口点
│   │   ├── Application.cpp          # 应用程序实现
│   │   ├── controllers/             # 控制器实现
│   │   ├── services/                # 服务实现
│   │   └── utils/                   # 工具实现
│   ├── sql/                         # 数据库迁移脚本
│   └── test/                        # 单元测试
├── frontend/                        # Vue 3 前端
│   ├── Dockerfile                   # 前端容器镜像
│   ├── package.json                 # npm 依赖
│   ├── vite.config.ts               # Vite 配置
│   └── src/                         # 前端源码
├── bridge/                          # Node.js 桥接服务
│   ├── Dockerfile                   # 桥接服务容器镜像
│   ├── package.json                 # npm 依赖
│   └── src/
│       ├── index.js                 # 入口：双端口 Express 服务器
│       ├── session.js               # 用户会话管理 (TTL)
│       └── logger.js                # Winston 日志
├── resources/                       # 静态资源
│   ├── live2d/                      # Live2D 模型文件
│   └── config/                      # 资源配置
├── database/                        # 数据库初始化脚本
├── config/                          # 全局配置
├── devops/                          # 部署脚本
├── docs/                            # 项目文档
├── nginx.conf                       # Nginx 配置
├── docker-compose.yml               # Docker Compose 编排
├── build.sh                         # 构建脚本 (Linux)
└── build.ps1                        # 构建脚本 (Windows)
```

---

## 🚀 快速开始

### 前置要求

- Docker 20.10+ & Docker Compose v2
- (可选) OpenClaw 运行在 `http://localhost:8000`

### 一键启动

```bash
# 1. 克隆仓库
git clone https://github.com/ermaotie6/yachiyoooooooo.git
cd yachiyoooooooo

# 2. 配置环境变量
cp backend/.env.example backend/.env
cp bridge/.env.example bridge/.env
cp frontend/.env.example frontend/.env

# 编辑 .env 文件，填入你的 API Key:
#   DEEPSEEK_API_KEY=sk-xxx
#   JWT_SECRET_KEY=your-secret-key
#   OPENCLAW_ENDPOINT=http://host.docker.internal:8000  (如果用 OpenClaw)

# 3. 启动所有服务
docker-compose up -d

# 4. 查看日志
docker-compose logs -f backend
docker-compose logs -f bridge

# 5. 访问
#   前端:  http://localhost (通过 Nginx)
#   API:   http://localhost:8080/api/v1/health
#   pgAdmin: http://localhost:5050
#   Grafana: http://localhost:3001
```

### 本地开发（不使用 Docker）

```bash
# 后端
cd backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
./yachiyo_cpp --config-dir ../config --env dev --port 8080

# 前端
cd frontend
npm install
npm run dev

# 桥接服务
cd bridge
npm install
node src/index.js
```

---

## 📡 API 端点

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/v1/health` | 健康检查 | ❌ |
| POST | `/api/v1/auth/register` | 用户注册 | ❌ |
| POST | `/api/v1/auth/login` | 用户登录 | ❌ |
| GET | `/api/v1/users` | 用户列表 | ✅ |
| GET | `/api/v1/users/:id` | 用户详情 | ✅ |
| GET | `/api/v1/posts` | 帖子列表 | ❌ |
| POST | `/api/v1/posts` | 发布帖子 | ✅ |
| POST | `/api/v2/ai/chat` | AI 对话 | ✅ |
| WS | `ws://host:9001` | WebSocket 实时通信 | ✅ |
| GET | `/api/v1/messages` | 消息记录 | ✅ |

---

## 🐳 Docker 服务一览

| 服务 | 容器名 | 端口 | 说明 |
|------|--------|------|------|
| postgres | yachiyo-postgres | 5432 | PostgreSQL 15 数据库 |
| redis | yachiyo-redis | 6379 | Redis 7 缓存 |
| backend | yachiyo-backend | 8080, 9001 | C++ 后端 (HTTP + WebSocket) |
| frontend | yachiyo-frontend | 3000 | Vue 3 前端 |
| nginx | yachiyo-nginx | 80, 443 | 反向代理 |
| bridge | yachiyo-bridge | 8765, 8766 | Node.js 桥接服务 (接收 + 回调) |
| pgadmin | yachiyo-pgadmin | 5050 | PostgreSQL 管理工具 |
| redis-commander | yachiyo-redis-commander | 8082 | Redis 管理工具 |
| prometheus | yachiyo-prometheus | 9091 | 监控指标采集 |
| grafana | yachiyo-grafana | 3001 | 监控数据可视化 |

---

## ⚙ 配置说明

### 后端核心配置 (`backend/config/config.yaml`)

```yaml
server:
  host: "0.0.0.0"
  port: 8080
  workers: 4                    # HTTP 工作线程数
  cors:
    enabled: true
    origin: "*"
  rateLimit:
    enabled: true
    maxRequests: 100
    windowSeconds: 60

database:
  host: "localhost"
  port: 5432
  name: "yachiyo"
  user: "postgres"
  password: "postgres"
  poolSize: 10

websocket:
  host: "0.0.0.0"
  port: 9001

redis:
  enabled: true
  host: "localhost"
  port: 6379

ai:
  provider: "deepseek"           # openai / deepseek / qwen
  model: "deepseek-chat"

openclaw:
  bridge_endpoint: "http://localhost:8765"
  callback_port: 8766
```

### 关键环境变量

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `JWT_SECRET_KEY` | JWT 签名密钥 | (必填) |
| `DEEPSEEK_API_KEY` | DeepSeek API Key | (选填) |
| `OPENCLAW_ENDPOINT` | OpenClaw 服务地址 | `http://host.docker.internal:8000` |
| `OPENCLAW_BRIDGE_ENDPOINT` | 桥接服务地址 | `http://bridge:8765` |
| `DATABASE_HOST` | PostgreSQL 地址 | `postgres` |
| `REDIS_URL` | Redis 连接地址 | `redis://redis:6379` |

---

## 📄 许可证

[MIT License](LICENSE) © Yachiyo Project
