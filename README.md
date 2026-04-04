# 🎀 Yachiyo — AI 虚拟形象直播平台

<div align="center">

**基于 C++20 和 Vue 3 的 AI 虚拟主播交互平台**

Live2D 形象 · 实时聊天 · 语音合成 · 内容审核 · AI 对话

[![Version](https://img.shields.io/badge/version-3.0.0-brightgreen)](#)
[![License](https://img.shields.io/badge/license-MIT-blue)](./LICENSE)
[![Backend](https://img.shields.io/badge/Backend-C%2B%2B20%2FCrow-orange)](#)
[![Frontend](https://img.shields.io/badge/Frontend-Vue3%2FTypeScript-green)](#)
[![Docker](https://img.shields.io/badge/Deploy-Docker%20Compose-blue)](#)

</div>

---

## 功能概览

| 模块 | 说明 | 状态 |
|------|------|------|
| 💬 AI 对话 | DeepSeek / OpenAI / 千问 多引擎 | ✅ 框架完成 |
| 🎭 Live2D 动画 | 表情/动作/口型同步，WebSocket 驱动 | ✅ 接口就绪 |
| 🔊 语音合成 | GPT-SoVITS 情感语音，3 种预设 | ✅ 框架完成 |
| 🛡️ 内容审核 | DeepSeek API + 敏感词规则双重审核 | ✅ 已集成 |
| 🌍 翻译服务 | 百度翻译 + DeepSeek 自动故障切换 | ✅ 已集成 |
| 🔐 认证系统 | JWT 认证 + bcrypt 密码 + 会话管理 | ✅ 已完成 |
| 📡 WebSocket | 双向实时通信，心跳保活 | ✅ 已完成 |
| 🤖 OpenClaw | AI Agent 编排框架网关 | ✅ 框架完成 |
| 📊 监控 | Prometheus + Grafana | ✅ 已配置 |

---

## 技术栈

| 层 | 技术 |
|---|------|
| **前端** | Vue 3 + TypeScript + Vite + Element Plus + Pinia |
| **后端** | C++20 / Crow Framework / nlohmann-json / yaml-cpp |
| **数据库** | PostgreSQL 15 + Redis 7 |
| **AI 对话** | DeepSeek / OpenAI / 千问 / Ollama |
| **语音合成** | GPT-SoVITS (HTTP API) |
| **内容审核** | DeepSeek Moderation API + 规则引擎 |
| **翻译** | 百度翻译 API + DeepSeek 备选 |
| **实时通信** | WebSocket (Crow 内置) |
| **动画** | Live2D Cubism SDK for Web |
| **部署** | Docker Compose / Nginx / systemd |
| **监控** | Prometheus + Grafana |

---

## 快速开始

### Docker Compose（推荐）

```bash
# 1. 克隆
git clone https://github.com/ermaotie6/yachiyoooooooo.git
cd yachiyoooooooo

# 2. 配置
cp backend/.env.example .env
# 编辑 .env 填入 API Key 和密钥
# 编辑 config/config.yaml 填入 DeepSeek / 百度翻译 API Key

# 3. 启动
docker compose up -d

# 4. 访问
# 前端:  http://localhost:3000
# API:   http://localhost:8080/api/v1/health
# Nginx: http://localhost (代理全部服务)
```

### 裸机开发

```bash
# 后端
cd backend && mkdir build && cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20
ninja
./yachiyo_cpp --config-dir ../config --env dev

# 前端
cd frontend
npm install
npm run dev    # http://localhost:5173
```

---

## 项目结构

```
├── backend/                 # C++20 后端
│   ├── src/
│   │   ├── main.cpp
│   │   ├── Application.cpp
│   │   ├── controllers/     # HTTP 路由控制器
│   │   │   ├── AuthController.cpp
│   │   │   ├── AIController.cpp
│   │   │   ├── MessageController.cpp
│   │   │   └── ...
│   │   ├── services/        # 业务逻辑
│   │   │   ├── AuthServiceImpl.cpp
│   │   │   ├── MessageServiceImpl.cpp
│   │   │   ├── DeepSeekModerationService.cpp
│   │   │   ├── TranslationService.cpp
│   │   │   ├── GPTSoVITSService.cpp
│   │   │   ├── Live2DAnimationService.cpp
│   │   │   ├── OpenClawGateway.cpp
│   │   │   └── ...
│   │   └── utils/
│   ├── include/             # 头文件
│   ├── config/              # 后端配置
│   └── Dockerfile
│
├── frontend/                # Vue 3 前端
│   ├── src/
│   │   ├── views/           # 页面
│   │   ├── components/      # Live2D、Auth 等组件
│   │   └── composables/     # useWebSocket、useAudioPlayer
│   └── Dockerfile
│
├── config/                  # 全局配置
│   └── config.yaml          # 主配置文件（API Key、数据库、Redis 等）
│
├── database/
│   └── init.sql             # 数据库初始化脚本
│
├── nginx.conf               # Nginx 反向代理配置
├── docker-compose.yml       # Docker 编排（全部服务）
├── scripts/
│   └── deploy-server.sh     # 一键部署脚本（Arch/Debian/Ubuntu）
│
└── docs/                    # 文档
    ├── 部署指南.md           # 完整部署指南
    ├── 待实现功能清单.md      # 功能状态追踪
    ├── GPT-SoVITS部署指南.md  # 语音合成部署
    ├── OpenClaw对接说明.md    # AI 编排对接
    ├── Linux部署与Cloudflare配置指南.md
    └── archive/             # 历史文档归档
```

---

## 核心架构

```
用户 ──→ Cloudflare CDN ──→ Nginx (80/443)
                               │
                    ┌──────────┼──────────┐
                    ↓          ↓          ↓
               Frontend    Backend    WebSocket
              (Vue 3)    (Crow C++)   (:9001)
               :3000       :8080
                               │
                    ┌──────────┼──────────┐
                    ↓          ↓          ↓
               PostgreSQL    Redis     外部 API
                :5432       :6379    (DeepSeek/百度/
                                     GPT-SoVITS/
                                     OpenClaw)
```

### 消息处理流水线

```
用户发送消息
  → 6层安全审核 (敏感词 → IP黑名单 → 频率限制 → 内容审核 → 上下文检查 → 情感验证)
  → AI 生成回复 (DeepSeek / OpenAI / 千问)
  → 翻译 (百度翻译 / DeepSeek)
  → 语音合成 (GPT-SoVITS)
  → WebSocket 推送 → 前端渲染 (文字 + Live2D 动画 + 音频播放)
```

---

## 配置说明

所有敏感配置支持 `${ENV_VAR}` 环境变量语法。

**必须配置**:
- `config/config.yaml` → DeepSeek API Key、百度翻译 APP ID/Key
- `.env` → JWT_SECRET_KEY、数据库密码

**详细配置文档**: [docs/部署指南.md](./docs/部署指南.md)

---

## Docker 服务一览

| 服务 | 镜像 | 端口 | 说明 |
|------|------|------|------|
| postgres | postgres:15-alpine | 5432 | 数据库 |
| redis | redis:7-alpine | 6379 | 缓存 |
| backend | 自构建 (ubuntu:22.04) | 8080, 9001 | 后端 API + WebSocket |
| frontend | 自构建 (node:18-alpine) | 3000 | 前端 |
| nginx | nginx:alpine | 80, 443 | 反向代理 |
| prometheus | prom/prometheus | 9091 | 监控（可选） |
| grafana | grafana/grafana | 3001 | 仪表盘（可选） |
| pgadmin | dpage/pgadmin4 | 5050 | DB 管理（可选） |

---

## 文档

| 文档 | 说明 |
|------|------|
| [部署指南](./docs/部署指南.md) | 完整部署流程、Cloudflare SSL、安全加固 |
| [待实现功能清单](./docs/待实现功能清单.md) | 功能完成状态 & 待办事项 |
| [GPT-SoVITS 部署指南](./docs/GPT-SoVITS部署指南.md) | 语音合成服务安装与训练 |
| [OpenClaw 对接说明](./docs/OpenClaw对接说明.md) | AI Agent 编排框架集成 |
| [Cloudflare 配置指南](./docs/Linux部署与Cloudflare配置指南.md) | Linux 部署 + DNS + SSL 详细配置 |

---

## License

MIT
