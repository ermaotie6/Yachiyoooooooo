# Yachiyo — AI 虚拟形象直播平台

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

## 项目简介

Yachiyo（八千代辉夜姬）是一个 AI 虚拟形象直播互动平台。用户通过弹幕与 AI 驱动的 Live2D 虚拟形象实时交互——发送弹幕后，系统完成内容审核、AI 对话、翻译（日语）、语音合成（GPT-SoVITS）、Live2D 表情/动作控制，最终通过 WebSocket 将文本、音频和动画指令推送到前端。

**对话推理由外部服务 OpenClaw 完成**（通过 Node.js 桥接中转）。本项目的职责是：

1. **接收弹幕** → 6 层内容审核
2. **转发到 OpenClaw** → 获取 AI 回复 + 情感 + 动作
3. **后处理** → 翻译日语 → GPT-SoVITS 语音合成 → Live2D 动画
4. **实时推送** → WebSocket 广播文本、音频、动画指令

---

## 功能概览

| 模块 | 说明 | 状态 |
| --- | --- | --- |
| 用户认证 | JWT 注册/登录/刷新，SHA-256+Salt 加密 | ✅ |
| 弹幕审核 | 6 层审核（速率限制 + IP + 关键词 + AI语义 + 行为分析 + 综合评分） | ✅ |
| OpenClaw 对接 | 通过 Bridge 转发弹幕，接收回复 + 情感 + 动作 | ✅ 框架就绪 |
| 翻译服务 | 百度翻译（主）+ DeepSeek LLM（备），自动降级 | ✅ |
| TTS 语音合成 | GPT-SoVITS 日语语音 + 情感参考音频 | ✅ 框架就绪 |
| Live2D 驱动 | 情感→表情映射 + 动作→Motion + Web Audio 口型同步 | ✅ |
| WebSocket | 全链路实时推送，用户消息 + AI 回复广播 | ✅ |
| 消息持久化 | PostgreSQL 存储消息、审核状态、对话上下文 | ✅ |
| Redis 缓存 | 速率限制、Token 黑名单、翻译/TTS 缓存 | ✅ |
| 监控 | Prometheus + Grafana | ✅ |

---

## 技术栈

### 后端 (C++20)

