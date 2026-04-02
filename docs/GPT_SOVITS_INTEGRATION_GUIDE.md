# 🎙️ GPT-SoVITS 语音合成集成指南

**版本**: 1.0  
**更新时间**: 2026-04-02  
**适用范围**: 虚拱语音合成、情感语音、多语言语音

---

## 📑 目录

1. [GPT-SoVITS 简介](#gpt-sovits-简介)
2. [系统架构](#系统架构)
3. [安装和配置](#安装和配置)
4. [API 集成](#api-集成)
5. [声音克隆](#声音克隆)
6. [性能优化](#性能优化)
7. [故障排查](#故障排查)

---

## GPT-SoVITS 简介

### 什么是 GPT-SoVITS?

GPT-SoVITS 是一个**强大的文本转语音 (TTS) 框架**，具有以下特点：

```
GPT-SoVITS = GPT-4级别语言理解 + SoVITS质量语音合成
```

**主要特性**:
- 🎯 **高质量语音合成** - 自然流畅，情感丰富
- 🌍 **多语言支持** - 日语、中文、英文等
- 🎨 **声音克隆** - 少量语音样本即可克隆
- ⚡ **低延迟** - GPU 加速，实时处理
- 📊 **情感控制** - 调整语速、音调、情感
- 🔧 **易于集成** - Python API 和 REST 端点

### 对标对比

| 特性 | GPT-SoVITS | 谷歌 TTS | Azure TTS | 本地 TacotronNLP |
|------|-----------|---------|----------|----------------|
| 质量 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| 延迟 | 低 (200ms) | 中 (1s) | 中 (1s) | 低 (100ms) |
| 多语言 | 是 | 是 | 是 | 部分 |
| 声音克隆 | 高效 | 否 | 否 | 否 |
| 情感 | 丰富 | 基础 | 基础 | 否 |
| 成本 | 免费 | 按量计费 | 按量计费 | 免费 |
| 部署 | 本地/云 | 云 | 云 | 本地 |

---

## 系统架构

### Yachiyo 中的语音合成流程

```
┌─────────────────────────────────────────────────────────┐
│                 用户输入文本                             │
│          (来自前端或 OpenClaw)                           │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────▼────────────┐
        │  OpenClaw 智能处理       │
        │  - 上下文理解           │
        │  - 情感判断           │
        │  - 语速调整           │
        └────────────┬─────────────┘
                     │
        ┌────────────▼────────────┐
        │  GPT-SoVITS 处理        │
        │  - 文本分析           │
        │  - 韵律生成           │
        │  - 语音合成           │
        └────────────┬──────────────┘
                     │
        ┌────────────▼────────────┐
        │  音频后处理             │
        │  - 降噪                │
        │  - 音量标准化          │
        │  - 格式转换            │
        └────────────┬────────────┘
                     │
        ┌────────────▼────────────┐
        │  缓存和分发             │
        │  - Redis 缓存          │
        │  - WebSocket 流传输    │
        │  - 浏览器播放          │
        └────────────────────────┘
```

### 后端集成架构

```cpp
┌─────────────────────────────────────────┐
│       Application Layer                 │
│       (聊天、消息处理)                   │
└────────────────┬────────────────────────┘
                 │
         ┌───────▼───────┐
         │  OpenClaw     │ ◄─── 统一管理所有 AI 逻辑
         │  Framework    │     - 对话生成
         │               │     - 情感分析
         │               │     - 语音参数
         └───────┬───────┘
                 │
    ┌────────────▼──────────────┐
    │  VoiceSynthesisService    │
    │  (语音合成服务)            │
    │                           │
    │  ┌─────────────────────┐  │
    │  │ GPT-SoVITS 调用     │  │
    │  │ - 模型推理          │  │
    │  │ - 参数传递          │  │
    │  │ - 音频生成          │  │
    │  └──────────┬──────────┘  │
    └─────────────┬──────────────┘
                  │
    ┌─────────────▼──────────────┐
    │  AudioProcessing           │
    │  - 编码 (MP3/WAV)          │
    │  - 缓存                    │
    └─────────────┬──────────────┘
                  │
         ┌────────▼────────┐
         │  WebSocket API  │
         │  (实时推送)     │
         └─────────────────┘
```

---

## 安装和配置

### 系统要求

```
硬件:
  - GPU: NVIDIA A100 或 RTX 3090 (推荐)
  - 内存: 16GB+ (GPU 显存 + 系统内存)
  - 存储: 50GB (模型文件 + 缓存)
  - CPU: 8核+

软件:
  - Python: 3.9 - 3.11
  - CUDA: 11.8+
  - cuDNN: 8.6+
  - PyTorch: 2.0+
```

### 安装步骤

#### 1. 克隆 GPT-SoVITS 仓库

```bash
# 获取 GPT-SoVITS
git clone https://github.com/RVC-Boss/GPT-SoVITS.git
cd GPT-SoVITS

# 创建虚拟环境
python -m venv venv
source venv/bin/activate  # Linux/Mac
# 或
venv\Scripts\activate  # Windows
```

#### 2. 安装依赖

```bash
# 安装 PyTorch (with CUDA)
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118

# 安装 GPT-SoVITS
pip install -r requirements.txt

# 安装额外依赖
pip install librosa scipy numpy pydantic
```

#### 3. 下载预训练模型

```bash
# 创建模型目录
mkdir -p models/gpt_sovits

# 下载主模型
# 从官方仓库下载或使用脚本
# https://github.com/RVC-Boss/GPT-SoVITS#models

# 模型结构:
# models/
# ├── gpt_sovits/
# │   ├── gpt_sovits.pth      # GPT 模型
# │   ├── vits.pth            # VITS 声码器
# │   └── config.json         # 配置文件
# └── hubert/
#     └── hubert_base.pt      # HuBERT 编码器
```

#### 4. 在 Yachiyo 中配置

```bash
# 后端配置
cd backend

# 创建配置文件
cat > config/gpt_sovits_config.yaml << EOF
gpt_sovits:
  model_path: "/path/to/models/gpt_sovits"
  device: "cuda:0"  # 或 "cpu"
  sample_rate: 22050
  
  # 推理参数
  inference:
    top_p: 0.6
    temperature: 0.7
    max_length: 2048
    
  # 语音参数
  voice:
    speed: 1.0          # 语速倍数 (0.5 - 2.0)
    pitch: 0.0          # 音调偏移 (-12 - +12)
    energy: 1.0         # 能量强度 (0.5 - 2.0)
    
  # 缓存配置
  cache:
    enabled: true
    max_size: 1000      # 最多缓存 1000 个合成结果
    ttl: 86400          # 24 小时过期

  # 服务器配置
  server:
    host: "127.0.0.1"
    port: 9880
    workers: 2
EOF
```

### 启动 GPT-SoVITS 服务

#### 方案 A: 独立服务器运行

```bash
#!/bin/bash
# start_gpt_sovits.sh

cd /path/to/GPT-SoVITS

# 激活虚拟环境
source venv/bin/activate

# 启动 Flask 服务器
python api_server.py \
  --host 0.0.0.0 \
  --port 9880 \
  --model_path models/gpt_sovits \
  --device cuda:0 \
  --workers 2
```

#### 方案 B: Docker 容器运行

```dockerfile
# Dockerfile.gpt_sovits
FROM nvidia/cuda:11.8.0-runtime-ubuntu22.04

WORKDIR /app

# 安装依赖
RUN apt-get update && apt-get install -y \
    python3.10 python3-pip git \
    && rm -rf /var/lib/apt/lists/*

# 克隆仓库
RUN git clone https://github.com/RVC-Boss/GPT-SoVITS.git .

# 安装 Python 依赖
RUN pip install --no-cache-dir torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118
RUN pip install --no-cache-dir -r requirements.txt

# 暴露端口
EXPOSE 9880

# 启动脚本
CMD ["python", "api_server.py", "--host", "0.0.0.0", "--port", "9880"]
```

启动容器:
```bash
docker run -d \
  --name gpt-sovits \
  --gpus all \
  -p 9880:9880 \
  -v /path/to/models:/app/models \
  gpt-sovits:latest
```

---

## API 集成

### C++ 后端集成

#### 1. 创建语音合成客户端

```cpp
// src/services/VoiceSynthesisService.cpp

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "VoiceSynthesisService.hpp"

using json = nlohmann::json;

class GPTSoVITSClient {
private:
    std::string sovits_url;
    std::string model_id;
    int cache_ttl;
    
public:
    GPTSoVITSClient(const std::string& url, const std::string& model)
        : sovits_url(url), model_id(model), cache_ttl(86400) {}
    
    /**
     * 合成语音
     * @param text 输入文本
     * @param speaker_id 说话人 ID
     * @param params 语音参数 (速度、音调等)
     * @return 音频 Base64 编码
     */
    std::string synthesize(
        const std::string& text,
        const std::string& speaker_id,
        const VoiceParams& params
    ) {
        // 1. 检查缓存
        std::string cache_key = generateCacheKey(text, speaker_id, params);
        auto cached = redis_cache.get(cache_key);
        if (cached) {
            LOG_INFO("从缓存返回音频: {}", cache_key);
            return *cached;
        }
        
        // 2. 调用 GPT-SoVITS API
        json request = {
            {"text", text},
            {"speaker_id", speaker_id},
            {"params", {
                {"speed", params.speed},
                {"pitch", params.pitch},
                {"energy", params.energy}
            }}
        };
        
        std::string response = callGPTSoVITS(request.dump());
        
        // 3. 解析响应
        json resp_json = json::parse(response);
        if (resp_json["status"] != "success") {
            LOG_ERROR("GPT-SoVITS 合成失败: {}", resp_json["error"]);
            throw std::runtime_error("语音合成失败");
        }
        
        std::string audio_base64 = resp_json["audio"];
        
        // 4. 缓存结果
        redis_cache.set(cache_key, audio_base64, cache_ttl);
        
        return audio_base64;
    }
    
    /**
     * 克隆声音
     * @param voice_samples 语音样本列表
     * @param speaker_name 新说话人名称
     * @return 说话人 ID
     */
    std::string cloneVoice(
        const std::vector<std::string>& voice_samples,
        const std::string& speaker_name
    ) {
        json request = {
            {"action", "clone_voice"},
            {"speaker_name", speaker_name},
            {"samples", voice_samples}
        };
        
        std::string response = callGPTSoVITS(request.dump());
        json resp_json = json::parse(response);
        
        if (resp_json["status"] != "success") {
            throw std::runtime_error("声音克隆失败");
        }
        
        return resp_json["speaker_id"];
    }
    
private:
    std::string callGPTSoVITS(const std::string& request_body) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("CURL 初始化失败");
        }
        
        std::string response;
        std::string url = sovits_url + "/api/synthesize";
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        // 写回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, 
            [](void* contents, size_t size, size_t nmemb, void* userp) {
                auto* buf = static_cast<std::string*>(userp);
                buf->append((char*)contents, size * nmemb);
                return size * nmemb;
            }
        );
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            throw std::runtime_error("请求失败: " + std::string(curl_easy_strerror(res)));
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return response;
    }
    
    std::string generateCacheKey(
        const std::string& text,
        const std::string& speaker_id,
        const VoiceParams& params
    ) {
        std::string key = speaker_id + "_" + text + "_" + 
                         std::to_string(params.speed) + "_" +
                         std::to_string(params.pitch);
        // MD5 哈希
        return md5(key);
    }
};
```

#### 2. 集成到聊天服务

```cpp
// src/services/ChatService.cpp

class ChatService {
private:
    GPTSoVITSClient voice_client;
    OpenClawService openclaw;  // 使用 OpenClaw 管理 AI 逻辑
    
public:
    /**
     * 处理用户消息并生成回复
     */
    ChatResponse processMessage(const ChatRequest& req) {
        ChatResponse response;
        
        // 1. OpenClaw 生成文本回复
        std::string ai_text = openclaw.generateResponse(
            req.user_id,
            req.message,
            req.context
        );
        
        // 2. OpenClaw 分析情感和语音参数
        VoiceParams voice_params = openclaw.analyzeEmotion(ai_text);
        
        // 3. GPT-SoVITS 合成语音
        std::string audio_base64 = voice_client.synthesize(
            ai_text,
            "yachiyo_default",  // 说话人 ID
            voice_params
        );
        
        // 4. 构建响应
        response.text = ai_text;
        response.audio = audio_base64;
        response.voice_params = voice_params;
        response.timestamp = getCurrentTime();
        
        return response;
    }
    
    /**
     * 流式音频传输
     */
    void streamAudio(
        const ChatRequest& req,
        crow::response& res
    ) {
        res.set_header("Content-Type", "application/octet-stream");
        res.set_header("Transfer-Encoding", "chunked");
        
        // 生成音频流
        std::string audio_data = voice_client.synthesize(...);
        
        // 分块发送
        const int chunk_size = 4096;
        for (size_t i = 0; i < audio_data.size(); i += chunk_size) {
            res.body += audio_data.substr(i, chunk_size);
        }
    }
};
```

### REST API 端点

```cpp
// 语音合成 API
app.post("/api/voice/synthesize").methods("POST"_method)(
    [&voice_service](const crow::request& req) {
        auto body = crow::json::load(req.body);
        
        VoiceParams params = {
            .speed = body["speed"].d(),
            .pitch = body["pitch"].d(),
            .energy = body["energy"].d()
        };
        
        std::string audio = voice_service.synthesize(
            body["text"].s(),
            body["speaker_id"].s(),
            params
        );
        
        return crow::response(200, crow::json::wvalue{
            {"status", "success"},
            {"audio", audio}
        }.dump());
    }
);

// 声音克隆 API
app.post("/api/voice/clone").methods("POST"_method)(
    [&voice_service](const crow::request& req) {
        auto body = crow::json::load(req.body);
        
        std::vector<std::string> samples;
        for (const auto& sample : body["samples"]) {
            samples.push_back(sample.s());
        }
        
        std::string speaker_id = voice_service.cloneVoice(
            samples,
            body["speaker_name"].s()
        );
        
        return crow::response(200, crow::json::wvalue{
            {"status", "success"},
            {"speaker_id", speaker_id}
        }.dump());
    }
);
```

### 前端调用示例

```typescript
// frontend/src/services/voiceService.ts

export class VoiceService {
  /**
   * 合成语音
   */
  async synthesize(text: string, params?: VoiceParams): Promise<ArrayBuffer> {
    const response = await fetch('/api/voice/synthesize', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        text,
        speaker_id: 'yachiyo_default',
        speed: params?.speed ?? 1.0,
        pitch: params?.pitch ?? 0.0,
        energy: params?.energy ?? 1.0,
      }),
    });

    const data = await response.json();
    
    // Base64 转 ArrayBuffer
    return this.base64ToArrayBuffer(data.audio);
  }

  /**
   * 播放音频
   */
  async playAudio(text: string): Promise<void> {
    const audioBuffer = await this.synthesize(text);
    
    const audioContext = new (window.AudioContext || (window as any).webkitAudioContext)();
    const source = audioContext.createBufferSource();
    
    const buffer = await audioContext.decodeAudioData(audioBuffer);
    source.buffer = buffer;
    source.connect(audioContext.destination);
    source.start(0);
  }

  /**
   * 克隆声音
   */
  async cloneVoice(samples: Blob[], speakerName: string): Promise<string> {
    const sampleBase64 = await Promise.all(
      samples.map(blob => this.blobToBase64(blob))
    );

    const response = await fetch('/api/voice/clone', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        samples: sampleBase64,
        speaker_name: speakerName,
      }),
    });

    const data = await response.json();
    return data.speaker_id;
  }

  private base64ToArrayBuffer(base64: string): ArrayBuffer {
    const binaryString = atob(base64);
    const bytes = new Uint8Array(binaryString.length);
    for (let i = 0; i < binaryString.length; i++) {
      bytes[i] = binaryString.charCodeAt(i);
    }
    return bytes.buffer;
  }

  private blobToBase64(blob: Blob): Promise<string> {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onloadend = () => {
        const base64 = (reader.result as string).split(',')[1];
        resolve(base64);
      };
      reader.onerror = reject;
      reader.readAsDataURL(blob);
    });
  }
}
```

---

## 声音克隆

### 完整克隆流程

```
第 1 步: 准备语音样本
  ✓ 收集 10-30 秒的清晰语音
  ✓ 格式: WAV 或 MP3
  ✓ 采样率: 22050Hz 或 44100Hz

第 2 步: 上传样本
  POST /api/voice/clone
  {
    "samples": ["base64_audio_1", "base64_audio_2"],
    "speaker_name": "新声音"
  }

第 3 步: 模型训练 (后台)
  ✓ HuBERT 特征提取
  ✓ GPT 风格学习
  ✓ VITS 声码器适应
  时间: 5-30 分钟 (取决于样本质量)

第 4 步: 测试克隆效果
  GET /api/voice/test?speaker_id=xxx
  测试音频: "你好，这是克隆的声音"

第 5 步: 投入使用
  使用 speaker_id 进行合成
```

### 示例实现

```cpp
// src/services/VoiceCloning.cpp

class VoiceCloningService {
public:
    /**
     * 克隆声音工作流
     */
    std::string cloneVoice(
        const std::vector<std::string>& audio_samples,
        const std::string& speaker_name
    ) {
        // 1. 验证样本
        for (const auto& sample : audio_samples) {
            if (!validateAudioSample(sample)) {
                throw std::runtime_error("音频样本无效");
            }
        }
        
        // 2. 生成 Speaker ID
        std::string speaker_id = generateSpeakerId(speaker_name);
        
        // 3. 保存样本到磁盘
        std::string sample_dir = "data/voice_samples/" + speaker_id;
        createDirectory(sample_dir);
        
        for (size_t i = 0; i < audio_samples.size(); ++i) {
            std::string file_path = sample_dir + "/sample_" + 
                                   std::to_string(i) + ".wav";
            saveAudioFile(file_path, audio_samples[i]);
        }
        
        // 4. 提交训练任务到 GPT-SoVITS
        submitTrainingJob(speaker_id, sample_dir);
        
        // 5. 保存元数据
        saveMetadata(speaker_id, speaker_name);
        
        return speaker_id;
    }

private:
    bool validateAudioSample(const std::string& audio_base64) {
        // 检查长度
        std::string audio = base64Decode(audio_base64);
        double duration = audio.size() / (2.0 * 22050);  // 16-bit, 22050Hz
        
        if (duration < 5 || duration > 120) {
            LOG_WARN("音频长度不在允许范围: {} 秒", duration);
            return false;
        }
        
        return true;
    }
    
    void submitTrainingJob(
        const std::string& speaker_id,
        const std::string& sample_dir
    ) {
        json job_request = {
            {"action", "train_speaker"},
            {"speaker_id", speaker_id},
            {"sample_dir", sample_dir},
            {"priority", "normal"}
        };
        
        // 发送给 GPT-SoVITS 后台任务队列
        sendToTaskQueue(job_request);
        
        // 监听完成事件
        waitForCompletion(speaker_id);
    }
};
```

---

## 性能优化

### 1. 音频缓存策略

```cpp
class AudioCache {
private:
    std::unordered_map<std::string, CachedAudio> cache;
    std::queue<std::string> lru_queue;
    const int MAX_CACHE_SIZE = 1000;
    const int TTL_SECONDS = 86400;  // 24 小时
    
public:
    std::optional<std::string> get(const std::string& key) {
        auto it = cache.find(key);
        if (it != cache.end()) {
            // 检查过期
            if (std::time(nullptr) - it->second.timestamp > TTL_SECONDS) {
                cache.erase(it);
                return std::nullopt;
            }
            
            it->second.hits++;  // 更新命中计数
            return it->second.audio;
        }
        return std::nullopt;
    }
    
    void set(const std::string& key, const std::string& audio) {
        if (cache.size() >= MAX_CACHE_SIZE) {
            // 移除最少使用的项
            auto lru_key = findLRUKey();
            cache.erase(lru_key);
        }
        
        cache[key] = {
            .audio = audio,
            .timestamp = std::time(nullptr),
            .hits = 0
        };
    }
};
```

### 2. 批量合成优化

```cpp
/**
 * 批量合成语音（减少 API 开销）
 */
std::vector<std::string> syntheziseBatch(
    const std::vector<std::string>& texts
) {
    // 分组合成，减少往返
    const int BATCH_SIZE = 5;
    std::vector<std::string> results;
    
    for (size_t i = 0; i < texts.size(); i += BATCH_SIZE) {
        auto batch = std::vector<std::string>(
            texts.begin() + i,
            texts.begin() + std::min(i + BATCH_SIZE, texts.size())
        );
        
        // 并行发送
        auto batch_results = voice_client.synthesizeBatch(batch);
        results.insert(results.end(), batch_results.begin(), batch_results.end());
    }
    
    return results;
}
```

### 3. GPU 优化

```bash
# 配置文件中的 GPU 优化参数

gpt_sovits:
  gpu:
    device: "cuda:0"
    mixed_precision: true  # 混合精度加速
    enable_tf32: true      # TF32 加速
    
  inference:
    # 批处理大小
    batch_size: 8
    # 量化
    quantization: "int8"   # 8-bit 量化
    # 编译优化
    torch_compile: true    # PyTorch 2.0 编译
    
  cache:
    # GPU 内存缓存
    gpu_cache_size: 2048MB
```

---

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 连接超时 | 服务未运行或防火墙 | 检查服务状态，开放端口 |
| CUDA 错误 | GPU 不可用或显存不足 | 检查 NVIDIA 驱动，清理显存 |
| 音频质量差 | 模型不匹配或参数错误 | 更新模型，调整参数 |
| 延迟过高 | 批处理过大 | 减小批大小或使用流式输出 |

### 诊断脚本

```bash
#!/bin/bash
# diagnose_gpt_sovits.sh

echo "=== GPT-SoVITS 诊断 ==="

# 检查服务
echo "【1】检查服务..."
curl -s http://localhost:9880/api/health | jq . || echo "❌ 服务不可用"

# 检查 GPU
echo "【2】检查 GPU..."
nvidia-smi || echo "❌ NVIDIA GPU 不可用"

# 检查模型
echo "【3】检查模型文件..."
ls -lh models/gpt_sovits/

# 测试合成
echo "【4】测试语音合成..."
curl -X POST http://localhost:9880/api/synthesize \
  -H "Content-Type: application/json" \
  -d '{"text": "测试", "speaker_id": "default"}' | jq .

echo "诊断完成"
```

---

**GPT-SoVITS 集成完成！** 🎙️

---

*最后更新: 2026-04-02*  
*下一步: 进行声音克隆和质量测试*
