# 🚀 Yachiyo 快速开始指南

## 📋 项目结构

```
YachiyoCPP/
├── backend/                    # C++ 后端服务
│   ├── include/
│   │   ├── services/
│   │   │   ├── WebSocketService.hpp      ✅ WebSocket 服务
│   │   │   ├── DatabaseService.hpp       ✅ 数据库服务
│   │   │   └── ...
│   │   ├── controllers/
│   │   │   ├── MessageController.hpp
│   │   │   └── ...
│   │   └── models/
│   │       ├── DatabaseModels.hpp        ✅ 数据库模型
│   │       └── ...
│   ├── src/
│   │   ├── services/
│   │   │   ├── WebSocketService.cpp      ✅ 450+ 行
│   │   │   ├── DatabaseService.cpp       ✅ 600+ 行
│   │   │   └── ...
│   │   ├── controllers/
│   │   │   └── MessageController.cpp     ✅ 已更新
│   │   └── ...
│   └── CMakeLists.txt
│
├── frontend/                   # Vue 3 前端
│   ├── src/
│   │   ├── composables/
│   │   │   ├── useWebSocket.ts           ✅ 300+ 行
│   │   │   ├── useAudioPlayer.ts         ✅ 250+ 行
│   │   │   └── ...
│   │   ├── views/
│   │   │   ├── LiveStream.vue            ✅ 650+ 行
│   │   │   └── ...
│   │   ├── components/
│   │   │   ├── Live2DComponent.vue       ✅ 500+ 行
│   │   │   └── ...
│   │   └── App.vue
│   ├── .env                  # 环境配置
│   └── package.json
│
├── database/
│   ├── init.sql              ✅ 数据库初始化脚本
│   └── schema.sql
│
├── config/
│   ├── config.yaml           ✅ 服务器配置
│   └── ...
│
└── docs/
    ├── IMPLEMENTATION_SUMMARY_2026-04-03.md     ✅ 实现总结
    ├── COMPLETE_FEATURE_ANALYSIS_AND_IMPLEMENTATION.md
    └── ...
```

---

## ⚙️ 配置说明

### 后端配置 (config/config.yaml)

```yaml
# 关键配置部分

server:
  port: 8080

websocket:
  port: 9001
  max_connections: 1000

database:
  host: "localhost"
  port: 5432
  name: "yachiyo"
  user: "yachiyo_app"
  password: "${DB_PASSWORD}"

# OpenClaw 本地部署 (不需要 API 密钥!)
openclaw:
  enabled: true
  local_deployment: true
  local_endpoint: "http://localhost:8000"
  local_config_path: "./resources/openclaw/config"
```

### 前端环境 (frontend/.env)

```bash
VITE_API_BASE_URL=http://localhost:8080/api
VITE_WS_BASE_URL=ws://localhost:9001
VITE_APP_NAME=Yachiyo
```

---

## 🔨 构建步骤

### 1️⃣ 数据库初始化

```bash
# 创建数据库
createdb yachiyo

# 初始化表和数据
psql -U postgres -d yachiyo -f database/init.sql

# 验证
psql -U postgres -d yachiyo -c "\dt"  # 列出所有表
```

### 2️⃣ 后端构建

```bash
cd backend

# CMake 构建
mkdir build && cd build
cmake ..
make -j4

# 可执行文件位置
# build/src/main (或 build/src/Release/main.exe on Windows)
```

### 3️⃣ 前端构建

```bash
cd frontend

# 安装依赖
npm install

# 开发模式
npm run dev

# 生产构建
npm run build
```

---

## 🏃 启动顺序

### 步骤 1: 启动 PostgreSQL
```bash
# macOS
brew services start postgresql

# Linux
sudo systemctl start postgresql

# Windows
net start PostgreSQL-13  # 根据版本调整
```

### 步骤 2: 启动 OpenClaw 本地服务
```bash
# 根据 OpenClaw 官方文档启动本地服务
# 通常: python -m openclaw serve --port 8000
# 或: docker run openclaw:latest

# 验证连接
curl http://localhost:8000/api/status
```