| 组件 | 技术 |
| --- | --- |
| HTTP 框架 | [Crow](https://github.com/CrowCpp/Crow) |
| 数据库 | PostgreSQL 15 ([libpqxx](https://github.com/jtv/libpqxx)) |
| 缓存 | Redis 7 ([hiredis](https://github.com/redis/hiredis)) |
| JSON | [nlohmann/json](https://github.com/nlohmann/json) |
| 配置 | [yaml-cpp](https://github.com/jbeder/yaml-cpp) |
| 日志 | [spdlog](https://github.com/gabime/spdlog) |
| HTTP 客户端 | libcurl |
| 加密 | OpenSSL (JWT + SHA-256) |
| 构建 | CMake 3.20+ |
| 测试 | Google Test |

### 前端 (Vue 3 + TypeScript)

| 组件 | 技术 |
| --- | --- |
| 框架 | Vue 3 (Composition API) + Vite |
| UI | Element Plus |
| 状态管理 | Pinia |
| Live2D | pixi.js + pixi-live2d-display |
| 语音 | Web Audio API |

### 基础设施

| 组件 | 技术 |
| --- | --- |
| 容器 | Docker Compose 3.8 (10 个服务) |
| 反向代理 | Nginx |
| 监控 | Prometheus + Grafana |
| 管理工具 | pgAdmin + Redis Commander |

---

## 快速开始

### 前置要求

- Docker 20.10+ & Docker Compose v2
- Git

### Docker Compose 启动（推荐）

```bash
# 1. 克隆
git clone https://github.com/ermaotie6/yachiyoooooooo.git
cd yachiyoooooooo

# 2. 创建 .env
cat > .env << 'EOF'
JWT_SECRET_KEY=your-random-secret-at-least-32-chars
DEEPSEEK_API_KEY=sk-your-deepseek-key
BAIDU_TRANSLATE_APPID=20150630000000001
BAIDU_TRANSLATE_API_KEY=your-baidu-translate-key
DB_PASSWORD=postgres
EOF

# 3. 启动
docker compose up -d

# 4. 验证（首次构建 C++ 后端约 5-15 分钟）
docker compose logs -f backend
curl http://localhost:8080/api/v1/health

# 5. 访问
#   前端:     http://localhost
#   pgAdmin:  http://localhost:5050
#   Grafana:  http://localhost:3001
```

### 本地开发

```bash
# 启动基础设施
docker compose up -d postgres redis

# 后端
cd backend && mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20
make -j$(nproc)
./yachiyo_cpp --config-dir ../config --env dev --port 8080

# 前端
cd frontend && npm install && npm run dev

# 桥接
cd bridge && npm install && node src/index.js
```

> 后端编译依赖详见 [开发指南](docs/开发指南.md)。

---

## 项目结构

```
Yachiyo/
├── backend/                 # C++20 后端
│   ├── CMakeLists.txt
│   ├── Dockerfile
│   ├── config/              # Docker 部署配置
│   ├── include/             # 头文件
│   │   ├── controllers/     # 路由控制器
│   │   ├── services/        # 业务服务
│   │   ├── models/          # 数据模型
│   │   ├── dto/             # 数据传输对象
│   │   └── utils/           # 工具类
│   ├── src/                 # 源文件
│   └── tests/               # Google Test 测试
│
├── frontend/                # Vue 3 前端
│   ├── src/
│   │   ├── views/           # 页面组件 (LiveStream.vue ⭐)
│   │   ├── components/      # 复用组件 (Live2DComponent.vue)
│   │   ├── composables/     # 组合式函数 (useWebSocket.ts)
│   │   └── stores/          # Pinia 状态管理
│   └── vite.config.ts
│
├── bridge/                  # Node.js 桥接 (JSON 转发到 OpenClaw)
├── resources/live2d/        # Live2D 模型文件
├── database/init.sql        # 数据库初始化
├── config/config.yaml       # 本地开发配置
├── nginx.conf               # Nginx 反向代理
├── docker-compose.yml       # 10 服务编排
└── docs/                    # 项目文档
```

---

## 环境变量

| 变量 | 说明 | 必填 | 默认值 |
| --- | --- | --- | --- |
| `JWT_SECRET_KEY` | JWT 签名密钥 (≥32字符) | ✅ | - |
| `DB_PASSWORD` | PostgreSQL 密码 | ✅ | - |
| `DEEPSEEK_API_KEY` | DeepSeek API Key | 选填 | - |
| `BAIDU_TRANSLATE_APPID` | 百度翻译 APPID | 选填 | - |
| `BAIDU_TRANSLATE_API_KEY` | 百度翻译密钥 | 选填 | - |
| `DB_HOST` | PostgreSQL 地址 | 选填 | `localhost` |
| `REDIS_HOST` | Redis 地址 | 选填 | `localhost` |
| `OPENCLAW_BRIDGE_ENDPOINT` | 桥接服务地址 | 选填 | `http://localhost:8765` |

---

## 文档索引

| 文档 | 说明 |
| --- | --- |
| [架构设计](docs/架构设计.md) | 系统架构、业务流程、WebSocket 协议、审核系统、Live2D 集成 |
| [API 参考](docs/API参考.md) | HTTP API 端点、数据库表结构、Redis 数据结构 |
| [开发指南](docs/开发指南.md) | 环境搭建、代码规范、调试技巧、Git 工作流 |
| [部署指南](docs/部署指南.md) | Docker / 裸机部署、HTTPS、安全加固 |
| [外部服务对接指南](docs/外部服务对接指南.md) | OpenClaw / GPT-SoVITS / 翻译 / 审核对接 |
| [GPT-SoVITS 部署指南](docs/GPT-SoVITS部署指南.md) | TTS 训练 (Windows) + 部署 (Linux) |
| [功能状态](docs/功能状态.md) | 功能模块实现状态跟踪 |

---

## 许可证

MIT License
