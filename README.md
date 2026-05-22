# Yachiyo — AI 虚拟形象直播平台

AI 驱动的 Live2D 虚拟主播直播互动系统。用户通过弹幕与 AI 驱动的虚拟形象实时交互——发送弹幕后，系统完成内容审核、AI 对话、翻译（日语）、语音合成、Live2D 动作控制，最终通过 WebSocket 推送到前端。

---

## 系统架构

```
用户浏览器 ──WebSocket──→ C++ Backend (:8080/:9001)
                              │
                              ├─ OpenClaw Gateway (AI 对话 + 审查 + 翻译，一站式)
                              ├─ GPT-SoVITS (日语语音合成，可选)
                              ├─ PostgreSQL (数据持久化)
                              └─ Redis (缓存 + 速率限制)
```

**核心特点**：
- **一个 API 完成所有 AI 工作**：OpenClaw System Prompt 内置内容审查、角色对话、日语翻译，不再依赖独立的审核/翻译 API
- **GPT-SoVITS 可选**：未部署时只推文字+动画，不影响核心体验
- **6 层规则审查**：速率限制、IP 黑名单、敏感词过滤、行为分析等规则层仍在后端执行

完整架构说明见 [docs/架构设计.md](docs/架构设计.md)。

---

## 技术栈

| 层 | 技术 |
|----|------|
| 后端 | C++20, Crow Framework, libpqxx, hiredis, spdlog, nlohmann/json |
| 前端 | Vue 3 + TypeScript + Vite + Element Plus + Pinia |
| Live2D | pixi.js + pixi-live2d-display |
| AI | OpenClaw Gateway (OpenAI-compatible API) |
| TTS | GPT-SoVITS (可选) |
| DB | PostgreSQL 15 + Redis 7 |
| 部署 | Docker Compose + Nginx |

---

## 快速开始

### 前置要求

- Docker 20.10+ & Docker Compose v2
- OpenClaw Gateway (自带或部署在可访问的地址)

### 1. 克隆

```bash
git clone <repo-url>
cd yachiyoooooooo
```

### 2. 配置

```bash
cat > .env << 'EOF'
# 必填
JWT_SECRET_KEY=your-random-secret-at-least-32-chars
DB_PASSWORD=your-database-password

# OpenClaw Gateway (必填 — 核心 AI 服务)
OPENCLAW_GATEWAY_TOKEN=your-openclaw-token
OPENCLAW_ENDPOINT=http://host.docker.internal:8100
OPENCLAW_MODEL=deepseek/deepseek-v4-flash

# GPT-SoVITS (可选 — 语音合成)
SOVITS_ENDPOINT=http://localhost:5000
EOF
```

### 3. 启动

```bash
# 最小部署（不含 TTS）
docker compose up -d postgres redis backend frontend nginx

# 含 GPT-SoVITS（需要先部署 TTS 服务）
docker compose up -d
```

### 4. 验证

```bash
curl http://localhost:8080/api/v1/health
# → {"status":"ok","version":"1.0.0"}

curl http://localhost:3000
# → Vue SPA 页面
```

---

## 外部服务对接

### OpenClaw Gateway（必选）

本项目的 AI 核心——处理内容审查、角色对话、日语翻译三项职责。

**System Prompt** 位于 `backend/config/yachiyo_system_prompt.txt`，要求 OpenClaw 返回 JSON：

```json
{"text":"中文回复","emotion":"happy","action":"wave","translation":"日文翻译","moderation":"pass"}
```

### GPT-SoVITS（可选）

日语语音合成。需要预先训练好模型（参考 `docs/GPT-SoVITS部署指南.md`）。

**部署步骤**：

