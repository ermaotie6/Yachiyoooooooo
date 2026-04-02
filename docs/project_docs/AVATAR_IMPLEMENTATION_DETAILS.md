# 💻 虚拟形象交互系统 - 代码实现详解

> **模块**: Avatar Interaction System  
> **难度**: ⭐⭐⭐⭐ 复杂  
> **代码行数**: 3000+ 行  
> **创建日期**: 2026年4月2日

---

## 🏗️ 文件结构规划

```
YachiyoCPP/
├── include/
│   ├── services/
│   │   ├── AvatarMessageQueueService.hpp         ← 消息队列
│   │   ├── AvatarResponseService.hpp             ← 虚拟形象处理
│   │   ├── TranslationService.hpp                ← 翻译服务
│   │   ├── GPTSoVITSService.hpp                  ← 语音合成
│   │   └── Live2DAnimationService.hpp            ← 动作驱动
│   ├── models/
│   │   ├── OpenClawIntegration.hpp               ← OpenClaw API
│   │   └── AvatarResponseModel.hpp               ← 响应数据模型
│   └── controllers/
│       └── AvatarInteractionController.hpp       ← WebSocket 控制器
├── src/
│   ├── services/
│   │   ├── AvatarMessageQueueService.cpp
│   │   ├── AvatarResponseService.cpp
│   │   ├── TranslationService.cpp
│   │   ├── GPTSoVITSService.cpp
│   │   └── Live2DAnimationService.cpp
│   ├── models/
│   │   ├── OpenClawIntegration.cpp
│   │   └── AvatarResponseModel.cpp
│   └── controllers/
│       └── AvatarInteractionController.cpp
├── config/
│   └── avatar_config.yaml                       ← 虚拟形象配置
└── migrations/
    └── avatar_tables.sql                        ← 数据库迁移
```

---

## 1️⃣ 消息队列服务实现

### 文件: include/services/AvatarMessageQueueService.hpp

```cpp
#pragma once

#include <queue>
#include <map>
#include <mutex>
#include <optional>
#include <chrono>
#include "dto/MessageDTO.hpp"

namespace Services {

class AvatarMessageQueueService {
public:
    enum class MessageProcessingStatus {
        PENDING,           // 等待处理
        PROCESSING,        // 处理中
        WAITING_AUDIO,     // 等待音频生成
        COMPLETED,         // 已完成
        FAILED,            // 失败
        TIMEOUT            // 超时
    };
    
    struct QueuedMessage {
        std::string messageId;
        std::string userId;
        std::string userName;
        std::string content;
        int64_t timestamp;
        MessageProcessingStatus status;
        int retryCount;
        int64_t processingStartTime;
    };
    
    struct QueueStats {
        int pendingCount;
        int processingCount;
        int failedCount;
        float avgProcessingTime;
        int totalProcessed;
    };
    
    AvatarMessageQueueService();
    
    // 入队一条消息
    void enqueueMessage(const MessageDTO& message);
    
    // 获取下一条待处理消息
    std::optional<QueuedMessage> getNextMessage();
    
    // 标记消息为处理中
    void markAsProcessing(const std::string& messageId);
    
    // 标记消息为完成
    void markAsCompleted(const std::string& messageId);
    
    // 标记消息为失败 (可重试)
    void markAsFailed(const std::string& messageId);
    
    // 获取消息处理状态
    std::optional<MessageProcessingStatus> getMessageStatus(
        const std::string& messageId);
    
    // 获取队列统计信息
    QueueStats getQueueStats() const;
    
    // 清除超时消息 (默认 5 分钟)
    void clearTimeoutMessages(int timeoutSeconds = 300);
    
    // 重新加入失败的消息
    void requeueFailedMessages(int maxRetries = 3);
    
    // 获取队列大小
    size_t getQueueSize() const;
    
private:
    // 内存队列
    std::queue<QueuedMessage> messageQueue;
    
    // 处理中的消息映射 (messageId → QueuedMessage)
    std::map<std::string, QueuedMessage> processingMap;
    
    // 已完成的消息映射 (用于追踪)
    std::map<std::string, QueuedMessage> completedMap;
    
    // 失败的消息队列 (用于重试)
    std::queue<QueuedMessage> failedQueue;
    
    // 互斥锁
    mutable std::mutex queueMutex;
    mutable std::mutex processingMutex;
    mutable std::mutex statsMutex;
    
    // 统计数据
    struct Statistics {
        int totalProcessed = 0;
        int totalFailed = 0;
        int64_t totalProcessingTime = 0;
    } statistics;
    
    // 辅助函数
    bool isMessageTimeout(const QueuedMessage& msg, 
                         int timeoutSeconds) const;
};

}  // namespace Services
```

