# 🎭 Yachiyo 虚拟形象交互系统 - 完整实现方案

> **项目**: Yachiyo 虚拟主播系统  
> **模块**: 虚拟形象交互核心  
> **创建日期**: 2026年4月2日  
> **技术栈**: C++20 + OpenClaw + GPT-SoVITS + Live2D  
> **难度**: ⭐⭐⭐⭐ 复杂  
> **工作量**: 60-80 小时

---

## 📐 系统架构总览

### 完整交互流程图

```
┌─────────────────────────────────────────────────────────────┐
│                     用户交互层                              │
├─────────────────────────────────────────────────────────────┤
│  已注册用户 → 输入文本 → 发送 → 安全审核                   │
│  (User) → (Text Input) → (Submit) → (Content Review)      │
└────────────────┬────────────────────────────────────────────┘
                 │ 文本通过安全审核
                 ▼
┌─────────────────────────────────────────────────────────────┐
│              Yachiyo 消息服务                               │
├─────────────────────────────────────────────────────────────┤
│  1. 6层内容审核系统 (MessageServiceImpl)                    │
│     - 速率限制、IP黑名单、敏感词过滤                      │
│     - 启发式分析、行为分析、人工审核标记                  │
│  2. 消息存储 (PostgreSQL)                                 │
│  3. 消息入队 (实时更新队列)                               │
└────────────────┬────────────────────────────────────────────┘
                 │ 消息已安全存储
                 ▼
┌─────────────────────────────────────────────────────────────┐
│                  实时聊天框                                 │
├─────────────────────────────────────────────────────────────┤
│  [从上到下顺序显示消息]                                     │
│  ┌─────────────────────────────┐                          │
│  │ 用户1: 你好啊！             │                          │
│  ├─────────────────────────────┤                          │
│  │ [八千代处理中...]           │                          │
│  └─────────────────────────────┘                          │
└────────────────┬────────────────────────────────────────────┘
                 │ 虚拟形象按顺序处理
                 ▼
┌─────────────────────────────────────────────────────────────┐
│           OpenClaw 自主代理处理                            │
├─────────────────────────────────────────────────────────────┤
│  输入: 消息文本 (Chinese)                                   │
│  │                                                          │
│  └─→ 情感分析 & 内容理解                                   │
│      └─→ 生成虚拟形象回应                                  │
│          └─→ 输出: (文本 + 表情) + (动作)                │
└────────────────┬────────────────────────────────────────────┘
                 │ 输出分为两部分
                 │
        ┌────────┴─────────────┐
        │                      │
        ▼                      ▼
┌──────────────────┐    ┌─────────────────────┐
│    文本输出      │    │    动作输出         │
│  Part A          │    │  Part B             │
└────────┬─────────┘    └──────────┬──────────┘
         │                         │
         ▼                         ▼
   ┌────────────┐          ┌──────────────┐
   │ 文本 + 表情│          │ Live2D 参数  │
   │ (中文)    │          │ 控制指令     │
   │ Ex:       │          │              │
   │ "哈哈，   │          │ ParamHead... │
   │  真有趣  │          │ ParamMouth..│
   │ 😂"      │          │ ParamEyeL...│
   └────┬──────┘          └──────┬───────┘
        │                        │
        ▼                        ▼
   ┌────────────┐          ┌──────────────┐
   │ 日语翻译   │          │ 实时应用动作 │
   │ (日本語)  │          │ (60 FPS)    │
   │ Ex:       │          └──────┬───────┘
   │ "ハハハ， │                 │
   │  面白い！"│                 ▼
   └────┬──────┘          ┌──────────────┐
        │                 │ 屏幕更新     │
        ▼                 │ 虚拟形象动画 │
   ┌────────────┐         └──────────────┘
   │ GPT-SoVITS │
   │ TTS 合成   │
   │ (日语声音) │
   └────┬──────┘
        │
        ▼
   ┌────────────────────┐
   │ 音频输出           │
   │ + 屏幕显示         │
   │ + 虚拟形象动作     │
   │ = 完整体验         │
   └────────────────────┘
```

---

## 🔄 核心流程详解

### 第一阶段: 用户输入与安全审核

#### 1.1 用户认证与消息提交

