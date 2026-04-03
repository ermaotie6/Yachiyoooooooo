# 🔍 Yachiyo AI 虚拟形象直播平台 - 功能缺失分析与实现方案

**文档版本**: 1.0  
**更新日期**: 2026年4月3日  
**作者**: 项目分析团队  
**目的**: 从代码文件角度分析项目功能完整性，识别缺失功能和配置需求

---

## 📑 目录

1. [项目基本架构](#项目基本架构)
2. [核心功能完整度分析](#核心功能完整度分析)
3. [缺失功能详情](#缺失功能详情)
4. [需要配置的关键模块](#需要配置的关键模块)
5. [实现方法与优先级](#实现方法与优先级)
6. [OpenClaw 正确理解与集成](#openclaw-正确理解与集成)
7. [实施路线图](#实施路线图)

---

## 项目基本架构

### 整体系统流程

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Web 前端 (Vue 3)                            │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  1. 登录界面 → 2. 虚拟形象展示区 → 3. 消息输入框            │   │
│  │             ↓ Live2D 动画       ↓ 实时语音播放             │   │
│  └──────────────────────────────────────────────────────────────┘   │
└────────────────────┬────────────────────────────────────────────────┘
                     │ WebSocket + REST API
                     ↓
┌─────────────────────────────────────────────────────────────────────┐
│                    后端服务 (C++20 + Crow)                          │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ 1. 认证服务 → 2. 消息接收 → 3. 6层安全审查 → 4. OpenClaw   │   │
│  │               ↓ 内容审核       ↓ 任务执行                 │   │
│  │ 5. 翻译服务 → 6. 语音合成 (GPT-SoVITS) → 7. 动作驱动     │   │
│  │               (中文→日文)                (Live2D参数)     │   │
│  │ 8. 实时推送 (WebSocket) → 前端渲染                        │   │
│  └──────────────────────────────────────────────────────────────┘   │
│  ┌──────────────┐  ┌─────────────┐  ┌──────────────┐               │
│  │  PostgreSQL  │  │   Redis     │  │ OpenClaw     │               │
│  │  (用户/消息) │  │ (缓存/会话) │  │ (任务执行)   │               │
│  └──────────────┘  └─────────────┘  └──────────────┘               │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 核心功能完整度分析

### ✅ 已实现的功能

| 功能模块 | 完整度 | 文件位置 | 说明 |
|---------|--------|---------|------|
| 用户认证 | 90% | `AuthService.hpp/.cpp` | JWT认证已实现，缺少注册验证 |
| 消息接收 | 85% | `ChatService.hpp/.cpp` | 基础收发已实现，缺WebSocket流式 |
| 6层审查 | 70% | `MessageServiceImpl.cpp` | 框架已有，AI审查需完善 |
| 数据持久化 | 80% | `models/Message.hpp` | 数据模型完整，ORM映射需完善 |
| 缓存机制 | 60% | `RedisCacheService.hpp` | Redis接口已定义，实现不完整 |
| OpenClaw集成 | 40% | 多个文件 | **关键缺失** - 见下文 |
| 翻译服务 | 30% | 文件不存在 | **缺失** - 需要实现 |
| GPT-SoVITS | 20% | 文件不存在 | **缺失** - 需要实现 |
| Live2D动画 | 25% | 文件不存在 | **缺失** - 需要实现 |
| WebSocket实时推送 | 15% | `controllers/` | **关键缺失** |

---

## 缺失功能详情

### 🔴 优先级最高 - 关键缺失

#### 1. OpenClaw 完整集成 (优先级: 🔴🔴🔴)

**当前状态**: 代码中只有框架定义，无实际实现

**代码位置**:
- `backend/include/services/AIService.hpp` - 已定义 AIProvider 枚举
- `backend/src/services/MessageServiceImpl.cpp` (第407行) - 注释提到 OpenClaw 但未实现
- `backend/config/config.yaml` - 缺少 OpenClaw 配置段

**缺失内容**:
```cpp
// ❌ 完全缺失 - OpenClaw 任务执行层
// 应在: backend/include/services/OpenClawTaskExecutor.hpp
class OpenClawTaskExecutor {
    // 以下功能未实现:
    // 1. 连接到本地/云端 OpenClaw 网关
    // 2. 发送用户消息进行 AI 处理
    // 3. 接收 OpenClaw 返回的:
    //    - 文本响应 (中文)
    //    - 情感标签 (开心/害羞/愤怒等)
    //    - 动作指令 (表情/姿态/时序)
    //    - 置信度评分
    // 4. Webhook 处理 - 接收 OpenClaw 主动推送的事件
    // 5. 缓存管理 - 避免重复请求相同消息
    // 6. 错误恢复 - 连接失败时的降级方案
};
```

**实现方法**:

1️⃣ **创建 OpenClaw 连接管理器**
```cpp
// backend/include/services/OpenClawGateway.hpp
class OpenClawGateway {
public:
    // 初始化连接
    bool connect(const std::string& gatewayUrl, const std::string& apiKey);
    
    // 发送消息给 OpenClaw (同步)
    OpenClawResponse processMessage(const OpenClawRequest& req);
    
    // 发送消息给 OpenClaw (异步)
    std::future<OpenClawResponse> processMessageAsync(const OpenClawRequest& req);
    
    // 健康检查
    bool healthCheck();
    
    // 注册 Webhook 处理器
    void registerWebhookHandler(const WebhookHandler& handler);
};
```

2️⃣ **OpenClaw 请求/响应结构**
```cpp
// backend/include/dto/OpenClawDTO.hpp
struct OpenClawRequest {
    std::string userId;
    std::string message;              // 用户输入 (中文)
    std::string avatarName;           // 虚拟形象名
    std::map<std::string, std::string> context;  // 用户历史/情境
    int64_t timestamp;
    
    // 可选: 上下文信息
    std::vector<Message> chatHistory; // 前N条消息
    std::string currentMood;          // 虚拟形象当前状态
    std::string platform;             // 平台: web/mobile/etc
};

struct OpenClawResponse {
    std::string responseId;
    std::string responseText;         // AI 生成的文本回应 (中文)
    std::vector<std::string> emotions; // ["开心", "害羞", ...]
    std::vector<ActionInstruction> actions;  // 动作序列
    float confidence;                 // 0.0-1.0 置信度
    int64_t processingTimeMs;
};

struct ActionInstruction {
    std::string type;  // "expression" / "gesture" / "pose"
    std::string value; // 具体值: "笑咪咪" / "挥手" / "站立"
    int durationMs;    // 持续时间
    std::string easing; // "linear" / "ease-in" / "ease-out"
};
```

3️⃣ **在消息服务中集成 OpenClaw**
```cpp
// backend/src/services/MessageServiceImpl.cpp - 第4层(AI审查)改进
Result<std::shared_ptr<Message>> MessageServiceImpl::sendMessage(...) {
    // 现有代码 ... 前3层审查 ...
    
    // ✅ 第4层: 改进的 AI 内容审查 - 使用 OpenClaw
    auto openClawRequest = buildOpenClawRequest(userId, message);
    auto openClawResult = openClawGateway->processMessage(openClawRequest);
    
    if (openClawResult.confidence > 0.7) {
        msg->setReviewStatus(ReviewStatus::APPROVED);
        msg->setOpenClawResponse(openClawResult);  // 保存响应供后续使用
    } else {
        msg->setReviewStatus(ReviewStatus::MANUAL_REVIEW);
        msg->setReviewReason("OpenClaw置信度不足");
    }
    
    // 后续阶段: 消息通过审查后立即通过 OpenClaw 响应服务处理
    // 见下文: AvatarResponseService
}
```

**配置需求**:
```yaml
# backend/config/config.yaml - 新增段落
openclaw:
  enabled: true
  gateway_url: "http://localhost:18789"  # 本地 OpenClaw Gateway
  api_key: "your-openclaw-api-key"
  connection_timeout: 5000
  request_timeout: 10000
  
  # Webhook 配置
  webhook:
    enabled: true
    port: 8081
    path: "/webhook/openclaw"
  
  # 缓存配置
  cache:
    enabled: true
    ttl_seconds: 3600
    max_entries: 10000
  
  # 降级配置 (OpenClaw 不可用时)
  fallback:
    enabled: true
    fallback_model: "local"  # 使用本地模型降级
```

---

#### 2. 虚拟形象响应服务 (优先级: 🔴🔴🔴)

**当前状态**: 完全缺失

**缺失内容**:
- 没有 `AvatarResponseService` 的实现
- 没有对 OpenClaw 输出的处理逻辑
- 没有将 OpenClaw 动作转换为前端指令的代码

**实现方法**:

```cpp
// backend/include/services/AvatarResponseService.hpp
class AvatarResponseService {
public:
    struct AvatarResponse {
        std::string messageId;
        std::string responseId;
        
        // 文本部分
        std::string originalText;    // OpenClaw 输出 (中文)
        std::string translatedText;  // 翻译后 (日文/英文)
        std::vector<std::string> emotions;
        
        // 音频部分
        std::string audioUrl;        // 合成语音URL
        std::string audioFormat;     // wav/mp3/etc
        int audioDurationMs;
        
        // 动画部分
        std::vector<AnimationKeyframe> keyframes;
        int totalAnimationDurationMs;
        
        int64_t createdAt;
    };
    
    // 生成完整响应: OpenClaw → 翻译 → 语音 → 动画
    Result<AvatarResponse> generateResponse(
        const std::string& messageId,
        const std::string& userId,
        const OpenClawResponse& openClawResponse
    );
    
private:
    std::shared_ptr<TranslationService> translationService;
    std::shared_ptr<GPTSoVITSService> voiceService;
    std::shared_ptr<Live2DAnimationService> animationService;
};
```

**处理流程**:
1. 接收 OpenClaw 输出 (文本 + 动作指令)
2. 翻译文本: 中文 → 日文/其他语言
3. 调用 GPT-SoVITS 合成语音
4. 将动作指令转换为 Live2D 参数
5. 整合所有部分为统一的 AvatarResponse
6. 通过 WebSocket 推送给前端

---

#### 3. WebSocket 实时推送 (优先级: 🔴🔴🔴)

**当前状态**: 框架定义不完整，缺实现

**缺失内容**:
```cpp
// ❌ 完全缺失实现
// backend/include/controllers/WebSocketController.hpp - 需要完善

class WebSocketController {
public:
    // 目前缺失:
    
    // 1. 消息队列 WebSocket 推送
    void broadcastMessage(const Message& msg);  // ❌ 未实现
    
    // 2. 虚拟形象响应推送 (最关键)
    void broadcastAvatarResponse(
        const AvatarResponse& response  // ❌ 未实现
    );
    
    // 3. 实时动画流推送
    void streamAnimationKeyframes(
        const std::vector<AnimationKeyframe>& keyframes  // ❌ 未实现
    );
    
    // 4. 音频流推送
    void streamAudio(const std::string& audioUrl);  // ❌ 未实现
    
    // 5. 在线用户管理
    void addClient(const std::string& userId, WebSocketConnection* conn);  // ❌ 未实现
    void removeClient(const std::string& userId);  // ❌ 未实现
    
    // 6. 错误处理
    void handleConnectionError(const std::string& userId);  // ❌ 未实现
};
```

**实现方法**:

```cpp
// backend/include/controllers/WebSocketController.hpp
class WebSocketController {
private:
    // 连接管理
    std::map<std::string, std::vector<WebSocketConnection*>> userConnections;
    std::mutex connectionMutex;

public:
    // 处理新连接
    void handleNewConnection(
        const std::string& userId,
        WebSocketConnection* conn
    ) {
        std::lock_guard<std::mutex> lock(connectionMutex);
        userConnections[userId].push_back(conn);
        LOG_INFO("WebSocket 连接: userId={}", userId);
    }
    
    // 推送虚拟形象响应给所有在线用户
    void broadcastAvatarResponse(const AvatarResponse& response) {
        std::lock_guard<std::mutex> lock(connectionMutex);
        
        json payload = {
            {"type", "avatar_response"},
            {"data", {
                {"messageId", response.messageId},
                {"text", response.translatedText},
                {"emotions", response.emotions},
                {"audioUrl", response.audioUrl},
                {"animation", {
                    {"keyframes", response.keyframes},
                    {"duration", response.totalAnimationDurationMs}
                }}
            }}
        };
        
        for (auto& [userId, connections] : userConnections) {
            for (auto* conn : connections) {
                conn->send(payload.dump());
            }
        }
    }
    
    // 推送实时动画帧
    void streamAnimationFrame(
        const AnimationKeyframe& frame,
        int frameIndex,
        int totalFrames
    ) {
        std::lock_guard<std::mutex> lock(connectionMutex);
        
        json payload = {
            {"type", "animation_frame"},
            {"frameIndex", frameIndex},
            {"totalFrames", totalFrames},
            {"parameters", frame.parameters},
            {"easing", frame.easing}
        };
        
        for (auto& [userId, connections] : userConnections) {
            for (auto* conn : connections) {
                conn->send(payload.dump());
            }
        }
    }
};
```

---

### 🟡 优先级高 - 主要缺失

#### 4. 翻译服务 (优先级: 🟡🟡🟡)

**当前状态**: 完全缺失

**需要实现**:
```cpp
// backend/include/services/TranslationService.hpp
class TranslationService {
public:
    enum class Language { CHINESE, JAPANESE, ENGLISH, KOREAN };
    
    struct TranslationRequest {
        std::string text;
        Language targetLanguage;
        Language sourceLanguage = Language::CHINESE;
    };
    
    struct TranslationResponse {
        std::string originalText;
        std::string translatedText;
        Language sourceLanguage;
        Language targetLanguage;
        float confidence;
        int64_t processingTimeMs;
    };
    
    // 翻译接口
    Result<TranslationResponse> translate(const TranslationRequest& req);
    
private:
    // 支持多个翻译引擎
    enum class Engine { OPENAI, GOOGLE, BAIDU, LOCAL };
    Engine preferredEngine;
};
```

**多语言支持的虚拟形象输出**:
- 中文 (原始): OpenClaw 生成
- 日文 (翻译): 通过 TranslationService 转换
- 英文 (可选): 同上
- 韩文 (可选): 同上

**实现选项**:
1. OpenAI GPT 翻译 (质量最好，需API key)
2. Google 翻译 API (稳定，需API key)
3. 本地 Ollama 模型 (免费，但质量一般)
4. Baidu 翻译 (专业级，需API key)

---

#### 5. GPT-SoVITS 语音合成 (优先级: 🟡🟡🟡)

**当前状态**: 完全缺失

**需要实现**:
```cpp
// backend/include/services/GPTSoVITSService.hpp
class GPTSoVITSService {
public:
    struct VoiceConfig {
        float speed = 1.0;      // 语速: 0.5-2.0
        float pitch = 0.0;      // 音调: -12.0 to +12.0
        float energy = 1.0;     // 能量: 0.0-2.0
        std::string emotion;    // 情感: happy/sad/angry/neutral
        std::string speaker;    // 说话者: 八千代/其他
    };
    
    struct SynthesisRequest {
        std::string text;       // 要转语音的文本
        std::string language;   // 语言: zh/ja/en/ko
        VoiceConfig config;
        bool returnMp3 = true;  // 返回格式
    };
    
    struct SynthesisResponse {
        std::string audioUrl;           // 生成的音频URL
        std::string audioBase64;        // Base64编码的音频
        int durationMs;
        std::string format;             // wav/mp3/ogg
    };
    
    // 合成语音
    Result<SynthesisResponse> synthesize(const SynthesisRequest& req);
    
    // 流式生成 (逐字生成)
    Result<void> synthesizeStream(
        const SynthesisRequest& req,
        std::function<void(const AudioChunk&)> callback
    );
};
```

**配置与部署**:
```yaml
# backend/config/config.yaml - 新增
gpt_sovits:
  enabled: true
  service_url: "http://localhost:5000"  # GPT-SoVITS 服务地址
  models:
    - name: "yachiyo"
      speaker_id: "yachiyo_8000hz"
      reference_audio: "/models/voice_samples/yachiyo.wav"
  
  # 缓存已生成的音频
  cache:
    enabled: true
    directory: "/var/cache/audio"
    ttl_hours: 24
```

---

#### 6. Live2D 动画驱动 (优先级: 🟡🟡🟡)

**当前状态**: 完全缺失

**需要实现**:
```cpp
// backend/include/services/Live2DAnimationService.hpp
class Live2DAnimationService {
public:
    struct AnimationKeyframe {
        int64_t timestampMs;
        std::map<std::string, float> parameters;  // Live2D 参数
        std::string easing;
    };
    
    struct AnimationSequence {
        std::string sequenceId;
        std::vector<AnimationKeyframe> keyframes;
        int totalDurationMs;
        bool loop;
    };
    
    // 将 OpenClaw 动作转为 Live2D 参数
    Result<AnimationSequence> convertActionsToAnimation(
        const std::vector<ActionInstruction>& actions,
        const std::string& avatarId
    );
    
    // 生成表情动画
    Result<AnimationSequence> generateExpressionAnimation(
        const std::string& expression,  // "笑咪咪", "害羞", ...
        int durationMs
    );
    
    // 生成姿态动画
    Result<AnimationSequence> generatePoseAnimation(
        const std::string& pose,  // "挥手", "点头", ...
        int durationMs
    );
};
```

**Live2D 参数映射示例**:
```cpp
// 表情参数
{
    "EyesOpen": 1.0,          // 0.0-1.0
    "Eyebrows": 0.5,          // 0.0-1.0
    "Mouth": 0.8,             // 0.0-1.0
    "FacialHairControl": 0.0
}

// 姿态参数
{
    "Angle": 0.0,             // 头部角度: -30 to +30
    "AngleX": 0.0,            // X轴旋转
    "AngleY": 0.0,            // Y轴旋转
    "AngleZ": 0.0,            // Z轴旋转
    "BodyAngleX": 0.0,
    "BodyAngleY": 0.0,
    "BodyAngleZ": 0.0
}
```

---

### 🟢 优先级中等 - 需要完善

#### 7. 消息内容审查机制 (优先级: 🟢🟢)

**当前状态**: 框架存在，实现不完整

**缺失内容**:

```cpp
// backend/src/services/MessageServiceImpl.cpp

// 🟡 第3层: 关键词检查 - 需要充实黑名单库
Result<std::pair<bool, double>> MessageServiceImpl::checkBlockedKeywords(
    const std::string& message
) {
    // ❌ 目前只有框架
    // 需要实现:
    // 1. 使用 Aho-Corasick 自动机提高检索性能
    // 2. 维护多层级黑名单:
    //    - 严格黑名单 (必须拦截)
    //    - 灰名单 (标记待审查)
    //    - 正则表达式规则
    // 3. 支持热更新 (无需重启)
    // 4. 缓存检查结果
}

// 🟡 第4层: AI 审查 - 需要真正的 AI 模型
Result<std::pair<bool, double>> MessageServiceImpl::aiContentReview(
    const std::string& message
) {
    // ❌ 目前是占位符
    // 需要实现:
    // 1. 对接 OpenClaw 进行内容安全检查
    // 2. 检测:
    //    - 侮辱性内容
    //    - 垃圾消息
    //    - 色情内容
    //    - 暴力内容
    //    - 广告推广
    // 3. 返回风险评分 (0.0-1.0)
}

// 🟡 第5层: 行为分析 - 需要真实实现
Result<bool> MessageServiceImpl::behaviorAnalysis(
    int64_t userId,
    const std::string& userIp
) {
    // ❌ 目前是占位符
    // 需要实现:
    // 1. 检测刷屏 (相同消息频繁发送)
    // 2. 检测速率异常 (短时间内消息过多)
    // 3. 检测分布式攻击 (多个IP相同模式)
    // 4. 使用机器学习模型检测异常行为
    // 5. 维护用户行为档案
}
```

**改进方案**:

```cpp
// backend/include/services/ContentModerationService.hpp
class ContentModerationService {
public:
    struct ModerationResult {
        bool isClean;
        float riskScore;  // 0.0-1.0
        std::vector<std::string> flaggedCategories;
        std::string reason;
    };
    
    // 6层完整审查流程
    ModerationResult moderateMessage(
        const std::string& message,
        int64_t userId,
        const std::string& userIp
    );
    
private:
    // 层级1: 速率限制
    bool checkRateLimit(int64_t userId, const std::string& userIp);
    
    // 层级2: IP黑名单
    bool checkIPBlacklist(const std::string& userIp);
    
    // 层级3: 关键词过滤
    ModerationResult checkKeywords(const std::string& message);
    
    // 层级4: AI内容审查 (使用 OpenClaw)
    ModerationResult aiReview(const std::string& message);
    
    // 层级5: 行为分析
    ModerationResult behaviorCheck(int64_t userId, const std::string& userIp);
    
    // 层级6: 人工审查标记
    bool needsManualReview(const ModerationResult& result);
};
```

---

#### 8. 前端消息输入和显示 (优先级: 🟢🟢)

**当前状态**: 基本框架存在，功能不完整

**缺失内容**:

前端文件: `frontend/src/views/Chat.vue`

```vue
<!-- 目前缺失的功能: -->

<!-- 1. 字数限制显示 -->
<el-input
  v-model="messageContent"
  type="textarea"
  :maxlength="500"
  show-word-limit
  @keyup.enter="sendMessage"
/>

<!-- 2. 实时消息流 -->
<!-- ❌ 缺失: WebSocket 连接处理 -->
<!-- 应该显示: -->
<!-- - 用户消息 (从消息框) -->
<!-- - OpenClaw 响应 (实时更新) -->
<!-- - 虚拟形象动画 (Live2D) -->
<!-- - 语音播放 (自动) -->

<!-- 3. 虚拟形象显示区 -->
<div class="avatar-container">
  <!-- ❌ 完全缺失: Live2D 组件集成 -->
  <!-- 应该显示: 八千代虚拟形象 + 实时动画 -->
  <canvas ref="liveCanvas"></canvas>
</div>

<!-- 4. 音频播放 -->
<!-- ❌ 缺失: 自动播放 OpenClaw 生成的语音 -->
<audio ref="audioPlayer" @ended="onAudioEnded"></audio>

<!-- 5. 消息历史显示 -->
<!-- ❌ 不完整: 需要标记哪些是用户消息, 哪些是OpenClaw响应 -->
<div class="message-history">
  <div v-for="msg in messages" :key="msg.id" :class="msg.sender">
    {{ msg.content }}
  </div>
</div>
```

**实现步骤**:

1️⃣ **创建 Chat.vue 完整版本**
```vue
<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useWebSocket } from '@/composables/useWebSocket'
import { useAudioPlayer } from '@/composables/useAudioPlayer'
import Live2DComponent from '@/components/Live2D.vue'

const messageContent = ref('')
const messages = ref([])
const { send: wsMessage, onMessage } = useWebSocket()
const { play: playAudio } = useAudioPlayer()

// 发送消息
const sendMessage = async () => {
  if (!messageContent.value.trim()) return
  
  // 1. 显示用户消息
  messages.value.push({
    id: Date.now(),
    sender: 'user',
    content: messageContent.value
  })
  
  // 2. 发送到后端
  wsMessage({
    type: 'user_message',
    content: messageContent.value
  })
  
  messageContent.value = ''
}

// 监听 WebSocket 消息
onMessage((event) => {
  const data = JSON.parse(event.data)
  
  if (data.type === 'avatar_response') {
    // 3. 显示虚拟形象响应
    messages.value.push({
      id: Date.now(),
      sender: 'avatar',
      content: data.data.text
    })
    
    // 4. 播放音频
    playAudio(data.data.audioUrl)
    
    // 5. 播放动画
    // $refs.live2d.playAnimation(data.data.animation)
  }
})

onMounted(() => {
  wsMessage({ type: 'connect' })
})
</script>

<template>
  <div class="chat-container">
    <!-- 虚拟形象显示 -->
    <Live2DComponent class="avatar" />
    
    <!-- 消息历史 -->
    <div class="message-list">
      <div v-for="msg in messages" :key="msg.id" :class="['message', msg.sender]">
        {{ msg.content }}
      </div>
    </div>
    
    <!-- 消息输入 -->
    <div class="input-area">
      <el-input
        v-model="messageContent"
        type="textarea"
        maxlength="500"
        show-word-limit
        @keyup.enter="sendMessage"
      />
      <el-button @click="sendMessage">发送</el-button>
    </div>
  </div>
</template>
```

2️⃣ **创建 Live2D 组件**
```vue
<!-- frontend/src/components/Live2D.vue -->
<script setup lang="ts">
import { ref, onMounted } from 'vue'

const canvasRef = ref()
let live2dModel

// 加载 Live2D 模型
onMounted(async () => {
  // 需要集成 Live2D SDK
  // 加载虚拟形象模型: 八千代
  // live2dModel = new Live2D.Cubism(modelPath)
})

// 播放动画
const playAnimation = (animation) => {
  // 根据 animation.keyframes 播放
  // 支持表情、姿态、动作等
}

defineExpose({ playAnimation })
</script>

<template>
  <canvas
    ref="canvasRef"
    class="live2d-canvas"
    width="1024"
    height="768"
  />
</template>
```

---

## 需要配置的关键模块

### 🔧 配置检查清单

| 配置项 | 文件位置 | 状态 | 优先级 |
|--------|---------|------|--------|
| OpenClaw 网关 | `config.yaml` | ❌ 缺失 | 🔴🔴🔴 |
| GPT-SoVITS | `config.yaml` | ❌ 缺失 | 🔴🔴🔴 |
| 翻译服务 | `config.yaml` | ❌ 缺失 | 🔴🔴 |
| Redis 连接 | `config.yaml` | ⚠️ 不完整 | 🟡 |
| PostgreSQL | `config.yaml` | ✅ 存在 | ✅ |
| JWT 密钥 | `config.yaml` | ⚠️ 使用默认值 | 🟡 |
| WebSocket | 代码中 | ❌ 缺失 | 🔴🔴🔴 |
| Live2D 模型 | 资源目录 | ❌ 缺失 | 🔴🔴 |

### 必需配置更新

```yaml
# backend/config/config.yaml - 完整版本

# OpenClaw 配置 (NEW - 关键)
openclaw:
  enabled: true
  gateway_url: "http://localhost:18789"
  api_key: "${OPENCLAW_API_KEY}"
  connection_timeout: 5000
  request_timeout: 10000
  cache:
    enabled: true
    ttl_seconds: 3600
    max_entries: 10000
  fallback:
    enabled: true
    fallback_model: "local"

# GPT-SoVITS 配置 (NEW - 关键)
gpt_sovits:
  enabled: true
  service_url: "http://localhost:5000"
  models:
    - name: "yachiyo"
      speaker_id: "yachiyo_8000hz"
      reference_audio: "/models/voice_samples/yachiyo.wav"
  cache:
    enabled: true
    directory: "/var/cache/audio"
    ttl_hours: 24

# 翻译服务配置 (NEW - 关键)
translation:
  enabled: true
  primary_engine: "openai"  # openai / google / baidu / local
  engines:
    openai:
      api_key: "${OPENAI_API_KEY}"
      model: "gpt-3.5-turbo"
    google:
      api_key: "${GOOGLE_TRANSLATE_KEY}"
    baidu:
      app_id: "${BAIDU_APP_ID}"
      secret_key: "${BAIDU_SECRET_KEY}"
  cache:
    enabled: true

# WebSocket 配置 (UPDATE)
server:
  websocket:
    enabled: true
    path: "/ws/chat"
    max_connections: 1000
    heartbeat_interval: 30

# 内容审查配置 (UPDATE)
moderation:
  enabled: true
  rate_limit:
    messages_per_minute: 30
    messages_per_hour: 500
  keywords:
    blacklist_file: "/etc/blacklist_keywords.txt"
    update_interval: 3600
  ai_review:
    min_confidence: 0.7
  manual_review_threshold: 0.5
```

---

## 实现方法与优先级

### Phase 1 - 第一阶段 (即时 - 1-2 周)

**目标**: 建立 OpenClaw 集成和实时推送的基础框架

| 任务 | 预期工作量 | 难度 |
|-----|---------|------|
| 1. 创建 OpenClaw 网关类 | 3-4小时 | ⭐⭐⭐ |
| 2. 实现 WebSocket 控制器 | 4-5小时 | ⭐⭐⭐⭐ |
| 3. 创建虚拟形象响应服务 | 2-3小时 | ⭐⭐ |
| 4. 配置 config.yaml | 1小时 | ⭐ |
| 5. 修改消息服务集成 | 2小时 | ⭐⭐ |
| **小计** | **12-15小时** | |

### Phase 2 - 第二阶段 (后续 - 2-3 周)

**目标**: 实现翻译、语音、动画等完整服务

| 任务 | 预期工作量 | 难度 |
|-----|---------|------|
| 1. 翻译服务实现 | 2-3小时 | ⭐⭐ |
| 2. GPT-SoVITS 集成 | 3-4小时 | ⭐⭐⭐ |
| 3. Live2D 服务实现 | 4-5小时 | ⭐⭐⭐⭐ |
| 4. 前端 Chat.vue 完整化 | 3-4小时 | ⭐⭐⭐ |
| 5. Live2D Vue 组件 | 2-3小时 | ⭐⭐⭐ |
| **小计** | **14-19小时** | |

### Phase 3 - 第三阶段 (优化 - 1-2 周)

**目标**: 完善审查机制、缓存、错误处理

| 任务 | 预期工作量 | 难度 |
|-----|---------|------|
| 1. 完善内容审查 6层 | 3-4小时 | ⭐⭐⭐ |
| 2. 缓存系统优化 | 2小时 | ⭐⭐ |
| 3. 错误恢复与降级 | 2-3小时 | ⭐⭐⭐ |
| 4. 性能测试与优化 | 4-5小时 | ⭐⭐⭐⭐ |
| 5. 文档更新 | 2小时 | ⭐ |
| **小计** | **13-16小时** | |

**总计: 39-50 小时 (4-6 人周)**

---

## OpenClaw 正确理解与集成

### 🤖 OpenClaw 是什么？

**错误理解**:
❌ "OpenClaw 是一款 AI 聊天机器人"
❌ "OpenClaw 用来处理自然语言"
❌ "OpenClaw 只能在云端使用"

**正确理解**:
✅ **OpenClaw 是一个自主人工智能虚拟助理框架**
- 可执行任务 (自动化操作): 日程管理、消息发送、文件处理、代码生成等
- 本地部署: 可在 Windows、macOS、Linux 本地设备上运行
- 多 AI 集成: 调用其他 AI 大模型 (GPT/Claude/Gemini) 和应用 API
- 持久记忆: 在本地存储配置数据和交互历史，拥有持久的上下文理解

### 📌 Yachiyo 中的 OpenClaw 用途

在 Yachiyo 虚拟形象直播平台中，OpenClaw 的角色：

```
用户消息 → OpenClaw 处理 → 返回:
  • 文本回应 (调用 GPT 等生成)
  • 情感标签 (分析用户情绪)
  • 动作指令 (生成虚拟形象动作)
  • 语音参数 (速度/音调/能量)
```

### 🔄 集成架构

```
┌─────────────────────────────────────────────────┐
│              Yachiyo 前端 (Vue 3)               │
│          用户输入消息 (字数限制 500)             │
└────────────────┬────────────────────────────────┘
                 │ WebSocket / REST
                 ↓
┌─────────────────────────────────────────────────┐
│         【消息接收与安全审查】                   │
│  ├─ 层级1: 速率限制                             │
│  ├─ 层级2: IP黑名单                             │
│  ├─ 层级3: 关键词过滤                           │
│  ├─ 层级4: AI审查 ← 📍 OpenClaw 参与            │
│  ├─ 层级5: 行为分析                             │
│  └─ 层级6: 人工审查标记                         │
└────────────────┬────────────────────────────────┘
                 │ (审查通过)
                 ↓
┌─────────────────────────────────────────────────┐
│        【OpenClaw 任务处理】 ★ 核心              │
│  ├─ 接收: 用户消息 + 上下文 + 虚拟形象设定     │
│  ├─ 执行: 调用外部AI大模型生成响应              │
│  └─ 返回:                                       │
│      ├─ 文本 (中文)                             │
│      ├─ 情感 [开心/害羞/愤怒/...]              │
│      └─ 动作 [笑咪咪/挥手/点头/...]            │
└────────────────┬────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────┐
│       【多层处理与合成】                        │
│  ├─ 翻译服务: 中文 → 日文/英文                  │
│  ├─ 语音合成: 文本 → 音频 (GPT-SoVITS)         │
│  ├─ 动画驱动: 动作 → Live2D 参数                │
│  └─ 组合: 文本 + 音频 + 动画                    │
└────────────────┬────────────────────────────────┘
                 │ WebSocket 实时推送
                 ↓
┌─────────────────────────────────────────────────┐
│        前端实时渲染                              │
│  ├─ 显示虚拱文字 (翻译后的文本)                 │
│  ├─ 播放语音 (自动)                             │
│  └─ 动画 (Live2D 执行动作指令)                  │
└─────────────────────────────────────────────────┘
```

### 🔧 集成具体步骤

#### Step 1: 部署 OpenClaw

```bash
# 在本地安装 OpenClaw
npm install -g openclaw@latest

# 启动 OpenClaw 网关
openclaw onboard --install-daemon
openclaw start

# 验证服务 (应在 http://localhost:18789)
curl http://localhost:18789/health
```

#### Step 2: 配置 Yachiyo

```yaml
# backend/config/config.yaml
openclaw:
  gateway_url: "http://localhost:18789"
  api_key: "your-key"
  enabled: true
```

#### Step 3: 实现网关类

```cpp
// backend/include/services/OpenClawGateway.hpp
class OpenClawGateway {
public:
    // 发送消息给 OpenClaw
    OpenClawResponse processMessage(const OpenClawRequest& req) {
        // 1. 构建 HTTP 请求
        // 2. POST 到 OpenClaw API
        // 3. 解析响应
        // 4. 缓存结果
        // 5. 返回结构化数据
    }
};
```

#### Step 4: 集成到消息处理

```cpp
// backend/src/services/MessageServiceImpl.cpp
// 在消息通过审查后调用:

auto openClawResponse = openClawGateway->processMessage(request);

// 保存响应供后续处理
msg->setOpenClawResponse(openClawResponse);
```

#### Step 5: 实现虚拟形象响应

```cpp
// backend/services/AvatarResponseService
// 整合 OpenClaw 输出 + 翻译 + 语音 + 动画
```

---

## 实施路线图

### 📅 时间表 (建议)

```
周1-2 (Phase 1)
├─ Mon: 需求分析与代码审查
├─ Tue-Wed: OpenClaw 网关开发
├─ Wed-Thu: WebSocket 控制器开发
├─ Thu-Fri: 虚拱形象响应服务开发
└─ Fri: 集成测试

周3-4 (Phase 2)
├─ Mon-Tue: 翻译服务实现
├─ Tue-Wed: GPT-SoVITS 集成
├─ Wed-Thu: Live2D 服务实现
├─ Thu: 前端组件开发
└─ Fri: 端到端测试

周5-6 (Phase 3)
├─ Mon: 审查机制完善
├─ Tue: 缓存与性能优化
├─ Wed-Thu: 测试与修复
├─ Fri: 文档和 Demo
```

### 🚀 部署检查清单

**开发环境**:
- [ ] C++20 编译器 (GCC 11+ / Clang 13+)
- [ ] CMake 3.20+
- [ ] PostgreSQL 13+
- [ ] Redis 6+
- [ ] Node.js 16+
- [ ] OpenClaw 已安装
- [ ] GPT-SoVITS 已部署
- [ ] Live2D SDK 已集成

**代码检查**:
- [ ] 所有关键类已实现
- [ ] 异常处理完整
- [ ] 日志记录充分
- [ ] 单元测试 >80% 覆盖率
- [ ] 集成测试通过

**配置检查**:
- [ ] config.yaml 已完整配置
- [ ] 环境变量已设置
- [ ] API Key 已配置
- [ ] 缓存策略已定义
- [ ] 备份方案已准备

**前端检查**:
- [ ] Vue 3 组件已实现
- [ ] WebSocket 连接正常
- [ ] Live2D 模型已加载
- [ ] 音频播放正常
- [ ] 消息流式显示正常

---

## 关键文件创建清单

### 后端需要创建的文件

```
backend/include/services/
├── ✅ OpenClawGateway.hpp          (新建)
├── ✅ AvatarResponseService.hpp    (新建)
├── ✅ TranslationService.hpp       (新建)
├── ✅ GPTSoVITSService.hpp         (新建)
├── ✅ Live2DAnimationService.hpp   (新建)
└── ✅ ContentModerationService.hpp (新建)

backend/include/controllers/
└── ✅ WebSocketController.hpp      (修改/完善)

backend/include/dto/
├── ✅ OpenClawDTO.hpp            (新建)
└── ✅ AvatarResponseDTO.hpp       (新建)

backend/src/services/
├── ✅ OpenClawGateway.cpp         (新建)
├── ✅ AvatarResponseService.cpp   (新建)
├── ✅ TranslationService.cpp      (新建)
├── ✅ GPTSoVITSService.cpp        (新建)
├── ✅ Live2DAnimationService.cpp  (新建)
├── ✅ MessageServiceImpl.cpp       (修改 - 第4层)
└── ✅ ChatService.cpp             (修改 - WebSocket集成)

backend/src/controllers/
└── ✅ WebSocketController.cpp     (新建/完善)

backend/config/
└── ✅ config.yaml               (修改 - 新增配置段)
```

### 前端需要创建的文件

```
frontend/src/
├── views/
│   └── ✅ Chat.vue              (完全改写)
│
├── components/
│   ├── ✅ Live2D.vue            (新建)
│   ├── ✅ MessageList.vue       (新建)
│   ├── ✅ MessageInput.vue      (新建)
│   └── ✅ AudioPlayer.vue       (新建)
│
├── composables/
│   ├── ✅ useWebSocket.ts       (新建)
│   ├── ✅ useAudioPlayer.ts     (新建)
│   └── ✅ useAvatarAnimation.ts (新建)
│
├── types/
│   ├── ✅ avatar.ts             (新建)
│   └── ✅ websocket.ts          (新建)
│
└── services/
    └── ✅ avatarService.ts      (新建)
```

---

## 总结

### 📊 完整度评估

| 模块 | 完整度 | 需要工作 | 预计投入 |
|-----|--------|---------|---------|
| 认证系统 | 90% | 5% | 1小时 |
| 消息管理 | 75% | 25% | 4小时 |
| 内容审查 | 50% | 50% | 6小时 |
| **OpenClaw集成** | **20%** | **80%** | **8小时** |
| **虚拱响应服务** | **0%** | **100%** | **8小时** |
| **翻译服务** | **0%** | **100%** | **3小时** |
| **语音合成** | **0%** | **100%** | **4小时** |
| **Live2D动画** | **0%** | **100%** | **5小时** |
| **WebSocket推送** | **10%** | **90%** | **5小时** |
| **前端界面** | **50%** | **50%** | **7小时** |
| **总体** | **48%** | **52%** | **51小时** |

### 🎯 核心优先级

**必须立即实现 (本周)**:
1. ✅ OpenClaw 网关集成
2. ✅ WebSocket 实时推送
3. ✅ 虚拱形象响应服务

**需要尽快实现 (下周)**:
1. ✅ 翻译服务
2. ✅ GPT-SoVITS 语音合成
3. ✅ Live2D 动画驱动
4. ✅ 前端完整化

**可以后续优化 (2周后)**:
1. ✅ 内容审查完善
2. ✅ 性能优化
3. ✅ 错误恢复

### 📚 参考资源

- **OpenClaw 官网**: https://openclaw.io/
- **GPT-SoVITS**: https://github.com/RVC-Boss/GPT-SoVITS
- **Live2D**: https://www.live2d.com/
- **Crow 框架**: https://crowcpp.org/
- **WebSocket Protocol**: RFC 6455

---

**文档完成日期**: 2026年4月3日  
**下一步**: 按照优先级开始实现 Phase 1 的功能