```bash
# 1. 安装 GPT-SoVITS
cd /opt && git clone https://github.com/RVC-Boss/GPT-SoVITS.git
cd GPT-SoVITS && python -m venv venv && source venv/bin/activate
pip install torch torchaudio --index-url https://download.pytorch.org/whl/cpu
pip install -r requirements.txt

# 2. 放置模型文件 (从训练机拷贝)
cp your-model.pth GPT-SoVITS/models/
cp your-model.ckpt GPT-SoVITS/models/
cp your-ref.wav GPT-SoVITS/reference/

# 3. 放置预训练基础模型
# 下载 s1bert25hz-*.ckpt, s2D488k.pth, s2G488k.pth
# 放入 GPT_SoVITS/pretrained_models/

# 4. 启动 API
python api.py \
  -s models/your-model.pth \
  -g models/your-model.ckpt \
  -dr reference/your-ref.wav \
  -dt "参考音频对应的文本" -dl "ja" \
  -a 0.0.0.0 -p 5000

# 5. 验证
curl http://localhost:5000/
```

未部署 GPT-SoVITS 时，系统自动跳过语音合成，只返回文字+动画。

---

## 项目结构

```
├── backend/                 # C++20 后端
│   ├── config/              # Docker 部署配置 + System Prompt
│   ├── include/             # 头文件
│   │   ├── controllers/     # 路由控制器
│   │   ├── services/        # 业务服务
│   │   ├── core/            # ServiceRegistry (管线装配)
│   │   ├── handlers/        # WebSocketMessageHandler (消息处理)
│   │   ├── dto/             # 数据传输对象
│   │   └── utils/           # 工具类
│   ├── src/                 # 源文件
│   └── tests/               # Google Test
│
├── frontend/                # Vue 3 前端
│   └── src/
│       ├── views/           # 页面 (LiveStream.vue)
│       ├── components/      # 组件 (AvatarStage, ChatPanel, Live2DComponent)
│       ├── composables/     # 组合式函数 (useWebSocket, useSubtitle 等)
│       └── stores/          # Pinia 状态管理
│
├── resources/live2d/        # Live2D 模型文件
├── database/init.sql        # PostgreSQL 初始化
├── config/                  # 本地开发配置
├── docker-compose.yml       # Docker 服务编排
├── nginx.conf               # Nginx 反向代理
└── docs/                    # 项目文档
```

---

## 环境变量

| 变量 | 必填 | 说明 |
|------|------|------|
| `JWT_SECRET_KEY` | ✅ | JWT 签名密钥 (≥32字符) |
| `DB_PASSWORD` | ✅ | PostgreSQL 密码 |
| `OPENCLAW_GATEWAY_TOKEN` | ✅ | OpenClaw 认证令牌 |
| `OPENCLAW_ENDPOINT` | 选 | OpenClaw 地址，默认 `http://host.docker.internal:8100` |
| `OPENCLAW_MODEL` | 选 | AI 模型，默认 `deepseek/deepseek-v4-flash` |
| `SOVITS_ENDPOINT` | 选 | GPT-SoVITS 地址，默认 `http://localhost:5000` |

---

## API 端点

| 端点 | 方法 | 认证 | 说明 |
|------|------|------|------|
| `/api/v1/health` | GET | ❌ | 健康检查 |
| `/api/v1/auth/register` | POST | ❌ | 用户注册 |
| `/api/v1/auth/login` | POST | ❌ | 用户登录 |
| `/api/v1/auth/refresh` | POST | ❌ | 刷新 Token |
| `/api/v1/auth/me` | GET | ✅ JWT | 获取当前用户 |
| `/api/v1/messages` | GET | ✅ JWT | 获取消息历史 |
| `/api/v2/ai` | POST | ✅ JWT | AI 文字聊天 |
| `ws://host:9001/ws` | WS | ✅ Token | 直播间 WebSocket |

---

## 文档索引

| 文档 | 说明 |
|------|------|
| [架构设计](docs/架构设计.md) | 系统架构、管线流程、WebSocket 协议、前端架构 |
| [外部服务对接](docs/外部服务对接指南.md) | OpenClaw/GPT-SoVITS 配置、环境变量一览 |
| [GPT-SoVITS 部署](docs/GPT-SoVITS部署指南.md) | 训练→部署完整流程 |
| [全链路分析](docs/全链路运行思路分析.md) | 数据流、降级策略、故障排查 |
| [Live2D 缺失分析](docs/Live2D缺失分析与修复.md) | Live2D 表情/动作文件现状与修复方案 |

---

## 许可证

MIT License