```cpp
// API 端点: POST /api/messages/submit
{
    "user_id": "user_12345",
    "content": "你好啊，八千代！",
    "platform": "web",
    "timestamp": 1743667200
}
```

**验证步骤**:
1. ✅ 检查用户是否已注册 (JWT 令牌验证)
2. ✅ 检查速率限制 (Redis)
3. ✅ 检查内容长度 (1-500 字符)
4. ✅ 检查文本编码 (UTF-8)

#### 1.2 六层内容审核系统

```cpp
// MessageService 中的 6 层审核逻辑

Layer 1: 速率限制
└─→ Redis 检查: 用户 1 分钟内消息数 < 10

Layer 2: IP 黑名单
└─→ PostgreSQL 检查: IP 是否在黑名单中

Layer 3: 敏感词过滤
└─→ 本地内存词库: 过滤 50+ 个敏感关键词

Layer 4: 启发式 AI 分析
└─→ 本地规则: 检查大写字母比例、链接、邮箱等

Layer 5: 行为分析
└─→ 用户历史: 检查用户发送模式、内容风格

Layer 6: 人工审核标记
└─→ 如果 Confidence < 0.7, 标记为待审核

返回结果:
{
    "allowed": true,
    "confidence": 0.95,
    "risk_score": 0.05,
    "categories": ["normal"]
}
```

**代码实现** (MessageServiceImpl.cpp):

```cpp
// 简化版本，展示核心逻辑
bool MessageServiceImpl::validateAndStoreMessage(
    const std::string& userId,
    const std::string& content,
    MessageDTO& outMessage) {
    
    // Layer 1-3: 快速检查
    if (!passRateLimiting(userId)) {
        LOG_WARN("速率限制: {}", userId);
        return false;
    }
    
    if (!passIPBlacklist()) {
        LOG_WARN("IP黑名单");
        return false;
    }
    
    if (!passSensitiveFilter(content)) {
        LOG_WARN("敏感词检测");
        return false;
    }
    
    // Layer 4-6: AI 和行为分析
    auto reviewResult = aiContentReview(content);
    
    if (reviewResult.allowed) {
        // 存储消息
        outMessage.id = generateMessageId();
        outMessage.userId = userId;
        outMessage.content = content;
        outMessage.timestamp = getCurrentTimestamp();
        outMessage.status = MessageStatus::PENDING_AVATAR_RESPONSE;
        
        // 保存到数据库
        messageRepository->save(outMessage);
        
        // 加入实时队列
        realTimeMessageQueue->enqueue(outMessage);
        
        LOG_INFO("消息已存储: {}, ID: {}", userId, outMessage.id);
        return true;
    }
    
    return false;
}
```

---

### 第二阶段: 虚拟形象处理队列

#### 2.1 消息队列管理

```cpp
// 新建文件: include/services/AvatarMessageQueueService.hpp

class AvatarMessageQueueService {
public:
    struct QueuedMessage {
        std::string messageId;
        std::string userId;
        std::string content;
        int64_t timestamp;
        MessageProcessingStatus status;  // PENDING, PROCESSING, COMPLETED
    };
    
    // 入队
    void enqueueMessage(const MessageDTO& message);
    
    // 获取下一条待处理消息
    std::optional<QueuedMessage> getNextMessage();
    
    // 更新处理状态
    void updateProcessingStatus(const std::string& messageId,
                               MessageProcessingStatus status);
    
    // 获取队列状态
    QueueStats getQueueStats();
    
private:
    std::queue<QueuedMessage> messageQueue;
    std::unordered_map<std::string, QueuedMessage> processingMap;
    std::mutex queueMutex;
};
```

#### 2.2 虚拟形象处理器

```cpp
// 新建文件: include/services/AvatarResponseService.hpp

class AvatarResponseService {
public:
    struct AvatarResponse {
        // Part A: 文本输出
        struct TextOutput {
            std::string chineseText;      // 中文回应文本
            std::string japaneseText;     // 日语翻译
            std::vector<std::string> expressions;  // 表情列表 [😊, 🥰, ...]
            float emotionIntensity;       // 情感强度 [0, 1]
        } textPart;
        
        // Part B: 动作输出
        struct MotionOutput {
            std::string expression;       // 表情名 (笑咪咪、眯眯眼等)
            std::map<std::string, float> parameters;  // Live2D 参数
            float animationDuration;      // 动画时长 (秒)
            std::vector<ParameterKeyframe> keyframes;  // 关键帧
        } motionPart;
        
        int64_t responseTime;            // 生成时间戳
        std::string responseId;          // 响应 ID
    };
    
    // 调用 OpenClaw 处理消息
    AvatarResponse processMessageWithOpenClaw(
        const std::string& messageContent,
        const std::string& userId);
    
    // 将结果存储到数据库
    void storeResponse(const AvatarResponse& response,
                      const std::string& messageId);
    
private:
    std::shared_ptr<OpenClawClient> openClawClient;
};
```

