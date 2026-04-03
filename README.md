# 🎀 Yachiyo - AI 虚拟形象直播平台

**完整的企业级 AI 虚拟形象直播平台，支持实时 Live2D 动画、WebSocket 通讯、内容审核和生产部署。**

[![Version](https://img.shields.io/badge/version-3.0.0-brightgreen)](./CHANGELOG.md)
[![License](https://img.shields.io/badge/license-MIT-blue)](./LICENSE)
[![Frontend](https://img.shields.io/badge/Frontend-Vue3%2FTS-green)](./frontend)

一个基于 C++ 和 Vue 3 的**完整的 AI 虚拟人物交互平台**，集成了 Live2D 形象、实时聊天、语音合成和 OpenClaw 自主 AI 框架。

---

## ⚡ 5 分钟快速开始

1. **配置环境**: 将 `backend/.env.example` 复制一份并重命名为 `backend/.env`
2. **启动所有服务（Docker）**: `docker-compose up -d --build`
3. **访问系统面板**: `http://localhost:3000`

---

## ✨ 核心机制解答

### 1. **Live2D 是怎么调用的？**
* **流程**：OpenClaw 动作情感 -> Backend 解析 -> WebSocket -> Live2D Canvas 绘制
* **代码位置**：`backend/src/services/Live2DAnimationService.cpp` & `frontend/src/components/Live2DComponent.vue`

### 2. **GPTSoVITS 是怎么调用的？**
* **流程**：OpenClaw 响应 -> Backend 请求 TTS -> WebSocket 音频下发 -> useAudioPlayer 播放
* **代码位置**：`backend/src/services/GPTSoVITSService.cpp` & `frontend/src/composables/useAudioPlayer.ts`

### 3. **本项目怎么与 OpenClaw 连接？**
* **流程**：基于 HTTP 协议在后端网关调用 OpenClaw API（发送文本+表情指令，获取响应并触发下游TTS和动画渲染）
* **代码位置**：`backend/src/services/OpenClawGateway.cpp`

---

## 🏗 技术栈

| 层 | 技术 |
| --- | ------ |
| **前端** | Vue 3 + TypeScript + Vite + Element Plus |
| **后端** | C++20 / Crow Framework |
| **数据库** | PostgreSQL + Redis (缓存/会话) |
| **AI 对话** | OpenClaw Gateway (HTTP) |
| **语音合成** | GPT-SoVITS (HTTP) |
| **内容审核** | DeepSeek Moderation API |
| **实时通讯** | WebSocket (双向) |
| **动画渲染** | Live2D Web Canvas |
| **部署** | Docker Compose |

## 🔧 项目结构

```text
backend/
  src/services/       # OpenClaw、GPTSoVITS、WebSocket、审核等核心服务
  src/controllers/    # HTTP 路由控制器
  include/            # 头文件
frontend/
  src/views/          # 页面 (LiveStream)
  src/components/     # Live2D、Auth 等组件
  src/composables/    # useWebSocket、useAudioPlayer 等组合函数
config/               # 配置文件
scripts/              # 构建脚本
```

## 📚 文档导航 & 归档

历史文档、阶段检查单和进度报告已归档至 `docs/archive/` 目录。
