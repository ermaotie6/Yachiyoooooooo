# 🚀 Yachiyo 虚拟形象系统 - 快速启动指南

## 📁 文件清单

### 已创建的所有文件

#### 头文件 (include/)
```
include/
├── dto/
│   └── AvatarResponse.hpp          ✅ 虚拟形象响应 DTO
├── models/
│   ├── AnimationKeyframe.hpp       ✅ 动画关键帧
│   ├── OpenClawIntegration.hpp     ✅ OpenClaw 数据模型
│   └── VoiceConfig.hpp             ✅ 语音配置
├── services/
│   ├── AvatarMessageQueueService.hpp       ✅ 消息队列
│   ├── AvatarResponseService.hpp           ✅ 响应服务
│   ├── OpenClawIntegration.hpp             ✅ OpenClaw 服务
│   ├── TranslationService.hpp              ✅ 翻译服务
│   ├── GPTSoVITSService.hpp                ✅ 语音合成
│   └── Live2DAnimationService.hpp          ✅ 动作驱动
└── controllers/
    └── AvatarInteractionController.hpp     ✅ 控制器
```

#### 源文件 (src/)
```
src/
├── services/
│   ├── AvatarMessageQueueService.cpp       ✅ 400 行
│   ├── AvatarResponseService.cpp           ✅ 200 行
│   ├── OpenClawIntegration.cpp             ✅ 300 行
│   ├── TranslationService.cpp              ✅ 350 行
│   ├── GPTSoVITSService.cpp                ✅ 400 行
│   └── Live2DAnimationService.cpp          ✅ 500 行
└── controllers/
    └── AvatarInteractionController.cpp     ✅ 300 行
```

#### 配置文件
```
config/
└── avatar_config.yaml              ✅ 400 行配置

avatar_database_migration.sql       ✅ 800 行数据库脚本

avatar_module_CMakeLists.txt        ✅ 70 行构建配置
```

#### 文档
```
AVATAR_DELIVERY_SUMMARY.md          ✅ 项目交付总结
IMPLEMENTATION_SUMMARY.md           ✅ 实现完成总结
QUICK_START_GUIDE.md                ✅ 本文件
```

---

## ⚡ 5 分钟快速启动

### Step 1: 环境检查
```bash
# 检查 C++ 版本
g++ --version          # 需要 C++20 以上

# 检查依赖库
apt list --installed | grep -E "curl|spdlog|nlohmann"

# 检查数据库
psql --version         # PostgreSQL
redis-cli --version    # Redis
```

### Step 2: 安装依赖
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    libspdlog-dev \
    nlohmann-json3-dev \
    libpq-dev \
    libhiredis-dev

# macOS
brew install cmake curl spdlog nlohmann-json redis postgresql
```

### Step 3: 编译源代码
```bash
# 创建构建目录
mkdir build
cd build

# 运行 CMake
cmake ..

# 编译 (使用 4 个并行进程)
make -j4

# 可选: 运行测试
make test

# 可选: 安装到系统
sudo make install
```

### Step 4: 配置数据库
```bash
# 创建 PostgreSQL 数据库
createdb yachiyo_db

# 初始化数据库表
psql yachiyo_db < avatar_database_migration.sql

# 验证表已创建
psql yachiyo_db -c "\dt"
```

### Step 5: 启动服务
```bash
# 设置环境变量
export YACHIYO_CONFIG=./config/avatar_config.yaml
export YACHIYO_LOG_LEVEL=INFO

# 启动后台服务
./yachiyo_server &

# 查看实时日志
tail -f logs/yachiyo.log
```

### Step 6: 测试系统
```bash
# 检查服务健康状态
curl http://localhost:8000/api/avatar/stats/queue

# 提交消息
curl -X POST http://localhost:8000/api/avatar/messages \
  -H "Content-Type: application/json" \
  -d '{"user_id": 1, "content": "你好啊，八千代！"}'

# 响应应该是:
# {"success": true, "message_id": 1712000000000, "status": "PENDING", "created_at": 1712000000}
```

---

## 📊 核心功能演示

### 1. 消息提交流程
```
用户发送消息
  ↓
