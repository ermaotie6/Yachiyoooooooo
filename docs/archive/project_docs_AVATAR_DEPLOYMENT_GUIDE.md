# 🚀 虚拟形象交互系统 - 快速部署指南

> **项目**: Yachiyo 虚拟主播系统  
> **模块**: 虚拟形象交互完整实现  
> **创建日期**: 2026年4月2日  
> **难度**: ⭐⭐⭐⭐  
> **工期**: 2 周

---

## 📋 前置准备清单

### 系统环境

- ✅ Ubuntu 20.04+ 或 Windows 10+
- ✅ C++20 编译器 (GCC 10+ 或 Clang 12+)
- ✅ CMake 3.16+
- ✅ PostgreSQL 12+
- ✅ Redis 6.0+
- ✅ Node.js 16+ (前端)

### 软件依赖

```bash
# 后端依赖
- Crow Framework (HTTP 服务器)
- nlohmann/json (JSON 处理)
- PostgreSQL C++ 驱动
- Redis++ (Redis 客户端)
- Live2D Cubism SDK
- cURL (HTTP 请求)

# 前端依赖
- Vue 3
- Pixi.js
- WebSocket
- Axios
```

### 外部服务

- ✅ OpenClaw API (自主代理)
- ✅ 翻译 API (中文→日语)
- ✅ GPT-SoVITS (语音合成)
- ✅ 参考音源文件 (虚拟形象的"声音")

---

## 📁 项目结构创建

### 步骤 1: 创建新文件和目录

```bash
# 进入项目目录
cd /path/to/YachiyoCPP

# 创建服务类文件
mkdir -p include/services
mkdir -p src/services
mkdir -p include/models
mkdir -p src/models
mkdir -p include/controllers
mkdir -p src/controllers

# 创建配置目录
mkdir -p config
mkdir -p migrations
mkdir -p scripts

# 创建虚拟形象资源目录
mkdir -p assets/avatar
mkdir -p assets/audio
```

### 步骤 2: 添加 CMakeLists.txt 配置

**修改**: `YachiyoCPP/CMakeLists.txt`

```cmake
# 添加以下内容到 CMakeLists.txt

# Live2D 支持
if(USE_LIVE2D)
    set(LIVE2D_SDK_PATH "${CMAKE_SOURCE_DIR}/third_party/CubismSdkForNative")
    if(EXISTS ${LIVE2D_SDK_PATH})
        include_directories(${LIVE2D_SDK_PATH}/Framework/src)
        link_directories(${LIVE2D_SDK_PATH}/Framework/out/cmake/linux/x86_64/Release)
    endif()
endif()

# 虚拟形象交互系统源文件
set(AVATAR_SOURCES
    src/services/AvatarMessageQueueService.cpp
    src/services/AvatarResponseService.cpp
    src/services/TranslationService.cpp
    src/services/GPTSoVITSService.cpp
    src/services/Live2DAnimationService.cpp
    src/models/OpenClawIntegration.cpp
    src/controllers/AvatarInteractionController.cpp
)

# 添加到可执行文件
add_executable(yachiyo_server
    ${BASE_SOURCES}
    ${AVATAR_SOURCES}
)

# 链接库
target_link_libraries(yachiyo_server
    ${BASE_LIBRARIES}
    cubism
    pthread
)
```

---

## 🗄️ 数据库设置

### 步骤 1: 创建数据库

```bash
# 连接到 PostgreSQL
psql -U postgres

# 创建数据库
CREATE DATABASE yachiyo_avatar;

# 切换到新数据库
\c yachiyo_avatar

# 执行迁移脚本
\i avatar_database_migration.sql

# 验证表创建
\dt
```

### 步骤 2: 验证表结构

```bash
# 显示所有表
\dt

# 查看具体表结构
\d messages
\d avatar_responses
\d avatar_actions
\d audio_files
```

---

## ⚙️ 配置文件设置

### 创建配置文件

**新建**: `YachiyoCPP/config/avatar_config.yaml`