### 文件: src/services/AvatarMessageQueueService.cpp

```cpp
#include "services/AvatarMessageQueueService.hpp"
#include "utils/Logger.hpp"
#include <algorithm>
#include <ctime>

namespace Services {

AvatarMessageQueueService::AvatarMessageQueueService() = default;

void AvatarMessageQueueService::enqueueMessage(
    const MessageDTO& message) {
    
    std::lock_guard<std::mutex> lock(queueMutex);
    
    QueuedMessage queuedMsg;
    queuedMsg.messageId = message.id;
    queuedMsg.userId = message.userId;
    queuedMsg.userName = message.userName;
    queuedMsg.content = message.content;
    queuedMsg.timestamp = message.timestamp;
    queuedMsg.status = MessageProcessingStatus::PENDING;
    queuedMsg.retryCount = 0;
    queuedMsg.processingStartTime = 0;
    
    messageQueue.push(queuedMsg);
    
    LOG_INFO("消息已入队: {} (用户: {})", 
             message.id, message.userName);
}

std::optional<AvatarMessageQueueService::QueuedMessage>
AvatarMessageQueueService::getNextMessage() {
    
    std::lock_guard<std::mutex> lock(queueMutex);
    
    if (messageQueue.empty()) {
        return std::nullopt;
    }
    
    QueuedMessage nextMsg = messageQueue.front();
    messageQueue.pop();
    
    return nextMsg;
}

void AvatarMessageQueueService::markAsProcessing(
    const std::string& messageId) {
    
    std::lock_guard<std::mutex> lock(processingMutex);
    
    auto it = processingMap.find(messageId);
    if (it != processingMap.end()) {
        it->second.status = MessageProcessingStatus::PROCESSING;
        it->second.processingStartTime = 
            std::chrono::system_clock::now().time_since_epoch().count();
    }
    
    LOG_DEBUG("消息标记为处理中: {}", messageId);
}

void AvatarMessageQueueService::markAsCompleted(
    const std::string& messageId) {
    
    std::lock_guard<std::mutex> lock(processingMutex);
    
    auto it = processingMap.find(messageId);
    if (it != processingMap.end()) {
        it->second.status = MessageProcessingStatus::COMPLETED;
        
        // 计算处理时间
        int64_t processingTime = 
            std::chrono::system_clock::now().time_since_epoch().count() -
            it->second.processingStartTime;
        
        // 移到完成队列
        completedMap[messageId] = it->second;
        processingMap.erase(it);
        
        // 更新统计
        {
            std::lock_guard<std::mutex> statsLock(statsMutex);
            statistics.totalProcessed++;
            statistics.totalProcessingTime += processingTime;
        }
        
        LOG_INFO("消息已完成: {} (耗时: {}ms)", 
                 messageId, processingTime / 1000000);
    }
}

void AvatarMessageQueueService::markAsFailed(
    const std::string& messageId) {
    
    std::lock_guard<std::mutex> lock(processingMutex);
    
    auto it = processingMap.find(messageId);
    if (it != processingMap.end()) {
        it->second.status = MessageProcessingStatus::FAILED;
        it->second.retryCount++;
        
        if (it->second.retryCount < 3) {
            // 加入失败队列用于重试
            failedQueue.push(it->second);
            LOG_WARN("消息处理失败，已加入重试队列: {} (重试: {})", 
                     messageId, it->second.retryCount);
        } else {
            LOG_ERROR("消息处理失败超过重试次数: {}", messageId);
        }
        
        {
            std::lock_guard<std::mutex> statsLock(statsMutex);
            statistics.totalFailed++;
        }
        
        processingMap.erase(it);
    }
}

std::optional<AvatarMessageQueueService::MessageProcessingStatus>
AvatarMessageQueueService::getMessageStatus(
    const std::string& messageId) {
    
    std::lock_guard<std::mutex> lock(processingMutex);
    
    // 检查处理中的消息
    auto it = processingMap.find(messageId);
    if (it != processingMap.end()) {
        return it->second.status;
    }
    
    // 检查已完成的消息
    auto completedIt = completedMap.find(messageId);
    if (completedIt != completedMap.end()) {
        return completedIt->second.status;
    }
    
    return std::nullopt;
}

AvatarMessageQueueService::QueueStats
AvatarMessageQueueService::getQueueStats() const {
    
    std::lock_guard<std::mutex> statsLock(statsMutex);
    
    QueueStats stats;
    
    {
        std::lock_guard<std::mutex> queueLock(queueMutex);
        stats.pendingCount = messageQueue.size();
    }
    
    {
        std::lock_guard<std::mutex> procLock(processingMutex);
        stats.processingCount = processingMap.size();
        stats.failedCount = failedQueue.size();
    }
    
    stats.totalProcessed = statistics.totalProcessed;
    stats.avgProcessingTime = statistics.totalProcessed > 0 ?
        static_cast<float>(statistics.totalProcessingTime) / 
        statistics.totalProcessed / 1000000.0f : 0.0f;
    
    return stats;
}

void AvatarMessageQueueService::clearTimeoutMessages(
    int timeoutSeconds) {
    
    std::lock_guard<std::mutex> lock(processingMutex);
    
    std::vector<std::string> timeoutIds;
    
    for (auto& [msgId, msg] : processingMap) {
        if (isMessageTimeout(msg, timeoutSeconds)) {
            timeoutIds.push_back(msgId);
        }
    }
    
    for (const auto& msgId : timeoutIds) {
        LOG_WARN("消息超时已清除: {}", msgId);
        processingMap.erase(msgId);
    }
}

bool AvatarMessageQueueService::isMessageTimeout(
    const QueuedMessage& msg,
    int timeoutSeconds) const {
    
    int64_t currentTime = 
        std::chrono::system_clock::now().time_since_epoch().count();
    int64_t elapsedTime = currentTime - msg.processingStartTime;
    
    return elapsedTime > (int64_t)timeoutSeconds * 1000000000;
}

size_t AvatarMessageQueueService::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return messageQueue.size();
}

}  // namespace Services
```