POST /api/avatar/messages
  ↓
消息验证
  ↓
入队 AvatarMessageQueueService
  ↓
返回 202 (消息已接受)
  ↓
后台异步处理
```

### 2. 虚拟形象处理流程
```
从队列获取消息
  ↓
调用 OpenClaw API → 获取回应文本 + 情感
  ↓
翻译服务 → 中文转日语
  ↓
语音合成 → 日语文本→音频
  ↓
动作驱动 → 生成 Live2D 参数
  ↓
WebSocket 推送给客户端
  ↓
保存到数据库
```

### 3. API 端点快速参考
```bash
# 提交消息
POST /api/avatar/messages
  body: {"user_id": 1, "content": "你好"}

# 查询消息状态
GET /api/avatar/messages/{id}/status

# 获取响应
GET /api/avatar/responses/{id}

# 获取聊天历史
GET /api/avatar/chat-history?user_id=1&limit=20

# 获取队列统计
GET /api/avatar/stats/queue

# WebSocket 连接
WS ws://localhost:8001/ws/avatar
```

---

## 🔧 故障排查

### 问题 1: 编译失败 - 找不到 crow
**解决方案**: Crow 是 header-only 库
```bash
# 确保 crow_all.h 在 include 路径中
cp crow_all.h /usr/local/include/
```

### 问题 2: 编译失败 - 找不到 spdlog
**解决方案**:
```bash
# 安装 spdlog
git clone https://github.com/gabime/spdlog.git
cd spdlog && mkdir build && cd build
cmake .. && make -j4 && sudo make install
```

### 问题 3: 运行时错误 - 数据库连接失败
**解决方案**:
```bash
# 检查 PostgreSQL 是否运行
sudo systemctl status postgresql

# 检查连接参数
psql -h localhost -U postgres -d yachiyo_db
```

### 问题 4: WebSocket 连接失败
**解决方案**:
```bash
# 检查防火墙
sudo ufw allow 8001/tcp

# 检查端口是否开放
netstat -tlnp | grep 8001
```

### 问题 5: OpenClaw API 调用超时
**解决方案**:
```yaml
# 修改 avatar_config.yaml
openclaw:
  timeout_ms: 10000  # 增加超时时间至 10 秒
  retry:
    max_attempts: 5
    backoff_ms: 2000
```

---

## 📈 性能调优

### 1. 增加队列大小
```yaml
message_queue:
  max_size: 5000  # 从 1000 增加至 5000
  worker_threads: 8  # 从 4 增加至 8
```

### 2. 优化缓存
```yaml
redis:
  pool_size: 20  # 从 10 增加至 20
  timeout_seconds: 10  # 从 5 增加至 10

openclaw:
  cache:
    max_size: 5000  # 从 1000 增加至 5000
```

### 3. 增加翻译缓存
```yaml
translation:
  cache:
    max_size: 10000  # 从 5000 增加至 10000
    ttl_hours: 240   # 从 168 增加至 240
```

### 4. 增加音频缓存
```yaml
gpt_sovits:
  cache:
    max_size_gb: 50  # 从 10 增加至 50
    ttl_hours: 1440  # 2 个月
```

---

## 🧪 测试命令

### 单消息测试
```bash
curl -X POST http://localhost:8000/api/avatar/messages \
  -H "Content-Type: application/json" \
  -d '{
    "user_id": 1001,
    "content": "八千代，你好吗？今天天气真好！"
  }'
```

### 批量消息测试 (Python 脚本)
```python
import requests
import json
import time

url = "http://localhost:8000/api/avatar/messages"
headers = {"Content-Type": "application/json"}

messages = [
    "你好啊，八千代！",
    "今天天气怎么样？",
    "你喜欢唱歌吗？",
    "我们可以一起聊天吗？",
    "再见，拜拜！"
]

for i, msg in enumerate(messages):
    payload = {
        "user_id": 1001,
        "content": msg
    }
    response = requests.post(url, json=payload, headers=headers)
    print(f"{i+1}. {msg}")
    print(f"   Response: {response.json()}")
    time.sleep(1)  # 间隔 1 秒