---

### 第三阶段: OpenClaw 处理与输出生成

#### 3.1 OpenClaw 集成

```cpp
// 新建文件: include/models/OpenClawIntegration.hpp

class OpenClawIntegration {
public:
    struct OpenClawRequest {
        std::string messageId;
        std::string userMessage;        // 用户输入 (中文)
        std::string userName;           // 用户名
        std::string avatarName;         // 虚拟形象名 (八千代辉夜姬)
        std::map<std::string, std::string> context;  // 上下文信息
    };
    
    struct OpenClawResponse {
        std::string responseId;
        
        // 文本部分
        std::string responseText;       // OpenClaw 生成的回应文本 (中文)
        std::vector<std::string> emotionTags;  // [开心, 害羞, ...]
        float confidenceScore;
        
        // 动作部分
        std::vector<ActionInstruction> actions;  // 动作指令序列
        int totalDurationMs;            // 总动画时长
    };
    
    struct ActionInstruction {
        std::string type;               // "expression" 或 "parameter"
        std::string value;              // 表情名或参数 ID
        float magnitude;                // 值或强度
        int durationMs;                 // 时长
        std::string easing;             // 过渡曲线 (linear, ease-in-out)
    };
    
    // 发送请求到 OpenClaw
    OpenClawResponse sendToOpenClaw(const OpenClawRequest& request);
    
    // 解析 OpenClaw 响应
    void parseOpenClawResponse(const std::string& rawResponse,
                              OpenClawResponse& outResponse);
};
```

**OpenClaw API 集成示例**:

```json
// 请求格式
POST /api/openClaw/process
{
    "message_id": "msg_12345",
    "user_message": "你好啊，八千代！",
    "user_name": "Visitor",
    "avatar_name": "八千代辉夜姬",
    "context": {
        "platform": "web",
        "mood": "friendly"
    }
}

// 响应格式
{
    "response_id": "resp_12345",
    "response_text": "哈哈，你好呀！今天心情怎么样？",
    "emotion_tags": ["开心", "热情"],
    "confidence_score": 0.95,
    "actions": [
        {
            "type": "expression",
            "value": "笑咪咪",
            "magnitude": 1.0,
            "duration_ms": 500,
            "easing": "ease-in-out"
        },
        {
            "type": "parameter",
            "value": "ParamMouthSmile",
            "magnitude": 0.9,
            "duration_ms": 800,
            "easing": "linear"
        },
        {
            "type": "parameter",
            "value": "ParamEyeLOpen",
            "magnitude": 1.0,
            "duration_ms": 1000,
            "easing": "ease-out"
        }
    ],
    "total_duration_ms": 1500
}
```

---

### 第四阶段: 文本翻译与语音合成

#### 4.1 中文→日语翻译

```cpp
// 新建文件: include/services/TranslationService.hpp

class TranslationService {
public:
    struct TranslationRequest {
        std::string sourceText;          // 中文文本
        std::string sourceLanguage;      // "zh-CN"
        std::string targetLanguage;      // "ja-JP"
        std::map<std::string, std::string> context;
    };
    
    struct TranslationResult {
        std::string translatedText;      // 日语文本
        float confidenceScore;           // 翻译置信度
        std::vector<std::string> alternativeTranslations;
        int64_t translationTime;
    };
    
    // 调用翻译 API (支持多个后端)
    TranslationResult translateToJapanese(
        const std::string& chineseText);
    
    // 支持的翻译引擎
    enum class TranslationEngine {
        GOOGLE_TRANSLATE,
        BAIDU_TRANSLATE,
        OPENAI_GPT,
        LOCAL_MODEL  // 本地离线模型
    };
    
private:
    TranslationEngine currentEngine;
    std::map<TranslationEngine, TranslationResult> translateCache;
};
```

