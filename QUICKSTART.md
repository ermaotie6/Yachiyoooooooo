# 🎯 快速开始指南

## 项目状态概览

**当前版本**: v0.75  
**完成度**: 75% (实现阶段)  
**最后更新**: 2024年

### 项目里程碑
```
Phase 1: 需求分析 & 设计          ✅ 100%
Phase 2: 服务接口定义              ✅ 100%
Phase 3: 核心服务实现              ✅ 100% (当前完成)
Phase 4: WebSocket & 测试框架      ✅ 100%
Phase 5: 真实 API 集成            ⏳ 0%
Phase 6: 前端开发                  ⏳ 0%
Phase 7: 部署与优化                ⏳ 0%
```

---

## 🚀 快速开始

### Linux / macOS 构建

```bash
# 克隆项目（如果还没有）
cd /path/to/Yachiyo

# 使用构建脚本
chmod +x build.sh
./build.sh

# 或手动构建
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### Windows 构建

```powershell
# PowerShell (管理员模式)
cd D:\Personal_Project\Yachiyo

# 运行构建脚本
.\build.ps1

# 或自定义选项
.\build.ps1 -CleanBuild -BuildTests -RunTests
```

### Docker 构建

```bash
docker build -t yachiyo:latest .
docker-compose up -d
```

---

## 📦 核心服务说明

### 1️⃣ OpenClawGateway
**用途**: 与 OpenClaw AI 通信  
**状态**: ✅ 生产就绪  
**关键功能**:
- JSON 请求/响应处理
- 1小时 TTL 缓存
- CURL HTTP 集成

**配置**:
```yaml
openclaw:
  endpoint: "http://localhost:8888/api/v1/chat"
  timeout_ms: 30000
  cache_ttl_seconds: 3600
```

### 2️⃣ TranslationService
**用途**: 多语言翻译  
**状态**: ✅ 框架就绪 (需要 API 密钥)  
**支持的引擎**:
- DeepSeek (优先)
- OpenAI (备选)
- Google Translate (备选)

**使用示例**:
```cpp
auto result = translation_service->translate(
    "你好", 
    "zh-CN", 
    "en-US",
    TranslationService::Engine::AUTO
);
```

### 3️⃣ GPTSoVITSService
**用途**: 语音合成 (TTS)  
**状态**: ✅ 生产就绪  
**特性**:
- 5 种情感映射
- 3 种嗓音预设
- 声音克隆支持

**情感映射**:
- 开心 → 高音高, 快语速
- 伤心 → 低音高, 慢语速
- 生气 → 中偏高音高, 正常语速
- 激动 → 高音高, 快语速
- 平静 → 正常, 正常

### 4️⃣ Live2DAnimationService
**用途**: 动画序列生成  
**状态**: ✅ 生产就绪  
**能力**:
- 情感→表情 映射
- 动作→运动 映射
- 嘴部同步
- 眼睛追踪

### 5️⃣ AvatarResponseService
**用途**: 主编排层  
**状态**: ✅ 生产就绪  
**流程**:
```
用户消息 → 审核 → AI → 翻译 → TTS → 动画 → 响应
```

### 6️⃣ DeepSeekModerationService
**用途**: 内容安全审核  
**状态**: ✅ 生产就绪  
**审核类别**:
- Violence (暴力)
- Adult (成人)
- Harassment (骚扰)
- Spam (垃圾)
- Hate (仇恨)
- Profanity (粗言)

---

## 🧪 运行测试

### 编译测试

```bash
cd build_test
cmake .. -DENABLE_TESTING=ON
make
ctest --output-on-failure
```

### 运行特定测试

```bash
./build_test/backend_test --gtest_filter="AvatarResponseServiceTest*"

# 或指定具体测试
./build_test/backend_test --gtest_filter="TestUserMessageProcessing*"
```

### 可用的测试用例

```
✓ TestAvatarServiceInitialization     - 服务初始化
✓ TestUserMessageProcessing            - 消息处理
✓ TestTranslationIntegration           - 翻译集成
✓ TestContentModeration                - 内容审核
✓ TestEmotionMapping                   - 情感映射
✓ TestPerformance                      - 性能基准
```

---

## 📊 代码统计

### 新增代码行数

| 组件 | 类型 | 行数 |
|------|------|------|
| 服务头文件 | Header | 950 |
| 服务实现 | Implementation | 1,580 |
| 控制器 | Controller | 550 |
| 测试代码 | Test | 420 |
| **总计** | | **3,550** |

### 文件清单

```
backend/include/services/
├── OpenClawGateway.hpp
├── TranslationService.hpp
├── GPTSoVITSService.hpp
├── Live2DAnimationService.hpp
├── AvatarResponseService.hpp
└── DeepSeekModerationService.hpp