# 获取队列统计
stats_url = "http://localhost:8000/api/avatar/stats/queue"
stats = requests.get(stats_url).json()
print(f"\nQueue Stats: {json.dumps(stats, indent=2)}")
```

### 性能压力测试
```bash
# 安装 Apache Bench
sudo apt-get install apache2-utils

# 发送 1000 个并发请求
ab -n 1000 -c 50 -p message.json \
  -T application/json \
  http://localhost:8000/api/avatar/messages
```

---

## 📚 文件位置说明

### 相对路径
```
Yachiyo/
├── include/              ← 头文件
├── src/                  ← 源文件
├── config/               ← 配置文件
├── build/                ← CMake 构建输出
├── logs/                 ← 运行日志
├── storage/              ← 文件存储 (音频等)
└── avatar_*.md           ← 各种文档
```

### 重要路径
```
源文件:           src/services/Avatar*.cpp
头文件:           include/services/Avatar*.hpp
配置:             config/avatar_config.yaml
数据库脚本:       avatar_database_migration.sql
构建配置:         avatar_module_CMakeLists.txt
```

---

## 🚀 生产部署清单

- [ ] 编译无误
- [ ] 所有单元测试通过
- [ ] 集成测试完成
- [ ] 性能测试达标
- [ ] 数据库迁移完成
- [ ] 配置文件已调整
- [ ] 日志级别设置为 INFO
- [ ] 监控告警已配置
- [ ] 备份策略已确定
- [ ] 灾难恢复计划已制定
- [ ] 用户文档已准备
- [ ] 技术支持人员已培训

---

## 📞 获取帮助

### 查看日志
```bash
tail -f logs/yachiyo.log          # 实时日志
tail -f logs/yachiyo.log | grep ERROR  # 只看错误
```

### 调试模式
```yaml
# 编辑 avatar_config.yaml
system:
  debug: true
  
logging:
  level: DEBUG   # 从 INFO 改为 DEBUG
```

### 性能分析
```bash
# 查看内存使用
ps aux | grep yachiyo_server

# 查看 CPU 使用
top -p $(pgrep -f yachiyo_server)

# 查看打开的文件描述符
lsof -p $(pgrep -f yachiyo_server) | wc -l
```

---

## ✅ 成功标志

看到以下日志表示系统运行正常:

```
[2026-04-02 10:00:00] [info] Avatar interaction routes initialized
[2026-04-02 10:00:00] [info] HTTP server started on 0.0.0.0:8000
[2026-04-02 10:00:00] [info] WebSocket server started on 0.0.0.0:8001
[2026-04-02 10:00:01] [info] Worker thread started
[2026-04-02 10:00:02] [info] OpenClaw health check: healthy
[2026-04-02 10:00:02] [info] Live2D animation service initialized with 4 expressions and 7 parameters
```

---

## 🎯 常用命令速查

```bash
# 启动服务
./yachiyo_server --config config/avatar_config.yaml

# 后台运行
nohup ./yachiyo_server > server.log 2>&1 &

# 停止服务
pkill -f yachiyo_server

# 查看服务状态
curl http://localhost:8000/api/avatar/stats/queue

# 查看日志最后 100 行
tail -100 logs/yachiyo.log

# 清空日志
> logs/yachiyo.log

# 数据库备份
pg_dump yachiyo_db > backup.sql

# 数据库恢复
psql yachiyo_db < backup.sql

# 重新初始化数据库
dropdb yachiyo_db
createdb yachiyo_db
psql yachiyo_db < avatar_database_migration.sql
```

---

## 🎉 完成!

所有源代码已准备就绪!现在你可以:

1. ✅ **编译**: `make -j4`
2. ✅ **测试**: `curl http://localhost:8000/api/avatar/stats/queue`
3. ✅ **部署**: 在生产环境运行
4. ✅ **扩展**: 添加更多功能

**祝贺!Yachiyo 虚拟形象系统已准备好启动!** 🌟

---

**快速启动指南** | 版本 1.0 | 2026年4月2日