---

## 2️⃣ 虚拟形象响应服务实现

### 文件: include/services/AvatarResponseService.hpp

```cpp
#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "models/OpenClawIntegration.hpp"
#include "services/TranslationService.hpp"
#include "services/GPTSoVITSService.hpp"
#include "services/Live2DAnimationService.hpp"

namespace Services {

class AvatarResponseService {
public:
    // 虚拟形象响应数据结构
    struct AvatarResponse {
        struct TextOutput {
            std::string responseId;
            std::string chineseText;
            std::string japaneseText;
            std::vector<std::string> expressions;
            float emotionIntensity;
            int64_t generatedTime;
        } textPart;
        
        struct MotionOutput {
            std::string expressionName;
            std::map<std::string, float> parameters;
            std::vector<Models::Live2DAnimationService::AnimationKeyframe> keyframes;
            int totalDurationMs;
            int64_t startTime;
        } motionPart;
        
        struct AudioOutput {
            std::string audioPath;
            std::string audioBase64;
            int durationMs;
            std::string format;
            int sampleRate;
        } audioPart;
        
        std::string responseId;
        std::string messageId;
        int64_t totalResponseTime;
    };
    
    AvatarResponseService();
    ~AvatarResponseService();
    
    // 初始化服务
    bool initialize();
    
    // 处理消息 (完整流程)
    AvatarResponse processMessage(
        const std::string& messageId,
        const std::string& userId,
        const std::string& userName,
        const std::string& messageContent);
    
    // 只生成文本响应
    TextOutput generateTextResponse(
        const std::string& messageContent);
    
    // 生成并播放完整响应
    AvatarResponse generateFullResponse(
        const std::string& messageContent,
        const std::string& userId);
    
private:
    std::shared_ptr<Models::OpenClawIntegration> openClawClient;
    std::shared_ptr<TranslationService> translationService;
    std::shared_ptr<GPTSoVITSService> voiceService;
    std::shared_ptr<Live2DAnimationService> animationService;
    
    // 将 OpenClaw 输出转换为文本响应
    TextOutput convertOpenClawToText(
        const Models::OpenClawIntegration::OpenClawResponse& response);
    
    // 将 OpenClaw 输出转换为动作指令
    MotionOutput convertOpenClawToMotion(
        const Models::OpenClawIntegration::OpenClawResponse& response);
    
    // 生成音频
    AudioOutput generateAudio(
        const std::string& japaneseText,
        const std::vector<std::string>& emotions);
    
    // 保存响应到数据库
    void persistResponse(const AvatarResponse& response);
    
    // 生成响应 ID
    std::string generateResponseId();
};

}  // namespace Services
```