backend/src/services/
├── OpenClawGateway.cpp
├── TranslationService.cpp
├── GPTSoVITSService.cpp
├── Live2DAnimationService.cpp
├── AvatarResponseService.cpp
└── DeepSeekModerationService.cpp

backend/include/controllers/
└── WebSocketController.hpp

backend/src/controllers/
└── WebSocketController.cpp

backend/test/
├── AvatarResponseServiceTest.hpp
└── AvatarResponseServiceTest.cpp
```

---

## 🔧 配置管理

### 配置文件位置
```
backend/config/config.yaml
```

### 服务配置示例

```yaml
services:
  openclaw:
    endpoint: "http://localhost:8888/api/v1/chat"
    timeout_ms: 30000
    cache_ttl_seconds: 3600
    
  translation:
    engines: [deepseek, openai, google]
    cache_ttl_seconds: 7200
    
  tts:
    endpoint: "http://localhost:9999/tts"
    voice_presets: [default, playful, cool]
    cache_ttl_seconds: 3600
    
  deepseek_moderation:
    endpoint: "https://api.deepseek.com/v1/moderation"
    categories:
      - violence
      - adult
      - harassment
      - spam
      - hate
      - profanity
```

### 环境变量

```bash
# API 密钥
export OPENCLAW_API_KEY="your_key_here"
export DEEPSEEK_API_KEY="your_key_here"
export OPENAI_API_KEY="your_key_here"
export GOOGLE_TRANSLATE_KEY="your_key_here"

