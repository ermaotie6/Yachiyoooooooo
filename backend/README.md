# Yachiyo 后端  AI 虚拟形象直播平台服务

**基于 C++20 和 Crow 框架的 AI 虚拟形象直播平台后端，为 OpenClaw 虚拟助理提供 RESTful API 和 WebSocket 实时通信支持。**

![Language](https://img.shields.io/badge/language-C%2B%2B20-red)
![Build](https://img.shields.io/badge/build-CMake%203.20+-blue)
![Platform](https://img.shields.io/badge/platform-Arch%20Linux-1793d1)

---

## 目录

- [技术栈](#技术栈)
- [项目结构](#项目结构)
- [快速开始](#快速开始)
- [API 端点](#api-端点)
- [配置说明](#配置说明)
- [Docker 部署](#docker-部署)

---

## 技术栈

| 技术 | 版本 | 用途 |
| :--- | :--- | :--- |
| C++ | 20 | 编程语言 |
| Crow | Latest | HTTP 框架 |
| PostgreSQL | 15+ | 关系型数据库 |
| libpqxx | 7+ | PostgreSQL C++ 客户端 |
| Redis | 7+ | 缓存 & 会话 |
| hiredis | Latest | Redis C 客户端 |
| libcurl | 7.5+ | HTTP 客户端 (AI/TTS/翻译 API) |
| OpenSSL | 1.1+ | JWT 签名 & 密码哈希 |
| nlohmann/json | 3.11+ | JSON 序列化 |
| yaml-cpp | 0.7+ | YAML 配置解析 |
| spdlog | 1.12+ | 日志系统 |
| libuuid | Latest | UUID 生成 |
| CMake | 3.20+ | 构建系统 |
| Docker | Latest | 容器化部署 |

---

## 项目结构

```
backend/
 CMakeLists.txt                 # 根 CMake 配置
 Dockerfile                     # Docker 镜像
 .env.example                   # 环境变量模板
 config/                        # 配置文件
    config.yaml                # 主配置 (YAML)
    init_database.sql          # 数据库初始化脚本
    prometheus.yml             # Prometheus 监控配置
 include/                       # 头文件
    Application.hpp            # 应用程序主类
    config/
       ConfigManager.hpp      # 配置管理器 (YAML)
    controllers/               # 路由控制器
       BaseController.hpp     # 控制器基类
       AuthController.hpp     # 认证控制器
       AIController.hpp       # AI 对话控制器
       HealthController.hpp   # 健康检查
       UserController.hpp     # 用户管理
       MessageController.hpp  # 消息管理
       WebSocketController.hpp# WebSocket 控制器
    dto/                       # 数据传输对象
       AuthDTO.hpp            # 认证 DTO
       CommonDTO.hpp          # 通用 DTO
       ChatRequest.hpp        # 聊天请求
       ChatMessageDTO.hpp     # 聊天消息
       UserDTO.hpp            # 用户 DTO
       OpenClawDTO.hpp        # OpenClaw 对接
       ModerationDTO.hpp      # 内容审查
       TranslationDTO.hpp     # 翻译
       TTSServiceDTO.hpp      # 语音合成
       Live2DDTO.hpp          # Live2D 动画
    models/                    # 数据模型
       BaseModel.hpp          # 模型基类
       DatabaseModels.hpp     # 数据库模型集合
       User.hpp               # 用户模型
       Message.hpp            # 消息模型
    services/                  # 业务服务
       AuthService.hpp        # 认证服务接口
       AuthServiceImpl.hpp    # 认证服务实现
       AIService.hpp          # AI 服务接口
       ChatService.hpp        # AI 聊天服务
       MessageService.hpp     # 消息服务接口
       MessageServiceImpl.hpp # 消息服务实现
       UserService.hpp        # 用户服务
       DatabaseService.hpp    # 数据库连接池
       WebSocketService.hpp   # WebSocket 服务
       AvatarResponseService.hpp  # 虚拟形象响应编排
       OpenClawGateway.hpp    # OpenClaw 网关
       DeepSeekModerationService.hpp # 内容审查
       TranslationService.hpp # 翻译服务
       GPTSoVITSService.hpp   # TTS 语音合成
       Live2DAnimationService.hpp # Live2D 动画控制
    utils/                     # 工具类
        HttpServer.hpp         # Crow HTTP 服务器封装
        DatabaseUtil.hpp       # 数据库工具
        RedisUtil.hpp          # Redis 工具
        JwtUtil.hpp            # JWT 签发/验证
        HashUtil.hpp           # 密码哈希
        JsonUtils.hpp          # JSON 工具
        ConfigParser.hpp       # 配置解析
        Logger.hpp             # 日志封装
        LogUtils.hpp           # 日志工具
        Result.hpp             # 通用 Result<T>
        ValidationUtils.hpp    # 输入验证
        OpencalwClient.hpp     # OpenClaw HTTP 客户端
        EmailService.hpp       # 邮件服务 (预留)
        Compat.hpp             # 命名空间兼容层
 src/                           # 源文件实现
    main.cpp                   # 程序入口
    Application.cpp            # 应用初始化与生命周期
    CMakeLists.txt             # 主可执行文件 CMake
    controllers/               # 控制器实现
    services/                  # 服务实现
    dto/                       # DTO 实现
    utils/                     # 工具实现
 scripts/
    build.sh                   # 构建脚本
 test/                          # 单元测试 (预留)
```

---

## 快速开始

### 环境要求

- Arch Linux (推荐) / Ubuntu / Debian
- CMake 3.20+, GCC 12+ 或 Clang 15+ (C++20)
- PostgreSQL 15+, Redis 7+

### 安装依赖 (Arch Linux)

```bash
sudo pacman -S postgresql redis hiredis openssl cmake crow spdlog yaml-cpp \
    nlohmann-json libpqxx curl util-linux-libs
```

### 编译

```bash
cd backend

# 使用构建脚本
bash scripts/build.sh --build

# 或手动构建
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 运行

```bash
# 确保 PostgreSQL 和 Redis 已启动
sudo systemctl start postgresql redis

# 初始化数据库
psql -U postgres -d yachiyo -f config/init_database.sql

# 启动
./build/yachiyo_cpp --config-dir config --env dev --port 8080

# 验证
curl http://localhost:8080/api/v1/health
```

---

## API 端点

### 认证

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/v1/auth/register` | 用户注册 |  |
| POST | `/api/v1/auth/login` | 用户登录 |  |
| POST | `/api/v1/auth/verify` | Token 验证 |  |
| POST | `/api/v1/auth/refresh` | 刷新 Token |  |
| POST | `/api/v1/auth/logout` | 用户登出 |  |

### 用户

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/v1/users` | 用户列表 |  |
| GET | `/api/v1/users/:id` | 用户详情 |  |

### AI 对话

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/v2/ai/chat` | AI 对话 |  |

### 消息

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/v1/messages/send` | 发送消息 (含审查) |  |
| GET | `/api/v1/messages` | 消息列表 |  |
| GET | `/api/v1/messages/:id` | 消息详情 |  |

### WebSocket

| 协议 | 地址 | 说明 |
|------|------|------|
| WS | `ws://host:9001` | 实时弹幕/AI 回复推送 |

### 健康检查

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/health` | 服务状态检查 |

---

## 配置说明

### 配置文件 (`config/config.yaml`)

```yaml
server:
  host: "0.0.0.0"
  port: 8080
  workers: 4
  cors:
    enabled: true
    origin: "*"

database:
  host: "localhost"
  port: 5432
  name: "yachiyo"
  user: "postgres"
  password: ""
  poolSize: 10

websocket:
  host: "0.0.0.0"
  port: 9001

redis:
  enabled: true
  host: "localhost"
  port: 6379

jwt:
  secret: "${JWT_SECRET_KEY}"
  expiresIn: 86400

ai:
  provider: "deepseek"
  model: "deepseek-chat"

openclaw:
  bridge_endpoint: "http://localhost:8765"
```

### 关键环境变量

| 变量 | 说明 | 必填 |
|------|------|------|
| `JWT_SECRET_KEY` | JWT 签名密钥 |  |
| `DEEPSEEK_API_KEY` | DeepSeek API Key |  |
| `BAIDU_TRANSLATE_APPID` | 百度翻译 APPID |  |
| `BAIDU_TRANSLATE_API_KEY` | 百度翻译密钥 |  |
| `DATABASE_HOST` | PostgreSQL 地址 |  |
| `REDIS_URL` | Redis 连接地址 |  |

---

## Docker 部署

```bash
# 使用项目根目录 docker-compose
cd ..
docker-compose up -d backend

# 或独立构建
docker build -t yachiyo-backend .
docker run -d -p 8080:8080 -p 9001:9001 \
  -e JWT_SECRET_KEY=your-secret \
  yachiyo-backend
```

---

## 相关文档

- [项目 README](../README.md)  全局架构与部署
- [前端 README](../frontend/README.md)  Vue 3 前端
- [GPT-SoVITS 部署指南](../docs/GPT-SoVITS部署指南.md)
- [OpenClaw 对接说明](../docs/OpenClaw对接说明.md)
- [待实现功能清单](../docs/待实现功能清单.md)