**翻译示例**:

```
输入 (中文):  "哈哈，你好啊！今天天气真好！"
输出 (日語):  "ハハハ、こんにちは！今日の天気は本当にいいですね！"

处理步骤:
1. 分句: ["哈哈", "你好啊", "今天天气真好"]
2. 翻译: ["ハハハ", "こんにちは", "今日の天気がいい"]
3. 组合: "ハハハ、こんにちは！今日の天気は本当にいいですね！"
```

#### 4.2 GPT-SoVITS 语音合成

```cpp
// 新建文件: include/services/GPTSoVITSService.hpp

class GPTSoVITSService {
public:
    struct VoiceConfig {
        std::string referenceAudioPath;   // 参考音源路径
        float voiceScale;                 // 音量缩放 [0.5, 2.0]
        float voicePitch;                 // 音高偏移 [-24, 24]
        float voiceSpeed;                 // 速度缩放 [0.5, 2.0]
        std::string emotion;              // 情感 (happy, sad, neutral)
    };
    
    struct SynthesisRequest {
        std::string text;                 // 日语文本
        std::string language;             // "ja" (日本語)
        VoiceConfig voiceConfig;
        std::string outputFormat;         // "wav", "mp3", "ogg"
        int outputSampleRate;             // 44100, 48000
    };
    
    struct SynthesisResult {
        std::string audioPath;            // 生成的音频文件路径
        int audioLengthMs;                // 音频长度 (毫秒)
        float confidenceScore;            // 合成置信度
        std::string audioBase64;          // Base64 编码的音频
    };
    
    // 调用 GPT-SoVITS 合成语音
    SynthesisResult synthesizeJapaneseSpeech(
        const std::string& japaneseText,
        const VoiceConfig& voiceConfig);
    
    // 注册参考音源 (虚拟形象的"声音")
    bool registerReferenceAudio(
        const std::string& audioPath);
    
    // 调整音色参数
    void adjustVoiceParameters(
        const VoiceConfig& newConfig);
    
private:
    VoiceConfig currentVoiceConfig;
    std::string referenceAudioPath;      // 八千代辉夜姬的参考音源
    
    // 本地或远程 GPT-SoVITS API 调用
    SynthesisResult callGPTSoVITSAPI(
        const SynthesisRequest& request);
};
```

**GPT-SoVITS 配置示例**:

```cpp
// 初始化服务
GPTSoVITSService voiceService;

// 注册参考音源 (从用户提供)
voiceService.registerReferenceAudio(
    "/path/to/yachiyo_reference_voice.wav"
);

// 合成语音
GPTSoVITSService::VoiceConfig voiceConfig;
voiceConfig.referenceAudioPath = "/path/to/reference.wav";
voiceConfig.voiceScale = 1.2;      // 音量 +20%
voiceConfig.voicePitch = 3;        // 音高 +3 半音
voiceConfig.voiceSpeed = 1.0;      // 正常速度
voiceConfig.emotion = "happy";     // 开心的语气

auto result = voiceService.synthesizeJapaneseSpeech(
    "ハハハ、こんにちは！",
    voiceConfig
);

// 输出: 日语音频文件
LOG_INFO("生成音频: {} ({}ms)", 
         result.audioPath, 
         result.audioLengthMs);
```

---

### 第五阶段: Live2D 动作驱动

#### 5.1 参数驱动系统

```cpp
// 新建文件: include/services/Live2DAnimationService.hpp

class Live2DAnimationService {
public:
    struct AnimationKeyframe {
        int64_t timestampMs;            // 相对于动画开始的时间
        std::map<std::string, float> parameters;  // 参数值映射
        std::string easing;              // 过渡曲线
    };
    
    struct AnimationSequence {
        std::string sequenceId;
        std::vector<AnimationKeyframe> keyframes;
        int totalDurationMs;
        bool loop;                        // 是否循环
    };
    
    // 将 OpenClaw 的动作指令转换为 Live2D 参数
    AnimationSequence convertActionsToAnimation(
        const std::vector<OpenClawIntegration::ActionInstruction>& actions);
    
    // 播放动画序列
    void playAnimationSequence(const AnimationSequence& sequence);
    
    // 停止当前动画
    void stopAnimation();
    
    // 获取当前动画状态
    float getAnimationProgress() const;
    
private:
    std::unique_ptr<Models::Live2DModel> live2dModel;
    std::optional<AnimationSequence> currentAnimation;
    int64_t animationStartTime;
    
    // 补间计算
    float interpolateParameter(
        float startValue,
        float endValue,
        float progress,
        const std::string& easingFunction);
};
```

