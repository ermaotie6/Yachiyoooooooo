# Yachiyo 后端

C++20 + Crow 框架构建的 AI 虚拟形象直播平台后端，提供 REST API 和 WebSocket 实时通信。

---

## 架构概述

管线流程：
```
用户弹幕 → WebSocketMessageHandler (认证+审查) 
         → AvatarResponseService
              ├── OpenClaw (AI对话 + 内容审查 + 日语翻译，一站式)
              ├── GPT-SoVITS (语音合成，可选)
              └── Live2D 动画 (表情+动作)
         → WebSocket 推送
```

外部依赖只有两个：**OpenClaw Gateway**（必选）和 **GPT-SoVITS**（可选）。

---

## 技术栈

| 技术 | 用途 |
| --- | --- |
| C++20 / Crow | HTTP + WebSocket 框架 |
| libpqxx | PostgreSQL 驱动 |
| hiredis | Redis 驱动 |
| libcurl | 外部 API 调用 (OpenClaw/TTS) |
| OpenSSL | JWT 签名 + SHA-256 密码哈希 |
| nlohmann/json | JSON 序列化 |
| yaml-cpp | YAML 配置解析 |
| spdlog | 异步日志 |
| Google Test | 测试 |
| CMake 3.20+ | 构建系统 |

---

## 快速开始

### 系统依赖

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake gcc git openssl yaml-cpp \
    postgresql-libs hiredis curl nlohmann-json
```

**Ubuntu:**
```bash
sudo apt install build-essential cmake g++ git libssl-dev \
    libyaml-cpp-dev libpq-dev libhiredis-dev libcurl4-openssl-dev \
    nlohmann-json3-dev pkg-config
```

### 编译

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=20
make -j$(nproc)
```

### 运行

```bash
./yachiyo_cpp --config-dir ../config --env dev --port 8080
curl http://localhost:8080/api/v1/health
```

---

## 项目结构

```
backend/
├── CMakeLists.txt
├── Dockerfile
├── config/
│   ├── config.yaml                  # Docker 部署配置
│   ├── yachiyo_system_prompt.txt    # OpenClaw System Prompt
│   └── prometheus.yml
├── include/
│   ├── Application.hpp              # 应用主类 (生命周期)
│   ├── config/ConfigManager.hpp     # YAML 配置管理
│   ├── controllers/                 # 路由控制器 (6 个)
│   ├── core/
│   │   └── ServiceRegistry.hpp      # 管线服务创建与装配
│   ├── dto/                         # 数据传输对象
│   ├── handlers/
│   │   └── WebSocketMessageHandler.hpp  # WS 消息处理 (认证+审查+管线)
│   ├── models/                      # 数据模型
│   ├── services/
│   │   ├── AuthServiceImpl.hpp      # JWT 认证
│   │   ├── MessageServiceImpl.hpp   # 消息 + 规则审查
│   │   ├── AvatarResponseService.hpp # 管线编排 ⭐
│   │   ├── OpenClawGateway.hpp      # AI 网关 (JSON+重试+熔断)
│   │   ├── GPTSoVITSService.hpp     # TTS 语音合成
│   │   ├── Live2DAnimationService.hpp # Live2D 动画
│   │   └── WebSocketService.hpp     # WebSocket 广播
│   └── utils/                       # JWT/Hash/Redis/HTTP/JSON/Log
├── src/                             # 源文件
│   ├── main.cpp
│   ├── Application.cpp
│   ├── core/ServiceRegistry.cpp
│   └── handlers/WebSocketMessageHandler.cpp
└── tests/                           # Google Test
```

---

## 命令行参数

| 参数 | 说明 | 默认值 |
| --- | --- | --- |
| `--config-dir` | 配置文件目录 | `config` |
| `--env` | 运行环境 | `dev` |
| `--port` | HTTP 端口 | `8080` |
| `--host` | 监听地址 | `0.0.0.0` |
| `--workers` | 工作线程数 | CPU 核心数 |

---

## 环境变量

| 变量 | 必填 | 说明 |
|------|------|------|
| `JWT_SECRET_KEY` | ✅ | JWT 签名密钥 (≥32字符) |
| `DB_PASSWORD` | ✅ | PostgreSQL 密码 |
| `OPENCLAW_GATEWAY_TOKEN` | ✅ | OpenClaw 认证令牌 |
| `OPENCLAW_ENDPOINT` | 选 | OpenClaw 地址 |
| `OPENCLAW_MODEL` | 选 | AI 模型名称 |
| `SOVITS_ENDPOINT` | 选 | GPT-SoVITS 地址 (不填则跳过语音) |

---

## API 概览

| 方法 | 路径 | 认证 | 说明 |
|------|------|------|------|
| POST | `/api/v1/auth/register` | ❌ | 用户注册 |
| POST | `/api/v1/auth/login` | ❌ | 登录 |
| POST | `/api/v1/auth/refresh` | ❌ | 刷新 Token |
| GET | `/api/v1/auth/me` | ✅ | 当前用户信息 |
| GET | `/api/v1/messages` | ✅ | 消息列表 |
| POST | `/api/v2/ai` | ✅ | AI 对话 |
| GET | `/api/v1/health` | ❌ | 健康检查 |
| WS | `ws://:9001` | ✅ | 直播间 WebSocket |
