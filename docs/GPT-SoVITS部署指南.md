# GPT-SoVITS 语音合成部署指南

> 适用于 Yachiyo 虚拟直播平台  
> GPT-SoVITS 是 AI 语音克隆 & 合成工具，用于生成八千代辉夜姬的实时语音。

---

## 1. 环境要求

### 硬件

| 项目 | 最低配置 | 推荐配置 |
| ---- | ---- | ---- |
| GPU | NVIDIA GPU 4GB VRAM | NVIDIA RTX 3060+ (8GB VRAM) |
| RAM | 8GB | 16GB+ |
| 磁盘 | 10GB 可用空间 | 20GB+ SSD |
| CUDA | 11.7+ | 12.1+ |

> ⚠️ **无 GPU 方案**: 可以使用 CPU 推理但速度极慢（约 10x），不适合实时直播场景。如果服务器没有 GPU，建议将 GPT-SoVITS 部署在带 GPU 的独立机器上。

### 软件

- Python 3.9 ~ 3.11（推荐 3.10）
- NVIDIA Driver 525+
- CUDA Toolkit 11.7+ 或 12.1+
- FFmpeg（音频处理）
- Git

---

## 2. 安装步骤

### 2.1 克隆仓库

```bash
cd /opt
git clone https://github.com/RVC-Boss/GPT-SoVITS.git
cd GPT-SoVITS
```

### 2.2 创建 Python 虚拟环境

```bash
# 使用 conda（推荐）
conda create -n sovits python=3.10 -y
conda activate sovits

# 或使用 venv
python3.10 -m venv venv
source venv/bin/activate
```

### 2.3 安装依赖

```bash
# 安装 PyTorch（根据 CUDA 版本选择）
# CUDA 11.8:
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118

# CUDA 12.1:
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121

# 安装项目依赖
pip install -r requirements.txt
```

### 2.4 安装 FFmpeg

```bash
# Arch Linux
sudo pacman -S ffmpeg

# Ubuntu/Debian
sudo apt install ffmpeg

# 验证
ffmpeg -version
```

### 2.5 下载预训练模型

GPT-SoVITS 需要基础预训练模型:

```bash
# 模型会放在 GPT_SoVITS/pretrained_models/ 目录下
# 方法 1: 自动下载（运行时会自动下载，但可能很慢）
# 方法 2: 手动下载
# 访问 https://huggingface.co/lj1995/GPT-SoVITS 下载以下文件:
#   - s1bert25hz-2kh-longer-epoch=68e-step=50232.ckpt
#   - s2D488k.pth
#   - s2G488k.pth
# 放入 GPT_SoVITS/pretrained_models/

# 中文语音还需要中文 BERT 模型:
# 下载 chinese-roberta-wwm-ext-large 到 GPT_SoVITS/pretrained_models/chinese-roberta-wwm-ext-large/
```

---

## 3. 训练八千代辉夜姬语音模型

### 3.1 准备参考音频

收集八千代辉夜姬的语音素材：

```
resources/sovits_training/
├── raw_audio/          # 原始音频（10~60 分钟，越多越好）
│   ├── clip_001.wav
│   ├── clip_002.wav
│   └── ...
└── transcripts.list    # 对应的文本标注
```

**音频要求**:
- 格式: WAV (16bit, 单声道)
- 采样率: 44100Hz 或 48000Hz
- 时长: 单条 2~15 秒，总计至少 10 分钟
- 质量: 无背景噪音、无 BGM、清晰人声

**文本标注文件格式** (`transcripts.list`):

```
raw_audio/clip_001.wav|yachiyo|ZH|你好，我是八千代辉夜姬
raw_audio/clip_002.wav|yachiyo|ZH|今天天气真好呢
raw_audio/clip_003.wav|yachiyo|JA|こんにちは、私は八千代輝夜姫です
```

格式: `音频路径|说话人名|语言(ZH/EN/JA)|文本内容`

### 3.2 数据预处理

```bash
cd /opt/GPT-SoVITS

# 启动 WebUI 进行数据处理和训练
python webui.py
```

在浏览器打开 `http://localhost:9871`，按以下步骤操作：

1. **0-数据准备**:
   - 选择音频文件目录
   - 进行音频切分（自动按静音切分为短片段）
   - 进行语音去噪（可选但推荐）
   - ASR 自动标注（或使用已有的 transcripts.list）

2. **1-训练 SoVITS 模型**:
   - 设置 batch_size（根据 VRAM 调整，6GB 显存建议 4~6）
   - 训练轮次: 建议 8~12 个 epoch
   - 保存路径: `logs/yachiyo/`

3. **2-训练 GPT 模型**:
   - 同上配置
   - 训练轮次: 建议 15~20 个 epoch

### 3.3 训练产物

训练完成后会生成：

```
logs/yachiyo/
├── yachiyo_e8_s200.pth      # SoVITS 模型
└── yachiyo-e15.ckpt          # GPT 模型
```

---

## 4. 部署 API 服务

### 4.1 命令行启动 API 服务器