### 文件: src/services/AvatarResponseService.cpp

```cpp
#include "services/AvatarResponseService.hpp"
#include "utils/Logger.hpp"
#include "utils/UUIDUtil.hpp"
#include <chrono>

namespace Services {

AvatarResponseService::AvatarResponseService() = default;

AvatarResponseService::~AvatarResponseService() = default;

bool AvatarResponseService::initialize() {
    LOG_INFO("初始化虚拟形象响应服务");
    
    // 初始化各个子服务
    openClawClient = std::make_shared<
        Models::OpenClawIntegration>();
    
    translationService = std::make_shared<
        TranslationService>();
    
    voiceService = std::make_shared<
        GPTSoVITSService>();
    
    animationService = std::make_shared<
        Live2DAnimationService>();
    
    if (!voiceService->registerReferenceAudio(
        "/path/to/yachiyo_reference_voice.wav")) {
        LOG_WARN("参考音源加载失败");
        // 继续运行，但没有语音合成
    }
    
    LOG_INFO("虚拟形象响应服务初始化完成");
    return true;
}

AvatarResponseService::AvatarResponse 
AvatarResponseService::processMessage(
    const std::string& messageId,
    const std::string& userId,
    const std::string& userName,
    const std::string& messageContent) {
    
    int64_t startTime = 
        std::chrono::system_clock::now().time_since_epoch().count();
    
    LOG_INFO("处理消息: {} (用户: {})", messageId, userName);
    
    AvatarResponse response;
    response.responseId = generateResponseId();
    response.messageId = messageId;
    
    try {
        // Step 1: 调用 OpenClaw 处理
        Models::OpenClawIntegration::OpenClawRequest request;
        request.messageId = messageId;
        request.userMessage = messageContent;
        request.userName = userName;
        request.avatarName = "八千代辉夜姬";
        
        auto openClawResponse = openClawClient->sendToOpenClaw(request);
        LOG_DEBUG("OpenClaw 响应: {}", openClawResponse.responseId);
        
        // Step 2: 转换为文本输出
        response.textPart = convertOpenClawToText(openClawResponse);
        
        // Step 3: 转换为动作输出
        response.motionPart = convertOpenClawToMotion(openClawResponse);
        
        // Step 4: 翻译并生成语音
        auto translationResult = translationService->translateToJapanese(
            response.textPart.chineseText);
        response.textPart.japaneseText = 
            translationResult.translatedText;
        
        response.audioPart = generateAudio(
            response.textPart.japaneseText,
            response.textPart.expressions);
        
        // Step 5: 保存响应
        persistResponse(response);
        
        int64_t endTime = 
            std::chrono::system_clock::now().time_since_epoch().count();
        response.totalResponseTime = 
            (endTime - startTime) / 1000000;  // 转换为毫秒
        
        LOG_INFO("消息处理完成: {} (耗时: {}ms)", 
                 messageId, response.totalResponseTime);
        
    } catch (const std::exception& e) {
        LOG_ERROR("处理消息失败: {}", e.what());
        throw;
    }
    
    return response;
}

AvatarResponseService::TextOutput
AvatarResponseService::convertOpenClawToText(
    const Models::OpenClawIntegration::OpenClawResponse& response) {
    
    TextOutput textOutput;
    textOutput.responseId = response.responseId;
    textOutput.chineseText = response.responseText;
    textOutput.expressions = response.emotionTags;
    textOutput.emotionIntensity = response.confidenceScore;
    textOutput.generatedTime = 
        std::chrono::system_clock::now().time_since_epoch().count();
    
    return textOutput;
}

AvatarResponseService::MotionOutput
AvatarResponseService::convertOpenClawToMotion(
    const Models::OpenClawIntegration::OpenClawResponse& response) {
    
    MotionOutput motionOutput;
    motionOutput.totalDurationMs = response.totalDurationMs;
    motionOutput.startTime = 
        std::chrono::system_clock::now().time_since_epoch().count();
    
    // 将 OpenClaw 的动作指令转换为 Live2D 动画
    auto animation = animationService->convertActionsToAnimation(
        response.actions);
    
    motionOutput.keyframes = animation.keyframes;
    
    return motionOutput;
}

AvatarResponseService::AudioOutput
AvatarResponseService::generateAudio(
    const std::string& japaneseText,
    const std::vector<std::string>& emotions) {
    
    AudioOutput audioOutput;
    
    // 根据情感调整音色
    GPTSoVITSService::VoiceConfig voiceConfig;
    voiceConfig.emotion = emotions.empty() ? 
        "neutral" : emotions[0];
    
    // 调整参数 (示例)
    if (voiceConfig.emotion == "开心" || 
        voiceConfig.emotion == "happy") {
        voiceConfig.voiceScale = 1.2f;
        voiceConfig.voicePitch = 3.0f;
    } else if (voiceConfig.emotion == "难过" || 
               voiceConfig.emotion == "sad") {
        voiceConfig.voiceScale = 0.8f;
        voiceConfig.voicePitch = -3.0f;
    }
    
    // 合成语音
    auto synthesisResult = 
        voiceService->synthesizeJapaneseSpeech(
            japaneseText, voiceConfig);
    
    audioOutput.audioPath = synthesisResult.audioPath;
    audioOutput.audioBase64 = synthesisResult.audioBase64;
    audioOutput.durationMs = synthesisResult.audioLengthMs;
    audioOutput.format = "wav";
    audioOutput.sampleRate = 44100;
    
    return audioOutput;
}

void AvatarResponseService::persistResponse(
    const AvatarResponse& response) {
    
    // TODO: 保存到数据库
    LOG_DEBUG("保存响应到数据库: {}", response.responseId);
}

std::string AvatarResponseService::generateResponseId() {
    return "resp_" + Utils::UUIDUtil::generateUUID();
}

}  // namespace Services
```