#### 5.2 表情与参数映射

```cpp
// 表情名 → Live2D 参数映射表

std::map<std::string, std::map<std::string, float>> ExpressionParameterMap = {
    {"笑咪咪", {
        {"ParamMouthSmile", 1.0f},
        {"ParamEyeLOpen", 0.5f},
        {"ParamEyeROpen", 0.5f},
        {"ParamBrowLY", 0.3f},
        {"ParamBrowRY", 0.3f}
    }},
    {"眯眯眼", {
        {"ParamEyeLOpen", 0.3f},
        {"ParamEyeROpen", 0.3f},
        {"ParamHeadAngleY", 15.0f},
        {"ParamMouthSmile", 0.5f}
    }},
    {"眼泪", {
        {"ParamBrowLY", -0.5f},
        {"ParamBrowRY", -0.5f},
        {"ParamMouthOpenY", -0.3f},
        {"ParamEyeLOpen", 0.7f},
        {"ParamEyeROpen", 0.7f}
    }},
    {"泪珠", {
        {"ParamEyeLOpen", 0.0f},
        {"ParamEyeROpen", 0.0f},
        {"ParamMouthOpenY", 1.0f},
        {"ParamBrowLY", -1.0f},
        {"ParamBrowRY", -1.0f}
    }}
};
```

---

### 第六阶段: 实时显示与同步

#### 6.1 前端实时更新

```cpp
// 新建文件: include/controllers/AvatarInteractionController.hpp

class AvatarInteractionController : public BaseController {
public:
    // WebSocket 端点: /ws/avatar/interaction
    static void handleWebSocketConnection(
        const std::shared_ptr<crow::websocket::connection>& conn);
    
    // 发送虚拟形象响应到客户端
    static void broadcastAvatarResponse(
        const AvatarResponseService::AvatarResponse& response);
    
private:
    static std::set<std::shared_ptr<crow::websocket::connection>> 
        activeConnections;
};
```

**WebSocket 消息格式**:

```json
// 从服务器发送到前端
{
    "type": "avatar_response",
    "message_id": "msg_12345",
    "response_id": "resp_12345",
    
    "text_part": {
        "chinese_text": "哈哈，你好呀！",
        "japanese_text": "ハハハ、こんにちは！",
        "expressions": ["😊", "开心"],
        "emotion_intensity": 0.9
    },
    
    "motion_part": {
        "expression": "笑咪咪",
        "parameters": {
            "ParamMouthSmile": 1.0,
            "ParamEyeLOpen": 0.5
        },
        "animation_duration": 1500,
        "keyframes": [
            {
                "timestamp": 0,
                "parameters": {"ParamMouthSmile": 0.0}
            },
            {
                "timestamp": 500,
                "parameters": {"ParamMouthSmile": 0.5}
            },
            {
                "timestamp": 1000,
                "parameters": {"ParamMouthSmile": 1.0}
            }
        ]
    },
    
    "audio": {
        "format": "wav",
        "sample_rate": 44100,
        "duration_ms": 3000,
        "url": "/api/audio/resp_12345.wav",
        "base64": "UklGRi4..."
    },
    
    "timing": {
        "response_time": 1743667200,
        "text_display_start": 0,
        "audio_start": 500,
        "animation_start": 200
    }
}
```

#### 6.2 前端 Vue 组件