```bash
cd /opt/GPT-SoVITS

python api.py \
  -s logs/yachiyo/yachiyo_e8_s200.pth \
  -g logs/yachiyo/yachiyo-e15.ckpt \
  -dr "resources/reference_audio/yachiyo_ref.wav" \
  -dt "你好，我是八千代辉夜姬" \
  -dl "zh" \
  -a 0.0.0.0 \
  -p 5000
```

参数说明：
- `-s`: SoVITS 模型路径
- `-g`: GPT 模型路径
- `-dr`: 参考音频路径（3~10秒的清晰语音）
- `-dt`: 参考音频对应的文本
- `-dl`: 参考音频的语言
- `-a`: 监听地址
- `-p`: 监听端口

### 4.2 API 接口

启动后可用接口：

**POST /synthesize** (Yachiyo 后端调用的接口)

```json
// 请求
{
  "text": "你好，我是八千代辉夜姬",
  "text_language": "zh",
  "ref_audio_path": "resources/reference_audio/yachiyo_ref.wav",
  "prompt_text": "你好，我是八千代辉夜姬",
  "prompt_language": "zh",
  "speed": 1.0
}

// 响应
{
  "success": true,
  "audio_url": "/audio/output_xxxxx.wav",
  "duration_ms": 2500
}
```

**GET /** (TTS 快捷接口)

```
GET http://localhost:5000/?text=你好&text_language=zh
```

返回 WAV 音频流。

### 4.3 验证服务

```bash
# 测试 API
curl -X POST http://localhost:5000/synthesize \
  -H "Content-Type: application/json" \
  -d '{"text": "测试语音合成", "text_language": "zh"}'

# 或简单测试
curl "http://localhost:5000/?text=测试&text_language=zh" --output test.wav
aplay test.wav  # 播放测试
```

---

## 5. Docker 部署（推荐）

### 5.1 Dockerfile

在 `GPT-SoVITS` 目录创建 `Dockerfile`:

```dockerfile
FROM nvidia/cuda:12.1.0-runtime-ubuntu22.04

RUN apt-get update && apt-get install -y \
    python3.10 python3.10-venv python3-pip ffmpeg git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN pip install --no-cache-dir torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121
RUN pip install --no-cache-dir -r requirements.txt

EXPOSE 5000

CMD ["python3", "api.py", "-a", "0.0.0.0", "-p", "5000", \
     "-s", "/app/models/sovits.pth", \
     "-g", "/app/models/gpt.ckpt", \
     "-dr", "/app/reference/ref.wav", \
     "-dt", "你好，我是八千代辉夜姬", \
     "-dl", "zh"]
```

### 5.2 添加到 docker-compose.yml

```yaml
  gpt-sovits:
    build: ./gpt-sovits
    ports:
      - "5000:5000"
    volumes:
      - ./resources/sovits_models:/app/models
      - ./resources/reference_audio:/app/reference
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]
    restart: unless-stopped
```

---

## 6. 与 Yachiyo 后端对接

### 6.1 配置文件

在 `backend/config/config.yaml` 中确保:

```yaml
gpt_sovits:
  enabled: true
  api_endpoint: "http://localhost:5000"  # 或 Docker 内: http://gpt-sovits:5000
  api_key: ""  # GPT-SoVITS 默认无鉴权，可留空
```

### 6.2 后端调用链路

```
用户发送消息
  → DeepSeek AI 生成文本回复
  → GPTSoVITSService::synthesize(text, emotion)
    → POST http://localhost:5000/synthesize
    → 返回 audio_url + duration_ms
  → WebSocket 推送给前端
    → {text, audio_url, audio_duration_ms, emotions, actions}
  → 前端播放音频 + Live2D 口型同步
```

### 6.3 voice_presets 配置

`GPTSoVITSService.cpp` 中已定义三种预设:

| 预设 | pitch | speed | energy | 适用场景 |
| ---- | ---- | ---- | ---- | ---- |
| DEFAULT | 1.0 | 1.0 | 0.5 | 日常对话 |
| PLAYFUL | 1.2 | 1.1 | 0.8 | 撒娇/兴奋 |
| COOL | 0.9 | 0.9 | 0.6 | 认真/冷酷 |

---

## 7. 常见问题

### Q: CUDA out of memory

降低推理时的 batch_size，或使用半精度推理:

```bash
# 在 api.py 启动时添加
export PYTORCH_CUDA_ALLOC_CONF=max_split_size_mb:128
```

### Q: 生成的语音质量差

- 确保参考音频质量高（无噪音、清晰）
- 增加训练数据量和训练轮次
- 尝试不同的参考音频

### Q: API 延迟过高

- 首次请求会加载模型，之后会快很多
- 推荐使用 GPU 推理，CPU 推理延迟 10x+
- 可以预热: 启动后先发一个测试请求加载模型

### Q: 如何支持多情感语音

准备不同情感的参考音频，通过 `synthesizeWithEmotion` 接口切换:

```
resources/reference_audio/
├── yachiyo_neutral.wav    # 平静
├── yachiyo_happy.wav      # 开心
├── yachiyo_sad.wav        # 悲伤
└── yachiyo_angry.wav      # 生气
```

后端会根据 emotion 参数选择不同的参考音频。
