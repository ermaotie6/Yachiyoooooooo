# Yachiyo 后端

C++20 + Crow 框架构建的 AI 虚拟形象直播平台后端，提供 REST API 和 WebSocket 实时通信。

---

## 技术栈

| 技术 | 用途 |
| --- | --- |
| C++20 / Crow | HTTP + WebSocket 框架 |
| libpqxx | PostgreSQL 驱动 |
| hiredis | Redis 驱动 |
| libcurl | 外部 API 调用 (翻译/TTS/审核) |
| OpenSSL | JWT 签名 + SHA-256 密码哈希 |
| nlohmann/json | JSON 序列化 |
| yaml-cpp | YAML 配置解析 (支持 `${ENV:default}` 语法) |
| spdlog | 异步日志 |
| Google Test | 单元测试 + 集成测试 |
| CMake 3.20+ | 构建系统 |

---

## 快速开始

### 系统依赖

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake ninja gcc git \
    openssl yaml-cpp postgresql-libs hiredis curl \
    nlohmann-json
```

**Debian / Ubuntu:**
```bash
sudo apt-get install -y build-essential cmake ninja-build gcc g++ git \
    libssl-dev libyaml-cpp-dev libpq-dev libhiredis-dev libcurl4-openssl-dev \
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
# 确保 PostgreSQL 和 Redis 已启动
./yachiyo_cpp --config-dir ../config --env dev --port 8080

# 验证
curl http://localhost:8080/api/v1/health
```

### 测试

```bash
cd build
ctest --output-on-failure
```

---

## 项目结构

```
backend/
├── CMakeLists.txt
├── Dockerfile                     # 多阶段 Docker 构建
├── config/
│   ├── config.yaml                # Docker 部署配置 (环境变量注入)
│   ├── init_database.sql          # 数据库初始化
│   └── prometheus.yml             # Prometheus 采集配置
├── include/                       # 头文件
│   ├── Application.hpp            # 应用主类 (单例)
│   ├── config/
│   │   └── ConfigManager.hpp      # YAML 配置管理 (支持数组索引)
│   ├── controllers/               # 路由控制器
│   ├── dto/                       # 数据传输对象
│   ├── models/                    # 数据模型 (User, Message)
│   ├── services/                  # 业务服务
│   │   ├── AuthServiceImpl.hpp    # JWT 认证
│   │   ├── MessageServiceImpl.hpp # 消息 + 6 层审核
│   │   ├── AvatarResponseService.hpp  # 核心编排 (7 步管线) ⭐
│   │   ├── OpenClawGateway.hpp    # OpenClaw 网关
│   │   ├── TranslationService.hpp # 百度翻译 + DeepSeek 备选
│   │   ├── GPTSoVITSService.hpp   # TTS 语音合成
│   │   ├── Live2DAnimationService.hpp # 动画生成
│   │   ├── DeepSeekModerationService.hpp # AI 内容审核
│   │   ├── DatabaseService.hpp    # PostgreSQL DAO
│   │   └── WebSocketService.hpp   # WebSocket 广播
│   └── utils/                     # 工具类 (JWT/Hash/Log/Redis)
├── src/                           # 源文件实现
│   ├── main.cpp                   # 入口
│   └── Application.cpp            # 生命周期管理
└── tests/
    ├── unit/                      # 13 个单元测试
    └── integration/               # 4 个集成测试
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

| 变量 | 说明 | 必填 |
| --- | --- | --- |
| `JWT_SECRET_KEY` | JWT 签名密钥 (≥32字符) | ✅ |
| `DB_PASSWORD` | PostgreSQL 密码 | ✅ |
| `DB_HOST` | PostgreSQL 地址 | 选填 (默认 `localhost`) |
| `REDIS_HOST` | Redis 地址 | 选填 (默认 `localhost`) |
| `DEEPSEEK_API_KEY` | DeepSeek API Key (审核+翻译备选) | 选填 |
| `BAIDU_TRANSLATE_APPID` | 百度翻译 APPID | 选填 |
| `BAIDU_TRANSLATE_API_KEY` | 百度翻译密钥 | 选填 |

配置文件支持 `${ENV_VAR:default}` 语法从环境变量读取。

---

## API 概览

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| POST | `/api/v1/auth/register` | 用户注册 |
| POST | `/api/v1/auth/login` | 登录 |
| POST | `/api/v1/auth/refresh` | 刷新 Token |
| POST | `/api/v1/auth/logout` | 注销 |
| GET | `/api/v1/auth/me` | 当前用户信息 |
| GET | `/api/v1/users` | 用户列表 |
| GET | `/api/v1/users/:id` | 用户详情 |
| PUT | `/api/v1/users/:id` | 更新用户 |
| GET | `/api/v1/messages` | 消息列表 |
| POST | `/api/v2/ai/chat` | AI 对话 |
| POST | `/api/v2/ai/tts` | TTS 合成 |
| GET | `/api/v2/ai/models` | 可用模型 |
| GET | `/api/v1/health` | 健康检查 |
| WS | `ws://:9001` | WebSocket 实时通信 |

完整 API 文档见 [API 参考](../docs/API参考.md)。

---

## Docker

```bash
# 使用项目根目录 docker-compose（推荐）
cd .. && docker compose up -d backend

# 或独立构建
docker build -t yachiyo-backend .
docker run -d -p 8080:8080 -p 9001:9001 \
  -e JWT_SECRET_KEY=your-secret \
  -e DB_HOST=host.docker.internal \
  -e DB_PASSWORD=postgres \
  yachiyo-backend
```

---

## 相关文档

- [项目 README](../README.md)
- [架构设计](../docs/架构设计.md) — 启动流程、6 层审核、AvatarResponseService 管线
- [API 参考](../docs/API参考.md) — 完整 API 端点 + 数据库表结构
- [开发指南](../docs/开发指南.md) — 编译、调试、添加 Controller/Service
- [部署指南](../docs/部署指南.md) — Docker / 裸机部署