```yaml
# ============================================================================
# Yachiyo 虚拟形象交互系统配置
# ============================================================================

avatar:
  # 虚拟形象基本信息
  name: "八千代辉夜姬"
  id: "avatar_yachiyo_001"
  type: "live2d"
  
  # 模型路径
  model_path: "/path/to/yachiyo_live2d/八千代辉夜姬.model3.json"
  texture_path: "/path/to/yachiyo_live2d"
  physics_config_path: "/path/to/yachiyo_live2d/八千代辉夜姬.physics3.json"
  
  # 动画配置
  animation:
    fps: 60
    smoothing: 10
    default_duration_ms: 1500
  
  # 表情映射
  expressions:
    happy: "笑咪咪"
    shy: "眯眯眼"
    sad: "眼泪"
    crying: "泪珠"

# ============================================================================
# OpenClaw 配置
# ============================================================================

openclaw:
  enabled: true
  api_endpoint: "http://localhost:8789/api"
  api_key: "your_openclaw_api_key"
  timeout_ms: 5000
  max_retries: 3
  
  # 请求配置
  request:
    include_context: true
    include_user_history: true
    include_platform_info: true
  
  # 响应处理
  response:
    cache_responses: true
    cache_duration_hours: 24

# ============================================================================
# 翻译服务配置
# ============================================================================

translation:
  engine: "google_translate"  # google_translate, baidu, openai, local_model
  
  # Google Translate
  google:
    enabled: false
    api_key: "your_google_api_key"
  
  # Baidu Translate
  baidu:
    enabled: false
    app_id: "your_baidu_app_id"
    app_key: "your_baidu_app_key"
  
  # OpenAI GPT (推荐)
  openai:
    enabled: true
    api_key: "sk-..."
    model: "gpt-3.5-turbo"
    temperature: 0.7
  
  # 缓存
  cache:
    enabled: true
    redis_key_prefix: "translation:"
    ttl_hours: 168

# ============================================================================
# GPT-SoVITS 语音合成配置
# ============================================================================

voicesynthesis:
  engine: "gpt_sovits"  # gpt_sovits, tacotron2
  
  # GPT-SoVITS
  gpt_sovits:
    api_endpoint: "http://localhost:9880"
    reference_audio_path: "/path/to/yachiyo_reference_voice.wav"
    
    # 音色配置
    voice:
      scale: 1.0         # 音量: 0.5-2.0
      pitch: 0.0         # 音高: -24 到 24
      speed: 1.0         # 速度: 0.5-2.0
      emotion: "neutral" # 情感
    
    # 输出配置
    output:
      format: "wav"
      sample_rate: 44100
      channels: 1
  
  # 音频缓存
  cache:
    enabled: true
    directory: "/tmp/audio_cache"
    max_size_mb: 1000
    ttl_hours: 48

# ============================================================================
# 消息队列配置
# ============================================================================

queue:
  # 队列类型
  type: "memory"  # memory, redis, rabbitmq
  
  # 内存队列
  memory:
    max_queue_size: 1000
    cleanup_interval_seconds: 60
    timeout_seconds: 300
  
  # Redis 队列 (可选)
  redis:
    host: "localhost"
    port: 6379
    key_prefix: "avatar_queue:"
  
  # 处理配置
  processing:
    worker_threads: 4
    batch_size: 10
    batch_timeout_ms: 500

# ============================================================================
# 内容审核配置
# ============================================================================

content_review:
  # 审核层配置
  layers:
    rate_limiting:
      enabled: true
      max_per_minute: 10
      max_per_hour: 100
    
    ip_blacklist:
      enabled: true
      check_interval_seconds: 300
    
    sensitive_filter:
      enabled: true
      word_file: "config/sensitive_words.txt"
      update_interval_hours: 24
    
    heuristic_analysis:
      enabled: true
      uppercase_ratio_threshold: 0.5
      punctuation_ratio_threshold: 0.3
      block_urls: true
      block_emails: true
    
    behavior_analysis:
      enabled: true
      suspicious_pattern_detection: true
    
    manual_review:
      enabled: true
      confidence_threshold: 0.7

# ============================================================================
# 数据库配置
# ============================================================================

database:
  type: "postgresql"
  host: "localhost"
  port: 5432
  name: "yachiyo_avatar"
  user: "yachiyo_user"
  password: "secure_password"
  
  # 连接池
  pool:
    min_connections: 5
    max_connections: 20
    connection_timeout_seconds: 30
  
  # 日志
  logging:
    enabled: true
    log_queries: false
    slow_query_threshold_ms: 1000

# ============================================================================
# Redis 配置
# ============================================================================

redis:
  host: "localhost"
  port: 6379
  db: 0
  password: ""
  
  # 连接池
  pool_size: 10
  
  # 键前缀
  key_prefix: "yachiyo:"

# ============================================================================
# WebSocket 配置
# ============================================================================

websocket:
  enabled: true
  path: "/ws/avatar/interaction"
  max_connections: 1000
  heartbeat_interval_ms: 30000
  
  # 消息配置
  message:
    max_size_bytes: 1048576  # 1 MB
    compression: true

# ============================================================================
# 日志配置
# ============================================================================

logging:
  level: "info"  # debug, info, warn, error, fatal
  
  # 控制台输出
  console:
    enabled: true
    format: "[{level}] {timestamp} {message}"
  
  # 文件输出
  file:
    enabled: true
    path: "logs/avatar.log"
    max_size_mb: 100
    max_files: 10
    format: "json"
  
  # 性能指标
  metrics:
    enabled: true
    database_path: "logs/metrics.db"
    flush_interval_seconds: 60

# ============================================================================
# 性能配置
# ============================================================================

performance:
  # 渲染配置
  rendering:
    target_fps: 60
    frame_skip_enabled: true
    frame_skip_threshold_ms: 50
  
  # 缓存配置
  caching:
    enable_response_cache: true
    enable_animation_cache: true
    cache_size_mb: 512
  
  # 优化
  optimization:
    enable_lazy_loading: true
    enable_resource_pooling: true
    enable_compression: true

# ============================================================================
# 安全配置
# ============================================================================

security:
  # API 安全
  api:
    require_authentication: true
    require_https: false  # 生产环境设为 true
    enable_rate_limiting: true
    enable_cors: true
  
  # 数据加密
  encryption:
    enable_database_encryption: false
    enable_transport_encryption: true
  
  # CORS
  cors:
    allowed_origins:
      - "http://localhost:3000"
      - "http://localhost:8080"
    allowed_methods: ["GET", "POST", "PUT", "DELETE"]
    allowed_headers: ["Content-Type", "Authorization"]

# ============================================================================
```