# 服务端点
export OPENCLAW_ENDPOINT="http://localhost:8888"
export TTS_ENDPOINT="http://localhost:9999"
export MODERATION_ENDPOINT="https://api.deepseek.com/v1"
```

---

## 🔌 集成指南

### 添加到现有项目

1. **复制头文件**
```bash
cp backend/include/services/* /path/to/your/project/include/
cp backend/include/controllers/* /path/to/your/project/include/
```

2. **复制实现文件**
```bash
cp backend/src/services/* /path/to/your/project/src/
cp backend/src/controllers/* /path/to/your/project/src/
```

3. **更新 CMakeLists.txt**
```cmake
add_library(yachiyo_services
    src/services/OpenClawGateway.cpp
    src/services/TranslationService.cpp
    src/services/GPTSoVITSService.cpp
    src/services/Live2DAnimationService.cpp
    src/services/AvatarResponseService.cpp
    src/services/DeepSeekModerationService.cpp
    src/controllers/WebSocketController.cpp
)

target_link_libraries(yachiyo_services PUBLIC
    nlohmann_json::nlohmann_json
    CURL::libcurl
)
```

4. **初始化服务**
```cpp
#include "services/AvatarResponseService.hpp"

// 创建服务
auto openclaw = std::make_shared<services::OpenClawGateway>();
auto translation = std::make_shared<services::TranslationService>();
auto tts = std::make_shared<services::GPTSoVITSService>();
auto animation = std::make_shared<services::Live2DAnimationService>();
auto moderation = std::make_shared<services::DeepSeekModerationService>();

// 创建主服务
auto avatar_service = std::make_shared<services::AvatarResponseService>(
    openclaw, translation, tts, animation, moderation
);

// 初始化
avatar_service->initialize(nullptr, "zh-CN");

// 使用
auto result = avatar_service->processUserMessage(
    "user_123",
    "你好",
    "zh-CN"
);
```

---

## 🎮 WebSocket 集成

### 服务器端

```cpp
#include "controllers/WebSocketController.hpp"

// 创建控制器
auto ws_controller = std::make_shared<WebSocketController>(avatar_service);
ws_controller->initialize();

// 处理连接
std::string sessionId = ws_controller->handleClientConnect(clientId, metadata);

// 处理消息
auto response = ws_controller->handleMessage(clientId, messageData);

// 处理断开
ws_controller->handleClientDisconnect(clientId);
```

### 客户端示例 (JavaScript)

```javascript
const ws = new WebSocket("ws://localhost:8080/ws/avatar");

ws.onopen = () => {
    // 发送用户消息
    ws.send(JSON.stringify({
        type: "USER_MESSAGE",
        payload: {
            text: "你好",
            language: "zh-CN"
        }
    }));
};

ws.onmessage = (event) => {
    const response = JSON.parse(event.data);
    console.log("Avatar 响应:", response);
    
    // 播放音频
    const audio = new Audio(response.payload.audioUrl);
    audio.play();
    
    // 渲染动画
    renderAnimations(response.payload.commands);
};
```

---

## 📝 API 文档

### 完整 API 文档位置

```
docs/
├── OpenClawIntegration.md          # OpenClaw API 规范
├── TranslationServiceAPI.md        # 翻译服务 API
├── GPTSoVITSUsage.md               # 语音合成使用指南
├── Live2DResources.md              # Live2D 资源指南
└── DeepSeekModerationAPI.md        # 审核 API 规范
```

### 快速 API 参考

**处理单条消息**
```cpp
auto result = avatar_service->processUserMessage(
    userId,              // 用户ID
    text,                // 用户消息
    targetLanguage       // 目标语言 (默认 zh-CN)
);
```

**批量处理**
```cpp
auto result = avatar_service->batchProcessMessages(
    userId,
    messages,            // std::vector<std::string>
    targetLanguage
);
```

**内容审核**
```cpp
auto result = moderation_service->moderate(text);
if (result.getValue().verdict == dto::Verdict::PASS) {
    // 内容通过审核
}
```

---

## 🐛 故障排除

### 常见问题

**Q: CMake 找不到 CURL**
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# macOS
brew install curl

# Windows
# 从 https://curl.se/download.html 下载
```

**Q: 编译失败，出现 C++20 错误**
```bash
# 确保使用 C++20 编译器
cmake .. -DCMAKE_CXX_STANDARD=20
```

**Q: 测试编译失败**
```bash
# 安装 GTest
sudo apt-get install libgtest-dev  # Ubuntu
# 或
brew install googletest             # macOS
```

**Q: 服务初始化失败**
```cpp
// 检查配置文件路径
// 检查 API 密钥和端点
// 查看日志输出详情
LOG_ERROR("服务初始化失败: {}", error_message);
```

---

## 📚 深入阅读

### 主要文档

1. **[实现报告](./IMPLEMENTATION_REPORT.md)**
   - 详细的服务实现说明
   - 代码行数统计
   - 完成度统计

2. **[完成总结](./COMPLETION_SUMMARY.md)**
   - 项目进度统计
   - 架构概览
   - 性能指标

3. **[构建报告](./BUILD_REPORT.txt)**
   - 构建统计信息
   - 代码行数
   - 构建状态

### API 规范

- [OpenClaw API 集成指南](./docs/OpenClawIntegration.md)
- [翻译服务 API 文档](./docs/TranslationServiceAPI.md)
- [GPT-SoVITS 使用指南](./docs/GPTSoVITSUsage.md)
- [Live2D 资源指南](./docs/Live2DResources.md)
- [DeepSeek 审核 API](./docs/DeepSeekModerationAPI.md)

---

## 🤝 贡献指南

### 代码风格
- C++20 标准
- 4 空格缩进
- 驼峰命名法
- 完整的错误处理

### 提交流程
1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/amazing`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing`)
5. 开启 Pull Request

---

## 📞 支持

### 获取帮助
- 📖 文档: 查看 `docs/` 目录
- 🐛 问题: 提交 GitHub Issue
- 💬 讨论: 加入项目讨论
- 📧 联系: yachiyo@example.com

### 报告 Bug
请提供以下信息:
- 操作系统和版本
- 复现步骤
- 预期行为
- 实际行为
- 日志输出

---

## 📜 许可证

本项目采用 MIT 许可证。详见 [LICENSE](./LICENSE) 文件。

---

## 🎉 致谢

感谢所有为本项目做出贡献的开发者!

---

**最后更新**: 2024年  
**维护者**: Yachiyo 开发团队  
**项目状态**: 🟡 积极开发中