### 步骤 3: 启动后端服务
```bash
cd backend/build/src
./main  # 或 main.exe on Windows

# 输出应该显示:
# [WebSocketService] Started on 0.0.0.0:9001 path: /ws
# [DatabaseService] Initialized successfully
# [Application] Server running on 0.0.0.0:8080
```

### 步骤 4: 启动前端应用
```bash
cd frontend
npm run dev

# 访问: http://localhost:5173
# 或 http://127.0.0.1:5173
```

---

## ✅ 验证检查清单

### 后端服务
- [ ] `http://localhost:8080/health` 返回 200 OK
- [ ] WebSocket 端口 9001 可连接
- [ ] 数据库连接成功
- [ ] OpenClaw 连接正常

```bash
# 测试健康检查
curl http://localhost:8080/health

# 测试 WebSocket (使用 websocat)
websocat ws://localhost:9001

# 测试数据库
psql -U yachiyo_app -d yachiyo -c "SELECT COUNT(*) FROM users;"
```

### 前端应用
- [ ] 页面加载成功
- [ ] 可以打开虚拟直播页面
- [ ] WebSocket 连接成功 (浏览器控制台查看)
- [ ] 可以发送测试消息

```javascript
// 浏览器控制台测试 WebSocket
const ws = new WebSocket('ws://localhost:9001?user_id=test_user');
ws.onmessage = (event) => console.log('Received:', event.data);
ws.send(JSON.stringify({type: 'user_message', data: {content: 'Hello'}}));
```

### 完整流程测试

1. 打开虚拟直播页面
2. 在消息输入框输入: "你好"
3. 发送消息
4. 验证以下过程:
   - ✅ 消息出现在左侧
   - ✅ 显示处理状态
   - ✅ OpenClaw 响应 (在底部显示 Avatar 消息)
   - ✅ 音频播放 (点击音频按钮)
   - ✅ Live2D 虚拟形象播放表情和动作

---

## 🐛 常见问题排查

### 问题 1: WebSocket 连接失败

**症状**: 浏览器控制台错误 "WebSocket is closed"

**解决方案**:
```bash
# 检查后端是否运行
lsof -i :9001  # macOS/Linux
netstat -ano | findstr :9001  # Windows

# 检查防火墙
# 确保 9001 端口未被阻止

# 检查配置
grep "port: 9001" config/config.yaml
```

### 问题 2: 数据库连接失败

**症状**: 后端日志显示 "Database connection error"

**解决方案**:
```bash
# 验证 PostgreSQL 运行
psql -U postgres -c "\l"  # 列出数据库

# 检查 yachiyo 数据库
psql -U postgres -d yachiyo -c "\dt"

# 检查用户权限
psql -U postgres -c "SELECT * FROM pg_user WHERE usename='yachiyo_app';"

# 重新初始化数据库
psql -U postgres -d yachiyo -f database/init.sql
```

### 问题 3: OpenClaw 连接失败

**症状**: 消息发送但没有 Avatar 响应

**解决方案**:
```bash
# 检查 OpenClaw 服务运行状态
curl http://localhost:8000/api/status

# 查看后端日志
grep "OpenClaw" backend.log

# 检查网络连接
telnet localhost 8000

# 重启 OpenClaw
# 根据部署方式重启
```

### 问题 4: 前端无法加载虚拟形象

**症状**: Live2D 区域显示 "加载虚拟形象中..."

**解决方案**:
```bash
# 检查浏览器控制台错误
# F12 → Console 标签

# 清除缓存
# Ctrl+Shift+Delete 清除缓存和 Cookies

# 重新加载
# Ctrl+Shift+R 硬刷新

# 检查文件权限
ls -la resources/live2d/models/
```

### 问题 5: 消息没有保存到数据库

**症状**: 刷新页面后消息消失