### 创建敏感词库

**新建**: `YachiyoCPP/config/sensitive_words.txt`

```
违法
政治
色情
暴力
骂人词汇
垃圾信息
广告
钓鱼网站
恶意代码
```

---

## 🚀 编译和运行

### 编译后端

```bash
# 进入项目目录
cd YachiyoCPP

# 创建编译目录
mkdir -p build && cd build

# 编译
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 运行服务器
./yachiyo_server
```

### 启动前端开发服务器

```bash
# 进入前端目录
cd YachiyoWeb

# 安装依赖
npm install

# 开发模式运行
npm run dev

# 访问
# http://localhost:3000
```

---

## 🧪 测试和验证

### 1. 后端 API 测试

```bash
# 测试消息提交
curl -X POST http://localhost:8000/api/messages/submit \
  -H "Content-Type: application/json" \
  -d '{"content": "你好啊，八千代！", "platform": "web"}'

# 测试虚拟形象模型信息
curl http://localhost:8000/api/avatar/model-info

# 测试队列状态
curl http://localhost:8000/api/avatar/queue-stats
```

### 2. WebSocket 测试

```javascript
// 在浏览器控制台运行

const ws = new WebSocket('ws://localhost:8000/ws/avatar/interaction');

ws.onopen = () => {
    console.log('WebSocket 已连接');
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('收到虚拟形象响应:', data);
};

ws.onerror = (error) => {
    console.error('WebSocket 错误:', error);
};
```

### 3. 完整流程测试

```bash
# 1. 发送消息
curl -X POST http://localhost:8000/api/messages/submit \
  -H "Content-Type: application/json" \
  -d '{
    "content": "今天天气真好！",
    "platform": "web"
  }'

# 2. 在浏览器中打开虚拟直播页面
# http://localhost:3000/live-stream

# 3. 观察虚拟形象的反应
# - 虚拟形象应该显示
# - 文字应该出现 (中文 + 日语)
# - 音频应该播放
# - 虚拟形象应该执行相应的动作
```

---

## 📊 性能基准测试

### 目标性能指标

| 指标 | 目标值 | 测试方法 |
|------|--------|--------|
| **消息处理时间** | < 2 秒 | 从输入到虚拟形象响应 |
| **Live2D 帧率** | 60 FPS | 渲染最复杂的动画 |
| **内存占用** | < 500 MB | 长时间运行 |
| **并发连接** | 100+ | WebSocket 压力测试 |
| **API 响应时间** | < 200 ms | 不包括 OpenClaw 处理 |

### 性能测试脚本