---

## 3️⃣ OpenClaw 集成实现

### 文件: include/models/OpenClawIntegration.hpp

```cpp
#pragma once

#include <string>
#include <vector>
#include <map>
#include <json/json.h>

namespace Models {

class OpenClawIntegration {
public:
    struct ActionInstruction {
        std::string type;              // "expression" 或 "parameter"
        std::string value;             // 表情名或参数 ID
        float magnitude;               // 值或强度 [0, 1]
        int durationMs;                // 时长 (毫秒)
        std::string easing;            // 过渡曲线
    };
    
    struct OpenClawRequest {
        std::string messageId;
        std::string userMessage;
        std::string userName;
        std::string avatarName;
        std::map<std::string, std::string> context;
    };
    
    struct OpenClawResponse {
        std::string responseId;
        std::string responseText;
        std::vector<std::string> emotionTags;
        float confidenceScore;
        std::vector<ActionInstruction> actions;
        int totalDurationMs;
    };
    
    OpenClawIntegration();
    
    // 发送请求到 OpenClaw
    OpenClawResponse sendToOpenClaw(
        const OpenClawRequest& request);
    
    // 连接到 OpenClaw
    bool connect(const std::string& apiEndpoint,
                const std::string& apiKey);
    
    // 检查连接状态
    bool isConnected() const;
    
private:
    std::string apiEndpoint;
    std::string apiKey;
    bool connected;
    
    // HTTP 请求
    std::string sendHTTPRequest(const std::string& endpoint,
                               const Json::Value& payload);
    
    // 解析响应
    void parseResponse(const std::string& rawResponse,
                     OpenClawResponse& outResponse);
};

}  // namespace Models
```