```vue
<!-- YachiyoWeb/src/components/AvatarInteractionPanel.vue -->

<template>
  <div class="avatar-interaction-panel">
    <!-- 虚拟形象显示区域 -->
    <div class="avatar-display-section">
      <Live2DViewer 
        ref="live2dViewer"
        :model-path="avatarModelPath"
        :auto-animate="false"
      />
    </div>
    
    <!-- 实时聊天框 -->
    <div class="chat-section">
      <div class="message-list">
        <div 
          v-for="msg in messages" 
          :key="msg.id"
          class="message-item"
          :class="msg.isAvatarResponse ? 'avatar' : 'user'"
        >
          <!-- 用户消息 -->
          <div v-if="!msg.isAvatarResponse" class="user-message">
            <span class="user-name">{{ msg.userName }}</span>
            <p class="message-text">{{ msg.content }}</p>
          </div>
          
          <!-- 虚拟形象响应 -->
          <div v-else class="avatar-response">
            <span class="avatar-name">八千代辉夜姬</span>
            
            <!-- 中文文本显示 -->
            <p class="response-text-cn">{{ msg.textPart.chineseText }}</p>
            
            <!-- 日语文本显示 (小字) -->
            <p class="response-text-jp">{{ msg.textPart.japaneseText }}</p>
            
            <!-- 表情标记 -->
            <div class="emotion-tags">
              <span 
                v-for="emotion in msg.textPart.expressions" 
                :key="emotion"
                class="emotion-tag"
              >
                {{ emotion }}
              </span>
            </div>
            
            <!-- 音频播放器 -->
            <audio 
              v-if="msg.audio"
              :src="msg.audio.url"
              controls
              class="audio-player"
            ></audio>
          </div>
        </div>
      </div>
      
      <!-- 消息输入框 -->
      <div class="input-section">
        <textarea 
          v-model="newMessage"
          placeholder="输入你的消息... (已注册用户)"
          maxlength="500"
          rows="3"
          @keydown.enter.ctrl="sendMessage"
        ></textarea>
        <button @click="sendMessage" :disabled="!newMessage">
          发送消息
        </button>
        <span class="char-count">{{ newMessage.length }}/500</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import Live2DViewer from '@/components/Live2DViewer.vue'

interface Message {
  id: string
  isAvatarResponse: boolean
  userName?: string
  content?: string
  textPart?: {
    chineseText: string
    japaneseText: string
    expressions: string[]
  }
  audio?: {
    url: string
    format: string
  }
}

const avatarModelPath = "/yachiyo_live2d/八千代辉夜姬.model3.json"
const messages = ref<Message[]>([])
const newMessage = ref("")
const live2dViewer = ref()

let ws: WebSocket | null = null

// 初始化 WebSocket 连接
const initializeWebSocket = () => {
  const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws'
  ws = new WebSocket(`${protocol}://${window.location.host}/ws/avatar/interaction`)
  
  ws.onopen = () => {
    console.log("WebSocket 已连接")
  }
  
  ws.onmessage = (event) => {
    const data = JSON.parse(event.data)
    
    if (data.type === 'avatar_response') {
      handleAvatarResponse(data)
    }
  }
  
  ws.onerror = (error) => {
    console.error("WebSocket 错误:", error)
  }
}

// 处理虚拟形象响应
const handleAvatarResponse = async (data: any) => {
  // 1. 添加消息到聊天框
  messages.value.push({
    id: data.response_id,
    isAvatarResponse: true,
    textPart: {
      chineseText: data.text_part.chinese_text,
      japaneseText: data.text_part.japanese_text,
      expressions: data.text_part.expressions
    },
    audio: {
      url: data.audio.url,
      format: data.audio.format
    }
  })
  
  // 2. 应用虚拟形象动作
  const animationData = data.motion_part
  live2dViewer.value?.playAnimation(animationData)
  
  // 3. 播放音频
  const audioElement = new Audio(data.audio.url)
  audioElement.play()
  
  // 4. 自动滚动到底部
  scrollToBottom()
}

// 发送消息
const sendMessage = async () => {
  if (!newMessage.value.trim()) return
  
  const userMessage: Message = {
    id: `msg_${Date.now()}`,
    isAvatarResponse: false,
    userName: "您",
    content: newMessage.value
  }
  
  messages.value.push(userMessage)
  
  // 发送到服务器
  const response = await fetch('/api/messages/submit', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      content: newMessage.value,
      platform: 'web'
    })
  })
  
  if (response.ok) {
    newMessage.value = ""
    scrollToBottom()
  }
}

const scrollToBottom = () => {
  setTimeout(() => {
    const chatSection = document.querySelector('.message-list')
    if (chatSection) {
      chatSection.scrollTop = chatSection.scrollHeight
    }
  }, 0)
}

initializeWebSocket()
</script>

