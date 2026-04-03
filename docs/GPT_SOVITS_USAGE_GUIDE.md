# 🎤 GPT-SoVITS 语音合成指南

**版本**: 1.0  
**更新日期**: 2026年4月3日  
**用途**: Yachiyo 虚拱形象直播平台的感情化语音合成

---

## 📋 目录

1. [简介](#简介)
2. [部署](#部署)
3. [声音库](#声音库)
4. [请求格式](#请求格式)
5. [响应格式](#响应格式)
6. [感情参数](#感情参数)
7. [集成示例](#集成示例)
8. [性能优化](#性能优化)

---

## 简介

### GPT-SoVITS 是什么？

GPT-SoVITS 是一个先进的语音合成模型，支持：
- 🎙️ **声音克隆** - 使用少量样本克隆特定声音
- 😊 **感情控制** - 根据感情参数调整语调
- 🚀 **快速推理** - 本地部署，低延迟
- 📊 **多参数控制** - 速度、音调、能量可调

### 为什么用 GPT-SoVITS？

| 功能 | GPT-SoVITS | TTS.js | Edge-TTS |
|------|-----------|--------|---------|
| 本地部署 | ✅ | ✅ | ❌ |
| 声音克隆 | ✅ | ❌ | ❌ |
| 感情合成 | ✅ | ❌ | ❌ |
| 多语言 | ✅ | ✅ | ✅ |
| 质量 | 🌟🌟🌟🌟🌟 | 🌟🌟 | 🌟🌟🌟 |
| 成本 | 💰 (一次) | 💰 (一次) | 🆓 |

---

## 部署

### 🔵 Docker 部署 (推荐)

```bash
# 1. 拉取 GPT-SoVITS Docker 镜像
docker pull rongjie313/gpt-sovits:latest

# 2. 运行容器
docker run -d \
  --name gpt-sovits \
  -p 5000:5000 \
  -v /path/to/models:/app/models \
  -v /path/to/output:/app/output \
  -e TTS_DEVICE=cuda \
  rongjie313/gpt-sovits:latest

# 3. 检查服务状态
curl http://localhost:5000/health
```

### 📦 本地部署

```bash
# 1. 克隆项目
git clone https://github.com/RVC-Boss/GPT-SoVITS.git
cd GPT-SoVITS

# 2. 创建环境
conda create -n gpt-sovits python=3.9
conda activate gpt-sovits

# 3. 安装依赖
pip install -r requirements.txt
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118

# 4. 下载预训练模型
python download_models.py

# 5. 启动服务
python api_server.py --port 5000 --device cuda
```

### 🐳 Docker Compose 集成

```yaml
# docker-compose.yml (更新)

services:
  gpt-sovits:
    image: rongjie313/gpt-sovits:latest
    container_name: gpt-sovits-service
    ports:
      - "5000:5000"
    volumes:
      - ./resources/voice_models:/app/models:ro
      - ./resources/synthesized_audio:/app/output
      - ./resources/voice_samples:/app/voice_samples:ro
    environment:
      - TTS_DEVICE=cuda
      - FLASK_ENV=production
      - TTS_PORT=5000
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:5000/health"]
      interval: 30s
      timeout: 10s
      retries: 3
    networks:
      - yachiyo-network
    restart: always
```

---

## 声音库

### 声音库结构

```
resources/voice_models/
├── default/
│   ├── model.pth                    # 模型文件
│   ├── config.json                  # 配置
│   └── reference_voice.wav          # 参考音频
├── yachiyou_warm_jp.pth             # 预训练声音 (温暖日本音)
├── yachiyou_cool_jp.pth             # 预训练声音 (冷淡日本音)
├── yachiyou_playful_jp.pth          # 预训练声音 (调皮日本音)
└── custom_voices/
    ├── user_001_voice.pth
    ├── user_001_reference.wav
    ├── user_002_voice.pth
    └── user_002_reference.wav
```

### 预设声音配置

```json
{
  "preset_voices": {
    "yachiyou_default": {
      "model_path": "resources/voice_models/yachiyou_warm_jp.pth",
      "language": "ja",
      "speaker_id": 0,
      "emotion_baseline": "neutral",
      "pitch_range": [50, 300],
      "speed_range": [0.5, 2.0],
      "reference_voice": "resources/voice_models/default/reference_voice.wav"
    },
    "yachiyou_playful": {
      "model_path": "resources/voice_models/yachiyou_playful_jp.pth",
      "language": "ja",
      "speaker_id": 1,
      "emotion_baseline": "happy",
      "pitch_range": [80, 350],
      "speed_range": [0.6, 1.8]
    },
    "yachiyou_cool": {
      "model_path": "resources/voice_models/yachiyou_cool_jp.pth",
      "language": "ja",
      "speaker_id": 2,
      "emotion_baseline": "calm",
      "pitch_range": [40, 200],
      "speed_range": [0.7, 1.5]
    }
  }
}
```

### 声音克隆 (对新用户/新角色)

```bash
# 1. 收集参考音频 (5-30秒)
#    samples/reference_speaker_001.wav

# 2. 运行克隆脚本
python gpt_sovits_clone_voice.py \
  --reference_wav resources/voice_samples/reference_speaker.wav \
  --output_model resources/voice_models/custom_voices/new_speaker.pth \
  --num_epochs 100
```

---

## 请求格式

### 🔵 语音合成请求 JSON

```json
{
  "request_id": "tts_20260403_001",
  "text": "こんにちは、元気ですか？",
  "language": "ja",
  "voice": {
    "preset": "yachiyou_default",
    "speaker_id": 0
  },
  "emotion": {
    "emotion_type": "happy",
    "intensity": 0.8,
    "pitch_shift": 1.2,
    "speed_factor": 1.0,
    "energy_level": 0.9
  },
  "output": {
    "format": "wav",
    "sample_rate": 22050,
    "bit_depth": 16
  }
}
```

### 字段说明

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `request_id` | string | ✅ | 请求ID |
| `text` | string | ✅ | 要合成的文本 |
| `language` | string | ✅ | 语言代码 (ja/en/zh) |
| `voice` | object | ✅ | 声音配置 |
| `emotion` | object | ✅ | 感情参数 |
| `output` | object | ❌ | 输出配置 |

### voice 对象详解

```json
{
  "preset": "yachiyou_default",           // 预设声音
  "speaker_id": 0,                         // 说话人ID
  "reference_wav": null,                   // 自定义参考音频 (可选)
  "custom_model_path": null                // 自定义模型路径 (可选)
}
```

### emotion 对象详解

```json
{
  "emotion_type": "happy",                 // 感情类型 (见感情表)
  "intensity": 0.8,                        // 感情强度 [0.0-1.0]
  "pitch_shift": 1.2,                      // 音调偏移 [0.5-2.0]
  "speed_factor": 1.0,                     // 速度因子 [0.5-2.0]
  "energy_level": 0.9,                     // 能量等级 [0.0-1.0]
  "breathiness": 0.3,                      // 气音程度 [0.0-1.0] (可选)
  "tension": 0.5                           // 紧张度 [0.0-1.0] (可选)
}
```

---

## 响应格式

### 🟢 语音合成响应 JSON

```json
{
  "request_id": "tts_20260403_001",
  "status": "success",
  "data": {
    "audio_url": "http://localhost:5000/audio/tts_20260403_001.wav",
    "audio_base64": "UklGRiYAAABXQVZFZm10IBAAAAABAAE...",
    "duration_ms": 2450,
    "sample_rate": 22050,
    "file_path": "/app/output/tts_20260403_001.wav",
    "emotion_applied": "happy",
    "emotion_intensity": 0.8,
    "processing_time_ms": 1850,
    "cache_hit": false,
    "file_size_bytes": 107800
  },
  "error": null
}
```

### 响应字段说明

| 字段 | 说明 |
|------|------|
| `audio_url` | 可直接访问的音频URL |
| `audio_base64` | Base64编码的音频 (用于直接传输) |
| `duration_ms` | 音频时长 (毫秒) |
| `file_path` | 服务器上的文件路径 |
| `emotion_applied` | 实际应用的感情 |
| `processing_time_ms` | 处理耗时 |

---

## 感情参数

### 感情类型表

| 感情 | 中文 | 推荐参数 | 使用场景 |
|------|------|---------|---------|
| `happy` | 开心 | pitch↑ energy↑ | 积极回应、赞许 |
| `sad` | 伤心 | pitch↓ speed↓ | 同情、关心 |
| `angry` | 生气 | energy↑ tension↑ | 警告、责备 |
| `calm` | 平静 | baseline | 信息通知、说明 |
| `playful` | 调皮 | pitch↑ breathiness↑ | 开玩笑、娱乐 |
| `romantic` | 浪漫 | pitch∧ energy↓ | 甜蜜互动 |
| `confused` | 困惑 | energy↓ | 询问、反问 |
| `excited` | 兴奋 | pitch↑↑ energy↑↑ | 惊喜、庆祝 |

### 感情强度建议

```
intensity 0.0-0.3  → 微妙的感情渲染 (保留专业)
intensity 0.3-0.6  → 适中的感情表达 (推荐)
intensity 0.6-0.8  → 明显的感情倾向 (戏剧性)
intensity 0.8-1.0  → 极端的感情表达 (特殊场景)
```

### 参数推荐值

```json
{
  "emotions": {
    "happy": {
      "pitch_shift": [1.1, 1.5],
      "energy_level": [0.8, 1.0],
      "speed_factor": [1.0, 1.2],
      "breathiness": [0.1, 0.3]
    },
    "sad": {
      "pitch_shift": [0.6, 0.9],
      "energy_level": [0.3, 0.6],
      "speed_factor": [0.7, 0.9],
      "breathiness": [0.2, 0.4]
    },
    "calm": {
      "pitch_shift": [0.95, 1.05],
      "energy_level": [0.4, 0.6],
      "speed_factor": [0.9, 1.0],
      "breathiness": [0.1, 0.2]
    },
    "excited": {
      "pitch_shift": [1.3, 1.8],
      "energy_level": [0.9, 1.0],
      "speed_factor": [1.1, 1.4],
      "breathiness": [0.3, 0.5]
    }
  }
}
```

---

## 集成示例

### C++ 后端实现

```cpp
// backend/include/services/GPTSoVITSService.hpp

#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <optional>

using json = nlohmann::json;

class GPTSoVITSService {
public:
    struct SynthesisRequest {
        std::string requestId;
        std::string text;
        std::string language;
        std::string voicePreset;
        std::string emotionType;
        float emotionIntensity;
        float pitchShift;
        float speedFactor;
        float energyLevel;
    };
    
    struct SynthesisResult {
        bool success;
        std::string audioUrl;
        std::string audioBase64;
        int durationMs;
        std::string filePath;
        int processingTimeMs;
        bool cacheHit;
    };
    
    /**
     * 合成语音
     */
    SynthesisResult synthesize(const SynthesisRequest& request) {
        auto startTime = std::chrono::steady_clock::now();
        
        // 1. 检查缓存
        auto cacheKey = generateCacheKey(request);
        auto cached = getFromCache(cacheKey);
        if (cached) {
            LOG_DEBUG("使用缓存音频: {}", cacheKey);
            cached.value().cacheHit = true;
            return cached.value();
        }
        
        // 2. 构建请求 JSON
        json requestJson = buildSynthesisRequest(request);
        
        // 3. 调用 GPT-SoVITS API
        std::string response = callGPTSoVITSAPI(requestJson);
        
        // 4. 解析响应
        SynthesisResult result = parseSynthesisResponse(response);
        
        // 5. 保存到缓存
        if (result.success) {
            saveToCache(cacheKey, result);
        }
        
        auto endTime = std::chrono::steady_clock::now();
        result.processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        ).count();
        
        result.cacheHit = false;
        return result;
    }
    
    /**
     * 从 OpenClaw 响应直接合成
     */
    SynthesisResult synthesizeFromOpenClaw(
        const OpenClawResponse& openClawResponse,
        const std::string& voicePreset = "yachiyou_default"
    ) {
        // 1. 从 OpenClaw 响应提取感情
        auto emotion = mapOpenClawEmotionToTTSEmotion(openClawResponse.emotions);
        
        // 2. 合成语音
        SynthesisRequest request{
            .requestId = generateRequestId(),
            .text = openClawResponse.text,
            .language = "ja",
            .voicePreset = voicePreset,
            .emotionType = emotion.type,
            .emotionIntensity = emotion.intensity,
            .pitchShift = emotion.pitchShift,
            .speedFactor = emotion.speedFactor,
            .energyLevel = emotion.energyLevel
        };
        
        return synthesize(request);
    }
    
    /**
     * 批量合成 (多语言/多感情)
     */
    std::vector<SynthesisResult> batchSynthesize(
        const std::vector<SynthesisRequest>& requests
    ) {
        std::vector<std::future<SynthesisResult>> futures;
        
        for (const auto& request : requests) {
            futures.push_back(
                std::async(std::launch::async, [this, request]() {
                    return synthesize(request);
                })
            );
        }
        
        std::vector<SynthesisResult> results;
        for (auto& f : futures) {
            results.push_back(f.get());
        }
        
        return results;
    }

private:
    json buildSynthesisRequest(const SynthesisRequest& request) {
        json j;
        j["text"] = request.text;
        j["language"] = request.language;
        j["voice"]["preset"] = request.voicePreset;
        j["emotion"]["emotion_type"] = request.emotionType;
        j["emotion"]["intensity"] = request.emotionIntensity;
        j["emotion"]["pitch_shift"] = request.pitchShift;
        j["emotion"]["speed_factor"] = request.speedFactor;
        j["emotion"]["energy_level"] = request.energyLevel;
        
        return j;
    }
    
    std::string callGPTSoVITSAPI(const json& requestJson) {
        CURL* curl = curl_easy_init();
        std::string readBuffer;
        
        std::string url = "http://localhost:5000/synthesis";
        std::string postData = requestJson.dump();
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        CURLcode res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            LOG_ERROR("GPT-SoVITS API 调用失败: {}", curl_easy_strerror(res));
            return "";
        }
        
        return readBuffer;
    }
    
    struct EmotionMapping {
        std::string type;
        float intensity;
        float pitchShift;
        float speedFactor;
        float energyLevel;
    };
    
    EmotionMapping mapOpenClawEmotionToTTSEmotion(
        const std::vector<std::string>& openClawEmotions
    ) {
        // 简单映射示例
        EmotionMapping mapping;
        
        if (openClawEmotions.empty()) {
            mapping.type = "calm";
            mapping.intensity = 0.5;
        } else if (openClawEmotions[0] == "开心") {
            mapping.type = "happy";
            mapping.intensity = 0.8;
            mapping.pitchShift = 1.2;
            mapping.energyLevel = 0.9;
        } else if (openClawEmotions[0] == "伤心") {
            mapping.type = "sad";
            mapping.intensity = 0.6;
            mapping.pitchShift = 0.8;
            mapping.energyLevel = 0.4;
        } else if (openClawEmotions[0] == "生气") {
            mapping.type = "angry";
            mapping.intensity = 0.7;
            mapping.pitchShift = 1.1;
            mapping.energyLevel = 0.95;
        } else {
            mapping.type = "calm";
            mapping.intensity = 0.5;
        }
        
        return mapping;
    }
};
```

### 在虚拱响应中使用

```cpp
// src/services/AvatarResponseService.cpp

void AvatarResponseService::generateAvatarResponse(
    const ChatMessage& userMessage,
    const std::string& userId
) {
    // 1. 调用 OpenClaw 获取响应
    auto openClawResponse = openClawService->process(userMessage.content);
    
    // 2. 翻译 (如果需要)
    std::string displayText = openClawResponse.text;
    if (userLanguage != "zh") {
        auto translation = translationService->translate(
            openClawResponse.text,
            "zh",
            userLanguage
        );
        displayText = translation.translatedText;
    }
    
    // 3. 合成语音
    auto synthesisResult = soVITSService->synthesizeFromOpenClaw(
        openClawResponse,
        "yachiyou_default"
    );
    
    // 4. 获取 Live2D 动作
    auto live2dActions = getLive2DActionsFromEmotion(openClawResponse.emotions);
    
    // 5. 组织最终响应
    AvatarResponse response{
        .responseId = generateResponseId(),
        .text = displayText,
        .audio_url = synthesisResult.audioUrl,
        .emotions = openClawResponse.emotions,
        .actions = live2dActions,
        .duration_ms = synthesisResult.durationMs
    };
    
    // 6. 发送给前端
    sendWebSocketMessage(userId, response);
}
```

---

## 性能优化

### 缓存策略

```cpp
// 使用 Redis 缓存音频
std::string cacheKey = "audio:" + request.text + ":" + 
                       request.emotionType + ":" + request.voicePreset;

// 检查缓存
auto cached = redis->get(cacheKey);
if (cached) {
    LOG_DEBUG("音频缓存命中");
    return cached.value();
}

// 否则合成并缓存 (TTL: 7天)
auto result = synthesize(request);
redis->set(cacheKey, result.audioBase64, 604800);  // 7 days
```

### 并发优化

```cpp
// 使用线程池处理多个合成请求
std::vector<std::string> texts = {"你好", "谢谢", "再见"};
auto results = soVITSService->batchSynthesize(texts);
```

### 配置示例

```yaml
# backend/config/config.yaml

gpt_sovits:
  enabled: true
  
  # 服务配置
  server:
    endpoint: "http://localhost:5000"
    timeout_seconds: 30
    retry_attempts: 3
    retry_delay_ms: 100
  
  # 缓存配置
  cache:
    enabled: true
    backend: "redis"
    ttl_days: 7
    max_entries: 100000
  
  # 默认设置
  defaults:
    sample_rate: 22050
    bit_depth: 16
    format: "wav"
    voice_preset: "yachiyou_default"
    emotion_intensity: 0.6
    speed_factor: 1.0
  
  # 性能配置
  performance:
    max_concurrent: 5
    batch_size: 10
    use_gpu: true
```

---

## 常见问题

### Q: 如何克隆新的声音？
**A**: 收集5-30秒的参考音频，使用 `gpt_sovits_clone_voice.py` 脚本。

### Q: 是否支持实时流式输出？
**A**: 支持，使用流式 API: `/stream_synthesis`

### Q: 最快能多快生成音频？
**A**: GPU 上约 200-500ms，CPU 上 1-3 秒。

### Q: 可以离线使用吗？
**A**: 完全可以，所有模型都本地部署。

---

**参考链接**: https://github.com/RVC-Boss/GPT-SoVITS  
**更新日期**: 2026年4月3日