---

## 4️⃣ 翻译服务实现

### 文件: include/services/TranslationService.hpp

```cpp
#pragma once

#include <string>
#include <memory>
#include <map>

namespace Services {

class TranslationService {
public:
    struct TranslationResult {
        std::string translatedText;
        float confidenceScore;
        int64_t translationTime;
    };
    
    enum class TranslationEngine {
        GOOGLE_TRANSLATE,
        BAIDU_TRANSLATE,
        OPENAI_GPT,
        LOCAL_MODEL
    };
    
    TranslationService();
    
    // 翻译中文为日语
    TranslationResult translateToJapanese(
        const std::string& chineseText);
    
    // 设置翻译引擎
    void setTranslationEngine(TranslationEngine engine);
    
private:
    TranslationEngine currentEngine;
    std::map<std::string, TranslationResult> translateCache;
    
    // 各个引擎的实现
    TranslationResult googleTranslate(const std::string& text);
    TranslationResult baiduTranslate(const std::string& text);
    TranslationResult openaiTranslate(const std::string& text);
    TranslationResult localModelTranslate(const std::string& text);
};

}  // namespace Services
```

---

## 5️⃣ GPT-SoVITS 语音服务实现

### 文件: include/services/GPTSoVITSService.hpp

```cpp
#pragma once

#include <string>
#include <memory>

namespace Services {

class GPTSoVITSService {
public:
    struct VoiceConfig {
        std::string referenceAudioPath;
        float voiceScale;              // [0.5, 2.0]
        float voicePitch;              // [-24, 24]
        float voiceSpeed;              // [0.5, 2.0]
        std::string emotion;           // "happy", "sad", "neutral"
    };
    
    struct SynthesisRequest {
        std::string text;
        std::string language;
        VoiceConfig voiceConfig;
        std::string outputFormat;
        int outputSampleRate;
    };
    
    struct SynthesisResult {
        std::string audioPath;
        int audioLengthMs;
        float confidenceScore;
        std::string audioBase64;
    };
    
    GPTSoVITSService();
    
    // 初始化 GPT-SoVITS 服务
    bool initialize(const std::string& modelPath,
                   const std::string& apiEndpoint);
    
    // 注册参考音源
    bool registerReferenceAudio(
        const std::string& audioPath);
    
    // 合成日语语音
    SynthesisResult synthesizeJapaneseSpeech(
        const std::string& japaneseText,
        const VoiceConfig& voiceConfig);
    
    // 调整音色参数
    void adjustVoiceParameters(const VoiceConfig& config);
    
private:
    VoiceConfig currentVoiceConfig;
    std::string modelPath;
    std::string apiEndpoint;
    std::string referenceAudioPath;
    
    // 调用 GPT-SoVITS API
    SynthesisResult callGPTSoVITSAPI(
        const SynthesisRequest& request);
    
    // 本地推理 (如果部署本地模型)
    SynthesisResult localInference(
        const SynthesisRequest& request);
};

}  // namespace Services
```

---

## 6️⃣ Live2D 动画服务实现

### 文件: include/services/Live2DAnimationService.hpp