```python
# tests/performance_test.py

import requests
import time
import json

BASE_URL = "http://localhost:8000"

def test_message_processing():
    """测试消息处理性能"""
    
    messages = [
        "你好啊，八千代！",
        "今天天气真好！",
        "你在做什么呢？",
        "哈哈，太有趣了！",
        "再见，祝你今天开心！"
    ]
    
    times = []
    
    for msg in messages:
        start = time.time()
        
        response = requests.post(
            f"{BASE_URL}/api/messages/submit",
            json={"content": msg, "platform": "web"}
        )
        
        end = time.time()
        elapsed = (end - start) * 1000
        times.append(elapsed)
        
        print(f"消息: '{msg}' -> {elapsed:.2f}ms")
    
    avg_time = sum(times) / len(times)
    max_time = max(times)
    min_time = min(times)
    
    print(f"\n性能统计:")
    print(f"  平均时间: {avg_time:.2f}ms")
    print(f"  最大时间: {max_time:.2f}ms")
    print(f"  最小时间: {min_time:.2f}ms")
    
    assert avg_time < 2000, f"平均处理时间过长: {avg_time}ms"
    print("✓ 性能测试通过")

def test_concurrent_connections():
    """测试并发连接"""
    
    import websocket
    import threading
    
    connections = []
    
    def connect():
        try:
            ws = websocket.create_connection(
                "ws://localhost:8000/ws/avatar/interaction"
            )
            connections.append(ws)
        except Exception as e:
            print(f"连接失败: {e}")
    
    # 创建 100 个并发连接
    threads = []
    for i in range(100):
        t = threading.Thread(target=connect)
        t.start()
        threads.append(t)
    
    for t in threads:
        t.join()
    
    print(f"成功连接: {len(connections)} / 100")
    
    # 关闭所有连接
    for conn in connections:
        conn.close()
    
    assert len(connections) >= 99, "连接成功率不足"
    print("✓ 并发连接测试通过")

if __name__ == "__main__":
    test_message_processing()
    test_concurrent_connections()
```

---

## 📝 部署检查清单

### 生产环境部署前

- [ ] 所有源文件已创建
- [ ] CMakeLists.txt 已配置
- [ ] 数据库表已创建
- [ ] 配置文件已更新
- [ ] 参考音源已复制
- [ ] OpenClaw 连接已测试
- [ ] 翻译 API 密钥已配置
- [ ] GPT-SoVITS 服务已启动
- [ ] Redis 服务已启动
- [ ] PostgreSQL 服务已启动
- [ ] 日志目录已创建
- [ ] 音频缓存目录已创建

### 运行时检查

- [ ] 后端服务已启动 (端口 8000)
- [ ] 前端服务已启动 (端口 3000)
- [ ] WebSocket 连接正常
- [ ] 消息队列正常运行
- [ ] 数据库连接正常
- [ ] Redis 连接正常
- [ ] OpenClaw API 可访问
- [ ] 翻译服务可用
- [ ] 语音合成服务可用

### 功能验证

- [ ] 用户可以发送消息
- [ ] 消息通过审核检查
- [ ] 虚拟形象能响应
- [ ] 文本正确显示 (中文 + 日语)
- [ ] 语音能正常播放
- [ ] 虚拟形象能正确动作
- [ ] 实时显示功能正常

---

## 🐛 常见问题排查

### 问题 1: WebSocket 连接失败

```
症状: WebSocket 连接被拒绝
原因: 端口被占用或防火墙阻止
解决:
  1. 检查端口: sudo netstat -tlnp | grep 8000
  2. 检查防火墙: sudo ufw allow 8000
  3. 重启服务: pkill yachiyo_server && ./yachiyo_server
```

### 问题 2: 数据库连接失败

```
症状: "Failed to connect to PostgreSQL"
原因: PostgreSQL 服务未运行或密码错误
解决:
  1. 启动 PostgreSQL: sudo systemctl start postgresql
  2. 检查密码: psql -U yachiyo_user -d yachiyo_avatar
  3. 更新配置文件中的密码
```

### 问题 3: Live2D 模型加载失败

```
症状: "Failed to load Live2D model"
原因: 模型路径错误或文件缺失
解决:
  1. 检查路径: ls -la /path/to/yachiyo_live2d/
  2. 更新配置文件中的模型路径
  3. 确保所有文件都已复制
```

### 问题 4: 音频生成失败

```
症状: "GPT-SoVITS synthesis failed"
原因: 服务未启动或网络问题
解决:
  1. 启动 GPT-SoVITS 服务
  2. 检查 API 端点
  3. 测试网络连接
  4. 检查参考音源文件
```

---

## 📞 获取帮助

遇到问题？查看这些资源:

1. **日志文件**: `logs/avatar.log`
2. **架构文档**: `VIRTUAL_AVATAR_INTERACTION_SYSTEM.md`
3. **实现详解**: `AVATAR_IMPLEMENTATION_DETAILS.md`
4. **Live2D 分析**: `YACHIYO_LIVE2D_ANALYSIS.md`

---

**现在你已准备好部署虚拟形象交互系统了！** 🎉

下一步: 按照步骤逐个执行，确保每个阶段都正确完成。