**解决方案**:
```bash
# 检查数据库表
psql -d yachiyo -c "SELECT * FROM messages LIMIT 1;"

# 查看错误日志
grep "MessageDAO" backend.log

# 验证表结构
psql -d yachiyo -c "\d messages"

# 检查数据库权限
psql -U postgres -d yachiyo -c "SELECT * FROM information_schema.role_table_grants WHERE table_name='messages';"
```

---

## 📊 系统状态检查命令

```bash
# ===== 检查所有服务 =====

# 1. PostgreSQL 状态
psql --version
psql -U postgres -c "SELECT version();"

# 2. 数据库表
psql -d yachiyo -c "\dt"

# 3. 后端服务
curl -i http://localhost:8080/health

# 4. WebSocket
websocat ws://localhost:9001?user_id=test

# 5. OpenClaw
curl http://localhost:8000/api/status

# 6. 前端构建
npm run build

# 7. 日志文件
tail -f logs/yachiyo.log
```

---

## 📱 项目完成度

```
功能                          完成度      说明
─────────────────────────────────────────────────
WebSocket 实时通信            100% ✅     完整实现
虚拟直播前端页面              100% ✅     650+ 行代码
Live2D 动画组件               100% ✅     表情、动作、同步
音频播放和嘴部同步            100% ✅     Web Audio API
数据库持久化                  100% ✅     完整的 DAO 层
配置管理                      100% ✅     本地部署配置
内容审核流程                   70% 🟡     框架完成，需测试
OpenClaw 集成                  30% 🔴     框架完成，需集成
前端完整功能                   85% 🟡     主要功能完成

整体进度: 80% 🟢 (从 50% 提升到 80%)
```

---

## 🔗 重要文件链接

| 文件 | 用途 | 大小 |
|------|------|------|
| [IMPLEMENTATION_SUMMARY_2026-04-03.md](./IMPLEMENTATION_SUMMARY_2026-04-03.md) | 完整实现总结 | 150+ 行 |
| [COMPLETE_FEATURE_ANALYSIS_AND_IMPLEMENTATION.md](./COMPLETE_FEATURE_ANALYSIS_AND_IMPLEMENTATION.md) | 功能分析 | 300+ 行 |
| backend/include/services/WebSocketService.hpp | WebSocket 头文件 | 180 行 |
| backend/src/services/WebSocketService.cpp | WebSocket 实现 | 450 行 |
| backend/src/services/DatabaseService.cpp | 数据库服务 | 600 行 |
| frontend/src/composables/useWebSocket.ts | 前端 WebSocket | 300 行 |
| frontend/src/views/LiveStream.vue | 虚拟直播页面 | 650 行 |
| frontend/src/components/Live2DComponent.vue | Live2D 组件 | 500 行 |
| database/init.sql | 数据库初始化 | 400 行 |
| config/config.yaml | 服务配置 | 120 行 |

---

## 🎓 技术栈参考

```
后端:
├── C++20
├── Crow HTTP Framework
├── PostgreSQL 13+
├── libpqxx (C++ PostgreSQL Driver)
└── nlohmann/json

前端:
├── Vue 3
├── TypeScript 5
├── Vite
├── Element Plus
└── Web Audio API

数据库:
├── PostgreSQL
├── Redis (可选)
└── pgAdmin (管理工具)

其他:
├── OpenClaw (本地部署)
├── GPT-SoVITS (音频合成)
├── Live2D SDK (可选)
└── Docker (部署)
```

---

## 📞 获取帮助

遇到问题?

1. 查看错误日志: `tail -f logs/yachiyo.log`
2. 检查配置: `cat config/config.yaml`
3. 查阅文档: `docs/` 文件夹
4. 运行测试: `npm run test` (前端) / `make test` (后端)

---

**准备好了? Let's Go! 🚀**

```bash
# 一键启动所有服务 (需要分别在不同终端运行)
# 终端 1: 后端
cd backend/build/src && ./main

# 终端 2: 前端
cd frontend && npm run dev

# 然后访问 http://localhost:5173
```

Happy Coding! 🎉