<style scoped>
.avatar-interaction-panel {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
  height: 100vh;
  padding: 20px;
}

.avatar-display-section {
  background: #000;
  border-radius: 8px;
  overflow: hidden;
}

.chat-section {
  display: flex;
  flex-direction: column;
  background: #f5f5f5;
  border-radius: 8px;
  padding: 20px;
}

.message-list {
  flex: 1;
  overflow-y: auto;
  margin-bottom: 20px;
}

.message-item {
  margin-bottom: 15px;
  padding: 12px;
  border-radius: 8px;
}

.message-item.user {
  background: #e3f2fd;
  text-align: right;
}

.message-item.avatar {
  background: #f3e5f5;
}

.user-message .user-name,
.avatar-response .avatar-name {
  font-weight: bold;
  display: block;
  margin-bottom: 5px;
}

.response-text-cn {
  font-size: 16px;
  color: #333;
  margin-bottom: 5px;
}

.response-text-jp {
  font-size: 12px;
  color: #666;
  margin-bottom: 8px;
}

.emotion-tags {
  display: flex;
  gap: 5px;
  margin-bottom: 10px;
  flex-wrap: wrap;
}

.emotion-tag {
  display: inline-block;
  padding: 3px 8px;
  background: #ddd;
  border-radius: 12px;
  font-size: 12px;
}

.audio-player {
  width: 100%;
  margin-top: 8px;
}

.input-section {
  display: flex;
  gap: 10px;
  align-items: flex-start;
}

.input-section textarea {
  flex: 1;
  padding: 10px;
  border: 1px solid #ddd;
  border-radius: 5px;
  font-size: 14px;
  font-family: sans-serif;
}

.input-section button {
  padding: 10px 20px;
  background: #2196F3;
  color: white;
  border: none;
  border-radius: 5px;
  cursor: pointer;
}

.input-section button:disabled {
  background: #ccc;
  cursor: not-allowed;
}

.char-count {
  font-size: 12px;
  color: #999;
}
</style>
```

---

## 🔐 安全性设计

### 文本验证流程

```
用户输入
  ↓
├─ 长度检查: 1-500 字符
├─ 编码验证: UTF-8
├─ XSS 防护: HTML 转义
├─ SQL 注入防护: 参数化查询
├─ 速率限制: 用户 1 分钟 < 10 条
├─ IP 检查: 黑名单验证
├─ 敏感词检查: 关键词过滤
├─ 启发式分析: 垃圾内容检测
├─ 行为分析: 用户模式检查
└─ 人工审核: Confidence < 0.7 标记

全部通过 ✅ → 存储到数据库
任何一项失败 ❌ → 拒绝并记录
```

### 内容审核配置

```cpp
// include/config/ContentReviewConfig.hpp

struct ContentReviewConfig {
    // Layer 1: 速率限制
    int maxMessagesPerMinute = 10;
    int maxMessagesPerHour = 100;
    
    // Layer 3: 敏感词库
    std::vector<std::string> sensitiveWords = {
        "违法", "政治敏感词", "商业竞争对手名称", ...
    };
    
    // Layer 4: 启发式规则
    float maxUppercaseRatio = 0.5f;      // 大写字母占比
    float maxPunctuationRatio = 0.3f;    // 标点符号占比
    bool blockUrlsInMessages = true;     // 阻止 URL
    bool blockEmailsInMessages = true;   // 阻止邮箱
    
