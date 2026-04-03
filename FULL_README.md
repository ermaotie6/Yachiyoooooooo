# Yachiyo - AI 虚拟形象直播平台

**一个完整的 AI 虚拟形象直播平台，以 OpenClaw 自主人工智能虚拟助理为核心，提供实时动画展示、WebSocket 实时通讯和智能内容审核的完整生产环境方案。**

![Version](https://img.shields.io/badge/version-2.0.0-brightgreen)
![License](https://img.shields.io/badge/license-MIT-blue)
![Status](https://img.shields.io/badge/status-Production%20Ready-success)
![Language](https://img.shields.io/badge/Backend-C%2B%2B20-red)
![Language](https://img.shields.io/badge/Frontend-Vue3%2FTypeScript-green)

---

## 📋 目录

- [快速开始](#快速开始)
- [系统架构](#系统架构)
- [核心功能](#核心功能)
- [项目结构](#项目结构)
- [安装配置](#安装配置)
- [使用指南](#使用指南)
- [API 文档](#api-文档)
- [开发与部署](#开发与部署)
- [常见问题](#常见问题)
- [贡献指南](#贡献指南)

---

## 🚀 快速开始

### 前提条件

- **后端**：C++20, CMake 3.20+, PostgreSQL 15+, Redis 7+
- **前端**：Node.js 18+, npm 9+
- **Docker**（可选）：Docker 20.10+, Docker Compose 2.0+

### 使用 Docker Compose（推荐）

```bash
# 克隆项目
git clone https://github.com/yachiyoooooooo/Yachiyo.git
cd Yachiyo

# 启动所有服务
docker-compose up -d

# 查看日志
docker-compose logs -f backend

# 停止服务
docker-compose down
```

**服务地址**：
- 前端：http://localhost:3000
- 后端 API：http://localhost:8080
- WebSocket：ws://localhost:8081
- PostgreSQL：localhost:5432（用户：postgres，密码：postgres）
- Redis：localhost:6379
- pgAdmin：http://localhost:5050
- Redis Commander：http://localhost:8082
- Grafana：http://localhost:3001

### 本地开发

#### 后端

```bash
cd backend

# 创建构建目录
mkdir -p build
cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 构建项目
make -j$(nproc)

# 运行应用
./src/yachiyo_cpp --config-dir ../config --env development
```

#### 前端

```bash
cd frontend

# 安装依赖
npm install

# 启动开发服务器
npm run dev

# 构建生产版本
npm run build

# 预览生产构建
npm run preview
```

---

## 🏗️ 系统架构

### 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                     Yachiyo 平台架构                          │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────┐         ┌──────────────────┐          │
│  │   Vue 3 前端      │         │  Nginx 反向代理   │          │
│  │ (TypeScript)      │◄────────┤  (SSL/TLS)       │          │
│  │ - LiveStream      │         │                  │          │
│  │ - Chat            │         │  ┌──────────────┐│          │
│  │ - Live2D          │         │  │ 域名解析      ││          │
│  │ - Admin           │         │  │ 负载均衡      ││          │
│  └──────────────────┘         └──────────────────┘          │
│           │                                                   │
│           │ HTTP REST API                                     │
│           │ WebSocket (WS)                                    │
│           ▼                                                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │          C++ 20 后端服务 (Crow Framework)             │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  API Controllers     Services          Utilities     │   │
│  │  ├─ Message          ├─ AI Service     ├─ JWT       │   │
│  │  ├─ User             ├─ Chat Service   ├─ Logger    │   │
│  │  ├─ Auth             ├─ WebSocket      ├─ Database  │   │
│  │  ├─ Admin            └─ Moderation     └─ Cache     │   │
│  │  └─ Health                                           │   │
│  └──────────────────────────────────────────────────────┘   │
│           │              │            │         │           │
│           ▼              ▼            ▼         ▼           │
│  ┌──────────────┐ ┌────────────┐ ┌─────────┐ ┌───────┐    │
│  │ PostgreSQL   │ │   Redis    │ │ Openclaw│ │OpenAI │    │
│  │ 数据库       │ │   缓存     │ │ 审核    │ │ 代理  │    │
│  │              │ │            │ │         │ │       │    │
│  │ Messages     │ │ Sessions   │ │ Filter  │ │ LLM   │    │
│  │ Users        │ │ Tokens     │ │ Check   │ │ 调用  │    │
│  │ Moderation   │ │ Contexts   │ │ Score   │ │       │    │
│  │ Logs         │ │            │ │         │ │       │    │
│  └──────────────┘ └────────────┘ └─────────┘ └───────┘    │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### 核心模块

| 模块 | 说明 | 技术栈 |
|------|------|---------|
| **前端** | Vue 3 + TypeScript + Vite | 动态UI、实时通讯、Live2D渲染 |
| **后端API** | C++20 + Crow + REST | 控制器、服务、DAO |
| **WebSocket** | 原生 C++ | 实时消息、连接管理 |
| **数据库** | PostgreSQL | 用户、消息、审核日志 |
| **缓存** | Redis | 会话、令牌、对话上下文 |
| **AI服务** | OpenAI/DeepSeek/Openclaw | LLM、内容审核 |
| **TTS** | GPT-SoVITS | 文字转语音 |
| **Live2D** | Live2D SDK | 虚拟形象动画 |
| **部署** | Docker + Compose | 容器化、编排 |

---

## ✨ 核心功能

### 直播功能
- ✅ **实时Live2D虚拟形象动画** - 嘴部与语音同步
- ✅ **WebSocket 实时通讯** - 低延迟消息传输
- ✅ **多用户在线** - 支持 10,000+ 并发连接
- ✅ **聊天系统** - 实时消息、弹幕、@提及

### AI交互
- ✅ **多AI支持** - OpenAI、DeepSeek、OpenClaw、本地Ollama
- ✅ **对话上下文管理** - 记忆对话历史
- ✅ **智能回复生成** - 基于用户消息
- ✅ **语音合成** - GPT-SoVITS 高质量TTS

### 内容审核
- ✅ **实时审核** - Openclaw内容安全检测
- ✅ **多维度评分** - 违规类型、严重程度、置信度
- ✅ **自动处理** - 违规消息隐藏/删除
- ✅ **管理员面板** - 审核日志、高风险消息查看

### 用户系统
- ✅ **用户认证** - JWT令牌、刷新令牌
- ✅ **权限管理** - 普通用户、管理员
- ✅ **个人资料** - 头像、昵称、偏好设置
- ✅ **会话管理** - 登录、登出、会话过期

### 管理功能
- ✅ **用户管理** - 列表、编辑、禁用
- ✅ **消息管理** - 查看、搜索、删除
- ✅ **内容审核** - 审查、批准、拒绝
- ✅ **统计分析** - 活跃用户、消息数量、违规率

---

## 📁 项目结构

```
Yachiyo/
├── backend/                      # C++ 后端服务
│   ├── include/                  # 头文件
│   │   ├── Application.hpp       # 应用主类
│   │   ├── config/               # 配置管理
│   │   ├── controllers/          # 控制器层
│   │   │   ├── BaseController.hpp
│   │   │   ├── MessageController.hpp
│   │   │   ├── AuthController.hpp
│   │   │   └── ...
│   │   ├── services/             # 服务层
│   │   │   ├── DatabaseService.hpp
│   │   │   ├── WebSocketService.hpp
│   │   │   ├── MessageService.hpp
│   │   │   └── ...
│   │   ├── models/               # 数据模型
│   │   │   ├── User.hpp
│   │   │   ├── Message.hpp
│   │   │   └── ...
│   │   ├── dto/                  # 数据传输对象
│   │   └── utils/                # 工具函数
│   │       ├── JwtUtil.hpp
│   │       ├── Logger.hpp
│   │       ├── DatabaseUtil.hpp
│   │       └── ...
│   ├── src/                      # 实现文件
│   │   ├── Application.cpp
│   │   ├── main.cpp
│   │   ├── services/
│   │   │   ├── DatabaseService.cpp
│   │   │   ├── WebSocketService.cpp
│   │   │   └── ...
│   │   ├── controllers/
│   │   └── ...
│   ├── config/                   # 配置文件
│   │   ├── config.yaml
│   │   └── .env.example
│   ├── sql/                      # 数据库脚本
│   │   └── init.sql
│   ├── Dockerfile                # Docker镜像定义
│   ├── docker-compose.yml        # Docker编排配置
│   ├── CMakeLists.txt            # CMake构建配置
│   └── build.sh                  # 构建脚本
│
├── frontend/                     # Vue 3 前端应用
│   ├── src/
│   │   ├── main.ts               # 应用入口
│   │   ├── App.vue               # 根组件
│   │   ├── views/                # 页面组件
│   │   │   ├── Home.vue
│   │   │   ├── LiveStream.vue    # 直播间
│   │   │   ├── Chat.vue
│   │   │   ├── Admin.vue
│   │   │   └── ...
│   │   ├── components/           # 可复用组件
│   │   │   ├── Live2DComponent.vue
│   │   │   ├── ChatBox.vue
│   │   │   ├── MessageBoard.vue
│   │   │   └── ...
│   │   ├── composables/          # 组合式函数
│   │   │   ├── useWebSocket.ts   # WebSocket连接管理
│   │   │   ├── useAudioPlayer.ts # 音频播放
│   │   │   └── ...
│   │   ├── stores/               # Pinia状态管理
│   │   │   ├── auth.ts
│   │   │   ├── chat.ts
│   │   │   └── ...
│   │   ├── router/
│   │   │   └── index.ts          # 路由配置
│   │   ├── api/                  # API调用
│   │   │   ├── auth.ts
│   │   │   ├── messages.ts
│   │   │   └── ...
│   │   └── types/                # TypeScript类型定义
│   ├── vite.config.ts            # Vite配置
│   ├── tsconfig.json
│   ├── package.json
│   ├── Dockerfile
│   └── .env.example
│
├── docs/                         # 文档
│   ├── API.md                    # API文档
│   ├── DEPLOYMENT.md             # 部署指南
│   ├── ARCHITECTURE.md           # 架构设计
│   └── ...
│
├── scripts/                      # 部署脚本
│   ├── start.sh                  # 启动脚本
│   ├── stop.sh                   # 停止脚本
│   └── deploy.sh                 # 部署脚本
│
├── docker-compose.yml            # 完整容器编排配置
├── nginx.conf                    # Nginx配置
├── README.md                     # 项目说明（本文件）
└── .env.example                  # 环境变量示例
```

---

## 🔧 安装配置

### 环境变量

复制 `.env.example` 创建 `.env` 文件：

```bash
# 后端配置
cp backend/.env.example backend/.env
# 前端配置
cp frontend/.env.example frontend/.env.local
```

编辑环境变量文件填入实际配置。

### 数据库初始化

PostgreSQL 初始化脚本在 Docker 启动时自动执行，或手动运行：

```bash
psql -h localhost -U postgres -d yachiyo -f backend/sql/init.sql
```

### 依赖安装

#### 后端依赖

```bash
cd backend

# Ubuntu/Debian
sudo apt-get install -y cmake g++ ninja-build libpq-dev nlohmann-json3-dev

# macOS
brew install cmake ninja libpq nlohmann-json
```

#### 前端依赖

```bash
cd frontend
npm install
```

---

## 📖 使用指南

### 后端启动

**本地开发**：
```bash
cd backend/build
./src/yachiyo_cpp --config-dir ../config --env development --port 8080
```

**命令行参数**：
```
--config-dir <dir>      配置文件目录 (默认: config)
--env <env>             运行环境: development/production (默认: dev)
--port <port>           HTTP监听端口 (默认: 8080)
--host <host>           监听地址 (默认: 0.0.0.0)
--workers <num>         工作线程数 (默认: CPU核心数)
--help                  显示帮助信息
```

### 前端启动

**开发模式**：
```bash
cd frontend
npm run dev
```

**生产构建**：
```bash
npm run build
npm run preview
```

### WebSocket 连接

前端会自动连接 WebSocket：

```typescript
import { useWebSocket } from '@/composables/useWebSocket'

const { connect, disconnect, send } = useWebSocket()

// 建立连接
connect('ws://localhost:8081')

// 发送消息
send({
  type: 'message',
  data: {
    message: 'Hello!',
    conversation_id: 123
  }
})

// 断开连接
disconnect()
```

---

## 📚 API 文档

### 基础信息

- **基础URL**：http://localhost:8080/api/v1
- **认证**：JWT Bearer Token
- **返回格式**：JSON

### 主要端点

#### 认证相关

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/auth/register` | 用户注册 |
| POST | `/auth/login` | 用户登录 |
| POST | `/auth/refresh` | 刷新令牌 |
| POST | `/auth/logout` | 用户登出 |

#### 消息相关

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/messages/send` | 发送消息 |
| GET | `/messages/list` | 获取消息列表 |
| GET | `/messages/context` | 获取对话上下文 |
| GET | `/messages/pending` | 获取待审查消息（管理员） |
| POST | `/messages/review` | 审查消息（管理员） |
| POST | `/messages/delete` | 删除消息 |
| GET | `/messages/high-risk` | 获取高风险消息（管理员） |

#### 用户相关

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/users/profile` | 获取用户资料 |
| PUT | `/users/profile` | 更新用户资料 |
| PUT | `/users/preferences` | 更新用户偏好 |
| GET | `/users/list` | 用户列表（管理员） |

### 请求示例

```bash
# 登录
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "username": "user",
    "password": "password"
  }'

# 发送消息
curl -X POST http://localhost:8080/api/v1/messages/send \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{
    "message": "Hello!",
    "conversation_id": 123
  }'
```

详见 [完整API文档](docs/API.md)。

---

## 🚀 开发与部署

### 本地开发流程

1. **创建功能分支**：
   ```bash
   git checkout -b feature/xxx
   ```

2. **开发并测试**：
   ```bash
   # 后端测试
   cd backend/build
   make test
   
   # 前端测试
   cd frontend
   npm run test
   ```

3. **提交更改**：
   ```bash
   git add .
   git commit -m "feat: 添加XXX功能"
   git push origin feature/xxx
   ```

4. **创建 Pull Request**

### 生产部署

#### 方式1：Docker Compose（推荐）

```bash
# 编辑 .env 配置生产参数
vim .env

# 启动服务
docker-compose -f docker-compose.yml up -d

# 查看日志
docker-compose logs -f backend

# 更新镜像
docker-compose pull
docker-compose up -d
```

#### 方式2：Kubernetes（高可用）

详见 [Kubernetes部署文档](docs/KUBERNETES.md)。

#### 方式3：传统服务器

1. 编译后端：
   ```bash
   cd backend && mkdir build && cd build
   cmake .. && make install
   ```

2. 构建前端：
   ```bash
   cd frontend && npm run build
   ```

3. 配置 Nginx 反向代理

4. 使用 systemd 管理服务

### 监控与维护

- **监控仪表板**：http://localhost:3001 (Grafana)
- **性能指标**：Prometheus http://localhost:9091
- **数据库管理**：pgAdmin http://localhost:5050
- **缓存管理**：Redis Commander http://localhost:8082

---

## ❓ 常见问题

### Q1: 如何更改数据库密码？

编辑 `.env` 文件，修改 `DATABASE_PASSWORD`，然后重启容器。

### Q2: WebSocket 连接失败怎么办？

检查防火墙配置，确保 8081 端口开放，查看后端日志定位问题。

### Q3: 如何添加新的AI模型？

1. 在 `include/services/AIService.hpp` 中定义新的服务接口
2. 实现具体的 AI 服务类
3. 在 Application.cpp 中注册服务
4. 更新 .env 配置

### Q4: 如何扩展到多个服务器？

使用 Docker Swarm 或 Kubernetes 编排多个容器副本，配置负载均衡器。

### Q5: 性能瓶颈在哪里？

主要瓶颈：
- 数据库查询 → 优化SQL、添加索引
- AI 响应 → 使用缓存、异步处理
- 消息队列 → 增加 Redis 节点
- 前端渲染 → 代码分割、懒加载

---

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

### 贡献流程

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 代码规范

- **C++**：遵循 C++20 标准，使用 snake_case 命名
- **TypeScript**：遵循 ESLint 配置，使用 camelCase 命名
- **提交消息**：使用 Conventional Commits 格式

---

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

## 📞 联系方式

- **问题反馈**：[GitHub Issues](https://github.com/yachiyoooooooo/Yachiyo/issues)
- **讨论区**：[GitHub Discussions](https://github.com/yachiyoooooooo/Yachiyo/discussions)
- **邮箱**：contact@yachiyo.com

---

## 🙏 致谢

感谢所有贡献者和支持者！

特别感谢：
- [Crow Framework](https://github.com/CrowCpp/Crow) - C++ Web框架
- [Vue.js](https://vuejs.org/) - 前端框架
- [OpenClaw](https://openclaw.ai/) - AI内容审核
- [Live2D](https://www.live2d.com/) - 虚拟形象动画

---

**最后更新**：2024年01月15日  
**版本**：2.0.0  
**状态**：✅ 生产就绪