```cpp
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "models/Live2DModel.hpp"
#include "models/OpenClawIntegration.hpp"

namespace Services {

class Live2DAnimationService {
public:
    struct AnimationKeyframe {
        int64_t timestampMs;
        std::map<std::string, float> parameters;
        std::string easing;
    };
    
    struct AnimationSequence {
        std::string sequenceId;
        std::vector<AnimationKeyframe> keyframes;
        int totalDurationMs;
        bool loop;
    };
    
    Live2DAnimationService();
    
    // 加载 Live2D 模型
    bool loadModel(const std::string& modelPath);
    
    // 转换 OpenClaw 动作指令为动画序列
    AnimationSequence convertActionsToAnimation(
        const std::vector<Models::OpenClawIntegration::ActionInstruction>& actions);
    
    // 播放动画序列
    void playAnimationSequence(const AnimationSequence& sequence);
    
    // 停止当前动画
    void stopAnimation();
    
    // 获取动画进度
    float getAnimationProgress() const;
    
    // 更新动画 (每帧调用)
    void update(float deltaTime);
    
    // 渲染当前帧
    void render();
    
private:
    std::unique_ptr<Models::Live2DModel> live2dModel;
    std::optional<AnimationSequence> currentAnimation;
    int64_t animationStartTime;
    float currentProgress;
    
    // 补间函数
    float easeLinear(float t);
    float easeInOutQuad(float t);
    float easeOutQuad(float t);
    float interpolateParameter(
        float startValue, float endValue, float progress,
        const std::string& easing);
    
    // 从表情名获取参数
    std::map<std::string, float> getExpressionParameters(
        const std::string& expressionName);
};

}  // namespace Services
```

---

## 7️⃣ WebSocket 控制器实现

### 文件: include/controllers/AvatarInteractionController.hpp

```cpp
#pragma once

#include <set>
#include <memory>
#include "BaseController.hpp"
#include "services/AvatarResponseService.hpp"
#include "services/AvatarMessageQueueService.hpp"

namespace Controllers {

class AvatarInteractionController : public BaseController {
public:
    static void initialize(
        std::shared_ptr<Services::AvatarResponseService> responseService,
        std::shared_ptr<Services::AvatarMessageQueueService> queueService);
    
    // WebSocket 连接处理
    static void handleWebSocketConnection(
        const std::shared_ptr<crow::websocket::connection>& conn);
    
    // 广播虚拟形象响应
    static void broadcastAvatarResponse(
        const Services::AvatarResponseService::AvatarResponse& response);
    
private:
    static std::set<std::shared_ptr<crow::websocket::connection>> 
        activeConnections;
    static std::shared_ptr<Services::AvatarResponseService> 
        responseService;
    static std::shared_ptr<Services::AvatarMessageQueueService> 
        queueService;
};

}  // namespace Controllers
```

---

## 🔄 完整调用流程

```cpp
// 1. 接收用户消息
POST /api/messages/submit
{
    "content": "你好啊，八千代！"
}

// 2. 消息通过 6 层审核
MessageServiceImpl::validateAndStoreMessage()

// 3. 消息入队
AvatarMessageQueueService::enqueueMessage()

// 4. 虚拟形象处理线程获取消息
AvatarMessageQueueService::getNextMessage()

// 5. 处理消息
AvatarResponseService::processMessage()
  ├─ OpenClaw API 处理
  ├─ 翻译为日语
  ├─ 生成语音 (GPT-SoVITS)
  └─ 生成动作指令

// 6. 发送 WebSocket 消息给客户端
AvatarInteractionController::broadcastAvatarResponse()

// 7. 前端显示
┌─────────────────────┐
│ 文本 (中文+日语)    │
│ 表情标记            │
│ 虚拟形象动作        │
│ 语音播放            │
└─────────────────────┘
```

---

## ✅ 实现清单

- [ ] 消息队列服务 (2000+ 行)
- [ ] 虚拟形象响应服务 (1500+ 行)
- [ ] OpenClaw 集成 (1000+ 行)
- [ ] 翻译服务 (800+ 行)
- [ ] GPT-SoVITS 服务 (800+ 行)
- [ ] Live2D 动画服务 (1200+ 行)
- [ ] WebSocket 控制器 (600+ 行)
- [ ] 数据库表和迁移
- [ ] 前端 Vue 组件

---

**完整代码框架已设计完成！**
**下一步: 逐个实现每个服务类**