    // Layer 6: 人工审核阈值
    float confidenceThreshold = 0.7f;
    bool requireManualReviewIfLow = true;
};
```

---

## 📊 数据库设计

### 表结构

```sql
-- 消息表
CREATE TABLE messages (
    id VARCHAR(50) PRIMARY KEY,
    user_id VARCHAR(50) NOT NULL,
    content TEXT NOT NULL,
    platform VARCHAR(20),
    status VARCHAR(20),  -- PENDING, APPROVED, REJECTED, PROCESSING
    risk_score FLOAT,
    created_at BIGINT,
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- 虚拟形象响应表
CREATE TABLE avatar_responses (
    id VARCHAR(50) PRIMARY KEY,
    message_id VARCHAR(50) NOT NULL,
    chinese_text TEXT NOT NULL,
    japanese_text TEXT NOT NULL,
    expressions TEXT,  -- JSON 数组
    emotion_intensity FLOAT,
    animation_parameters TEXT,  -- JSON 对象
    audio_path VARCHAR(255),
    duration_ms INT,
    created_at BIGINT,
    FOREIGN KEY (message_id) REFERENCES messages(id)
);

-- 虚拟形象动作表
CREATE TABLE avatar_actions (
    id VARCHAR(50) PRIMARY KEY,
    response_id VARCHAR(50) NOT NULL,
    action_type VARCHAR(50),  -- "expression", "parameter"
    action_value VARCHAR(100),
    magnitude FLOAT,
    duration_ms INT,
    easing VARCHAR(50),
    sequence_order INT,
    FOREIGN KEY (response_id) REFERENCES avatar_responses(id)
);

-- 音频文件表
CREATE TABLE audio_files (
    id VARCHAR(50) PRIMARY KEY,
    response_id VARCHAR(50),
    file_path VARCHAR(255),
    file_format VARCHAR(10),
    sample_rate INT,
    duration_ms INT,
    file_size BIGINT,
    created_at BIGINT,
    FOREIGN KEY (response_id) REFERENCES avatar_responses(id)
);
```

---

## 🚀 完整实现时间表

| 阶段 | 任务 | 工作量 | 周期 |
|------|------|--------|------|
| **第 1 阶段** | 用户输入与安全审核 | 8 小时 | 1 天 |
| **第 2 阶段** | 消息队列与处理 | 6 小时 | 1 天 |
| **第 3 阶段** | OpenClaw 集成 | 12 小时 | 2 天 |
| **第 4 阶段** | 翻译与语音合成 | 10 小时 | 2 天 |
| **第 5 阶段** | Live2D 动作驱动 | 10 小时 | 2 天 |
| **第 6 阶段** | 前端实时显示 | 12 小时 | 2 天 |
| **第 7 阶段** | 测试与优化 | 15 小时 | 2 天 |
| **第 8 阶段** | 部署与文档 | 7 小时 | 1 天 |
| **总计** | | **80 小时** | **2 周** |

---

## ✅ 集成检查清单

### 后端实现

- [ ] MessageServiceImpl - 6 层审核系统
- [ ] AvatarMessageQueueService - 消息队列
- [ ] AvatarResponseService - 虚拟形象处理
- [ ] OpenClawIntegration - OpenClaw API 集成
- [ ] TranslationService - 翻译服务
- [ ] GPTSoVITSService - 语音合成服务
- [ ] Live2DAnimationService - 动作驱动
- [ ] AvatarInteractionController - WebSocket 控制器
- [ ] 数据库表创建与迁移
- [ ] API 端点实现 (4 个)

### 前端实现

- [ ] Live2DViewer 组件 (WebGL 显示)
- [ ] AvatarInteractionPanel 面板
- [ ] 实时聊天框
- [ ] 消息输入框
- [ ] 音频播放器
- [ ] WebSocket 连接管理
- [ ] 动画播放控制

### 测试与优化

- [ ] 单元测试 (消息审核)
- [ ] 集成测试 (OpenClaw API)
- [ ] E2E 测试 (完整流程)
- [ ] 性能测试 (60 FPS Live2D)
- [ ] 安全测试 (输入验证)
- [ ] 压力测试 (并发消息)

### 部署

- [ ] Docker 容器化
- [ ] 参考音源配置
- [ ] GPT-SoVITS 模型部署
- [ ] OpenClaw 连接配置
- [ ] 翻译 API 密钥配置
- [ ] 生产数据库迁移
- [ ] 监控与日志

---

## 🎯 系统流程总结

```
用户 → 输入消息 → 安全审核 → 入队
                              ↓
                    虚拟形象处理 (OpenClaw)
                              ↓
                    ┌─────────┴──────────┐
                    ↓                    ↓
              文本输出              动作输出
          (中文 + 日语 + 表情)    (Live2D 参数)
                    │                    │
                    ↓                    ↓
              翻译 → TTS           应用动画
           (GPT-SoVITS)           (60 FPS)
                    │                    │
                    └─────────┬──────────┘
                              ↓
                    实时显示与用户交互
                 (文本 + 音频 + 虚拟形象)
```

---

**完整系统架构已设计完成！**
**预计工作量: 80 小时，工期: 2 周**

需要我继续详细实现某个特定阶段吗？
