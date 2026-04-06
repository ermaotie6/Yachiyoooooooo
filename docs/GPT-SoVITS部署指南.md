# GPT-SoVITS 语音合成部署指南

> 适用于 Yachiyo 虚拟直播平台
> GPT-SoVITS 是 AI 语音克隆 & 合成工具，用于生成八千代辉夜姬的日语语音。

---

## 0. 总体流程

```
Windows 本机训练 → 导出模型文件 → 上传到 Linux 服务器 → 启动 API 服务 → Yachiyo 后端调用
```

训练在 Windows 完成（有 GUI、资源消耗可控），推理部署在 Linux 服务器（CPU 或 GPU 均可）。

---

## 1. 环境要求

### 1.1 训练环境（Windows 本机）

| 项目 | 最低配置 | 推荐配置 |
|------|---------|---------|
| GPU | NVIDIA 4GB VRAM | NVIDIA RTX 3060+ (8GB VRAM) |
| RAM | 8GB | 16GB+ |
| 磁盘 | 10GB 可用 | 20GB+ SSD |
| CUDA | 11.7+ | 12.1+ |
| Python | 3.9 ~ 3.11 | 3.10 |
| 系统 | Windows 10/11 | |

> 训练资源消耗不大。一段 5 分钟的语音素材，SoVITS 训练约 10~20 分钟，GPT 训练约 20~40 分钟（RTX 3060 基准）。总计 1 小时以内就能完成。显存占用约 4~6GB，训练期间可以正常使用电脑。

### 1.2 推理/部署环境（Linux 服务器）

GPT-SoVITS 支持 **GPU 推理** 和 **CPU 推理** 两种模式。后端代码同时覆盖两种模式，通过配置切换。

| 模式 | 硬件要求 | 单条延迟 | 适用场景 |
|------|---------|---------|---------|
| **GPU 推理** | NVIDIA GPU 4GB+ VRAM | 0.5~2 秒 | 实时直播、低延迟 |
| **CPU 推理** | 4 核 CPU + 8GB RAM | 5~30 秒 | 非实时、预生成、低成本服务器 |

> **CPU 推理说明**：延迟较高但完全可用。如果你的直播场景可以容忍几秒延迟（比如先显示文字再播放语音），CPU 推理完全够用，不需要 GPU 服务器。

### 1.3 软件依赖

- Python 3.9 ~ 3.11（推荐 3.10）
- FFmpeg（音频处理）
- Git
- （GPU 模式）NVIDIA Driver 525+、CUDA Toolkit 11.7+ 或 12.1+

---

## 2. Windows 训练完整步骤

### 2.1 安装 GPT-SoVITS

```powershell
# 克隆仓库
cd D:\AI
git clone https://github.com/RVC-Boss/GPT-SoVITS.git
cd GPT-SoVITS

# 创建虚拟环境（推荐 conda）
conda create -n sovits python=3.10 -y
conda activate sovits

# 安装 PyTorch（根据你的 CUDA 版本）
# CUDA 11.8:
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118
# CUDA 12.1:
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121

# 安装项目依赖
pip install -r requirements.txt

# 安装 FFmpeg（Windows）
# 方法1: 从 https://www.gyan.dev/ffmpeg/builds/ 下载，解压后将 bin 目录加入 PATH
# 方法2: winget install ffmpeg
# 方法3: choco install ffmpeg
```

### 2.2 下载预训练模型

GPT-SoVITS 需要基础预训练模型，首次运行 WebUI 时会自动下载。也可以手动下载：

```
访问 https://huggingface.co/lj1995/GPT-SoVITS
下载以下文件放入 GPT_SoVITS/pretrained_models/:
  - s1bert25hz-2kh-longer-epoch=68e-step=50232.ckpt
  - s2D488k.pth
  - s2G488k.pth

中文 BERT（如果合成中文也需要）:
  下载 chinese-roberta-wwm-ext-large 到 GPT_SoVITS/pretrained_models/chinese-roberta-wwm-ext-large/
```

### 2.3 准备语音素材

你有一段约 5 分钟的完整语音素材。GPT-SoVITS 训练需要 **短片段**（2~15 秒/条），所以需要先切分。

#### 素材放置

```
D:\AI\GPT-SoVITS\
└── training_data/
    └── yachiyo/
        └── raw/
            └── yachiyo_full.wav    ← 你的 5 分钟完整音频
```

**音频格式要求**：

- 格式：WAV（16bit，单声道）
- 采样率：44100Hz 或 48000Hz
- 质量：无背景噪音、无 BGM、清晰人声
- 如果你的素材是 MP3/M4A，先用 FFmpeg 转换：

```powershell
ffmpeg -i yachiyo_full.mp3 -ar 44100 -ac 1 -sample_fmt s16 yachiyo_full.wav
```

### 2.4 启动 WebUI 进行训练

```powershell
cd D:\AI\GPT-SoVITS
conda activate sovits
python webui.py
```

浏览器打开 `http://localhost:9871`，按以下步骤操作：

#### 步骤 1: 音频切分

在 WebUI 的 **"0-音频处理"** 标签页：

1. **输入目录**: `training_data/yachiyo/raw`
2. **输出目录**: `training_data/yachiyo/sliced`（会自动创建）
3. 点击 **"音频切分"** — 自动按静音区间切为 2~15 秒的短片段
4. 切分完成后检查 `sliced/` 目录，应该有 20~50 个小片段

> 5 分钟的音频通常切出 20~40 个片段，这个数量用于 few-shot 声音克隆完全足够。

#### 步骤 2: 语音去噪（推荐）

在同一标签页：

1. **输入目录**: `training_data/yachiyo/sliced`
2. 点击 **"语音去噪"**
3. 输出到 `training_data/yachiyo/denoised`

#### 步骤 3: ASR 自动标注

1. **输入目录**: `training_data/yachiyo/denoised`（或 `sliced`）
2. **ASR 模型**: 选择 `funasr`（中文推荐）或 `whisper`（多语言）
3. **语言**: 选择对应语言
4. 点击 **"ASR 标注"**
5. 生成的标注文件在 `training_data/yachiyo/` 下，格式：

```
denoised/clip_001.wav|yachiyo|ZH|你好我是八千代辉夜姬
denoised/clip_002.wav|yachiyo|ZH|今天天气真好呢
...
```

6. **手动校对**（重要！）：打开生成的 `.list` 文件，逐条听音频对照文本，修正错误

#### 步骤 4: 训练 SoVITS 模型

在 **"1-SoVITS 训练"** 标签页：

1. **训练集路径**: 指向标注好的 `.list` 文件
2. **模型名称**: `yachiyo`
3. **batch_size**: 根据显存调整
   - 4GB VRAM → batch_size 2~3
   - 6GB VRAM → batch_size 4~6
   - 8GB+ VRAM → batch_size 6~8
4. **训练轮次 (epoch)**: 8~12（5 分钟素材建议 10）
5. 点击 **"开始训练"**
6. 等待完成（约 10~20 分钟）

#### 步骤 5: 训练 GPT 模型

在 **"2-GPT 训练"** 标签页：

1. **训练集路径**: 同上
2. **模型名称**: `yachiyo`
3. **batch_size**: 同上
4. **训练轮次 (epoch)**: 15~20（5 分钟素材建议 15）
5. 点击 **"开始训练"**
6. 等待完成（约 20~40 分钟）

### 2.5 训练产物

训练完成后，模型文件在：

```
D:\AI\GPT-SoVITS\logs\yachiyo\
├── yachiyo_e10_s400.pth        ← SoVITS 模型（约 200~400MB）
└── yachiyo-e15.ckpt            ← GPT 模型（约 300~500MB）
```

### 2.6 选取参考音频

从切分后的音频中选一条 **3~10 秒** 的清晰片段作为参考音频（推理时使用）：

```powershell
# 从切分结果中复制一条效果好的
copy training_data\yachiyo\denoised\clip_005.wav reference_audio\yachiyo_ref.wav
```

**参考音频的选取标准**：

- 时长 3~10 秒（太短效果差，太长浪费计算）
- 语速适中、情感自然
- 发音清晰、无杂音
- 最好包含完整句子

在 WebUI 的 **"推理"** 标签页测试效果，确认满意后再部署。

### 2.7 多情感参考音频（可选）

如果你的 5 分钟素材中有不同情感的片段，可以分别选取：

```
reference_audio/
├── yachiyo_neutral.wav     ← 平静/默认
├── yachiyo_happy.wav       ← 开心
├── yachiyo_sad.wav         ← 悲伤
└── yachiyo_angry.wav       ← 生气
```

后端会根据 OpenClaw 返回的 emotion 标签自动选择对应的参考音频。

---

## 3. 从 Windows 迁移到 Linux 服务器

### 3.1 需要上传的文件

从 Windows 上传以下文件到服务器（总共约 1~2GB）：

```
需要上传的文件:
├── yachiyo_e10_s400.pth        ← SoVITS 模型
├── yachiyo-e15.ckpt            ← GPT 模型
├── yachiyo_ref.wav             ← 参考音频（默认）
├── yachiyo_happy.wav           ← 参考音频（开心，可选）
├── yachiyo_sad.wav             ← 参考音频（悲伤，可选）
└── yachiyo_angry.wav           ← 参考音频（生气，可选）
```

上传到服务器：

```bash
# 在服务器上创建目录
ssh your-server "mkdir -p /opt/yachiyo/sovits-models /opt/yachiyo/reference-audio"

# 上传模型
scp yachiyo_e10_s400.pth your-server:/opt/yachiyo/sovits-models/
scp yachiyo-e15.ckpt your-server:/opt/yachiyo/sovits-models/

# 上传参考音频
scp yachiyo_ref.wav your-server:/opt/yachiyo/reference-audio/
scp yachiyo_happy.wav your-server:/opt/yachiyo/reference-audio/   # 可选
scp yachiyo_sad.wav your-server:/opt/yachiyo/reference-audio/     # 可选
scp yachiyo_angry.wav your-server:/opt/yachiyo/reference-audio/   # 可选
```

### 3.2 跨平台兼容性

PyTorch 模型文件（`.pth`、`.ckpt`）是 **跨平台的**，Windows 训练的模型可以直接在 Linux 上加载，无需额外转换。唯一注意：

- Windows 和 Linux 的 PyTorch 版本应尽量一致（大版本相同即可）
- Python 版本保持一致（3.10）
- 预训练基础模型（`s2G488k.pth` 等）也需要在服务器上部署

---

## 4. Linux 服务器部署

### 4.1 安装 GPT-SoVITS

```bash
cd /opt
git clone https://github.com/RVC-Boss/GPT-SoVITS.git
cd GPT-SoVITS

# 创建虚拟环境
python3.10 -m venv venv
source venv/bin/activate
```

### 4.2 安装依赖（GPU 模式 vs CPU 模式）

#### GPU 模式（服务器有 NVIDIA 显卡）

```bash
# 安装 GPU 版 PyTorch
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121

# 安装项目依赖
pip install -r requirements.txt
```

#### CPU 模式（服务器无 GPU）

```bash
# 安装 CPU 版 PyTorch（注意：与 GPU 版不同的 index-url）
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu

# 安装项目依赖
pip install -r requirements.txt
```

> CPU 模式不需要 CUDA、不需要 NVIDIA 驱动、不需要任何 GPU 相关组件。PyTorch CPU 版本会自动使用 CPU 进行推理。

### 4.3 放置模型和参考音频

```bash
# 将上传的文件放到正确位置
cp /opt/yachiyo/sovits-models/yachiyo_e10_s400.pth /opt/GPT-SoVITS/models/
cp /opt/yachiyo/sovits-models/yachiyo-e15.ckpt /opt/GPT-SoVITS/models/
cp /opt/yachiyo/reference-audio/*.wav /opt/GPT-SoVITS/reference/

# 确保预训练模型也存在
ls /opt/GPT-SoVITS/GPT_SoVITS/pretrained_models/
# 应该有: s1bert25hz-*.ckpt, s2D488k.pth, s2G488k.pth
# 如果没有，需要手动下载（同2.2节）
```

### 4.4 安装 FFmpeg

```bash
# Ubuntu/Debian
sudo apt install ffmpeg

# Arch Linux
sudo pacman -S ffmpeg

# 验证
ffmpeg -version
```

### 4.5 启动 API 服务

#### GPU 模式启动

```bash
cd /opt/GPT-SoVITS
source venv/bin/activate

python api.py \
  -s models/yachiyo_e10_s400.pth \
  -g models/yachiyo-e15.ckpt \
  -dr reference/yachiyo_ref.wav \
  -dt "你好，我是八千代辉夜姬" \
  -dl "zh" \
  -a 0.0.0.0 \
  -p 5000
```

#### CPU 模式启动

```bash
cd /opt/GPT-SoVITS
source venv/bin/activate

# 关键: 设置 CUDA_VISIBLE_DEVICES 为空，强制使用 CPU
CUDA_VISIBLE_DEVICES="" python api.py \
  -s models/yachiyo_e10_s400.pth \
  -g models/yachiyo-e15.ckpt \
  -dr reference/yachiyo_ref.wav \
  -dt "你好，我是八千代辉夜姬" \
  -dl "zh" \
  -a 0.0.0.0 \
  -p 5000
```

> CPU 模式下首次请求会比较慢（模型加载），后续请求约 5~15 秒/条（取决于文本长度和 CPU 性能）。

#### 参数说明

| 参数 | 说明 |
|------|------|
| `-s` | SoVITS 模型路径 |
| `-g` | GPT 模型路径 |
| `-dr` | 默认参考音频路径（3~10秒清晰语音） |
| `-dt` | 参考音频对应的文本 |
| `-dl` | 参考音频的语言（zh/en/ja） |
| `-a` | 监听地址 |
| `-p` | 监听端口 |

### 4.6 用 systemd 管理服务（推荐）

创建 `/etc/systemd/system/gpt-sovits.service`：

```ini
[Unit]
Description=GPT-SoVITS TTS API Service
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/GPT-SoVITS
# GPU 模式:
ExecStart=/opt/GPT-SoVITS/venv/bin/python api.py -s models/yachiyo_e10_s400.pth -g models/yachiyo-e15.ckpt -dr reference/yachiyo_ref.wav -dt "你好，我是八千代辉夜姬" -dl zh -a 0.0.0.0 -p 5000
# CPU 模式 (取消注释下面两行，注释掉上面的 ExecStart):
# Environment=CUDA_VISIBLE_DEVICES=
# ExecStart=/opt/GPT-SoVITS/venv/bin/python api.py -s models/yachiyo_e10_s400.pth -g models/yachiyo-e15.ckpt -dr reference/yachiyo_ref.wav -dt "你好，我是八千代辉夜姬" -dl zh -a 0.0.0.0 -p 5000
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable gpt-sovits
sudo systemctl start gpt-sovits
sudo systemctl status gpt-sovits
```

### 4.7 验证

```bash
# 健康检查
curl http://localhost:5000/

# 测试合成
curl -X POST http://localhost:5000/synthesize \
  -H "Content-Type: application/json" \
  -d '{"text": "テスト音声合成", "text_language": "ja"}'

# 或快捷测试
curl "http://localhost:5000/?text=テスト&text_language=ja" --output test.wav
aplay test.wav
```

---

## 5. Docker 部署

### 5.1 GPU 模式 Dockerfile

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

### 5.2 CPU 模式 Dockerfile

```dockerfile
FROM python:3.10-slim

RUN apt-get update && apt-get install -y ffmpeg git && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

# 关键: 安装 CPU 版 PyTorch
RUN pip install --no-cache-dir torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu
RUN pip install --no-cache-dir -r requirements.txt

# 强制 CPU 模式
ENV CUDA_VISIBLE_DEVICES=""

EXPOSE 5000

CMD ["python3", "api.py", "-a", "0.0.0.0", "-p", "5000", \
     "-s", "/app/models/sovits.pth", \
     "-g", "/app/models/gpt.ckpt", \
     "-dr", "/app/reference/ref.wav", \
     "-dt", "你好，我是八千代辉夜姬", \
     "-dl", "zh"]
```

### 5.3 添加到 docker-compose.yml

```yaml
  # GPU 模式:
  gpt-sovits:
    build:
      context: ./gpt-sovits
      dockerfile: Dockerfile.gpu
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

  # CPU 模式 (二选一，注释掉上面的 GPU 模式):
  # gpt-sovits:
  #   build:
  #     context: ./gpt-sovits
  #     dockerfile: Dockerfile.cpu
  #   ports:
  #     - "5000:5000"
  #   volumes:
  #     - ./resources/sovits_models:/app/models
  #     - ./resources/reference_audio:/app/reference
  #   restart: unless-stopped
```

---

## 6. 与 Yachiyo 后端对接

### 6.1 配置文件

在 `config/config.yaml` 中：

```yaml
gpt_sovits:
  enabled: true
  api_endpoint: "http://localhost:5000"  # Docker 内: http://gpt-sovits:5000
  mode: "cpu"                            # "gpu" 或 "cpu"（仅影响 timeout 策略）
```

在 `backend/config/config.yaml` 中：

```yaml
ai:
  gpt_sovits:
    enabled: true
    endpoint: "http://localhost:5000"
    timeout_seconds: 60  # CPU 模式建议 60，GPU 模式可设为 15
```

### 6.2 API 接口

**POST /synthesize**

```json
{
  "text": "こんにちは、八千代輝夜姫です",
  "text_language": "ja",
  "ref_audio_path": "reference/yachiyo_ref.wav",
  "prompt_text": "你好，我是八千代辉夜姬",
  "prompt_language": "zh",
  "speed": 1.0
}
```

响应：

```json
{
  "success": true,
  "audio_url": "/audio/output_xxxxx.wav",
  "duration_ms": 2500
}
```

**GET /** (快捷接口)

```
GET http://localhost:5000/?text=テスト&text_language=ja
```

返回 WAV 音频流。

### 6.3 后端调用链路

```
OpenClaw 返回回答文本 + 情感标签
  → TranslationService: 翻译为日语
  → GPTSoVITSService::synthesizeWithEmotion(日语文本, emotion)
    → 根据 emotion 选择参考音频 (happy→yachiyo_happy.wav)
    → POST http://localhost:5000/synthesize
    → 返回 audio_url + duration_ms
  → WebSocket 推送给前端
    → {text, audio_url, audio_duration_ms, emotions, actions}
  → 前端播放音频 + Live2D 口型同步
```

### 6.4 情感参考音频映射

| 情感标签 | 参考音频 | 说明 |
|---------|---------|------|
| neutral/default | yachiyo_ref.wav | 平静默认 |
| happy/excited | yachiyo_happy.wav | 开心/兴奋 |
| sad | yachiyo_sad.wav | 悲伤 |
| angry | yachiyo_angry.wav | 生气 |

如果某个情感没有对应的参考音频，会 fallback 到默认的 `yachiyo_ref.wav`。

---

## 7. 常见问题

### Q: 训练需要多大资源？会很久吗？

**不大。** 5 分钟语音素材：

- 显存占用：4~6GB（RTX 3060 足够）
- SoVITS 训练：10~20 分钟
- GPT 训练：20~40 分钟
- 总计约 1 小时，训练期间电脑可正常使用

### Q: Windows 训练的模型能直接在 Linux 用吗？

**可以。** PyTorch 模型文件是跨平台的，直接复制到 Linux 即可使用。保证 Python 版本一致（3.10）就行。

### Q: CPU 模式推理速度如何？

约 5~15 秒/条（取决于文本长度和 CPU 性能）。如果直播场景允许"先显示文字，几秒后播放语音"，CPU 模式完全够用。

### Q: 首次请求很慢？

正常。首次请求需要加载模型到内存（GPU ~5 秒，CPU ~30 秒）。之后请求就快了。建议服务启动后先发一个预热请求：

```bash
curl "http://localhost:5000/?text=預熱&text_language=zh" > /dev/null 2>&1
```

### Q: 如何提升语音质量？

1. 确保参考音频质量高（无噪音、清晰）
2. ASR 标注要手动校对，错误标注会影响效果
3. 增加训练轮次（SoVITS 12、GPT 20）
4. 如果 5 分钟素材效果不够好，补充更多素材（10~30 分钟效果更佳）

### Q: CUDA out of memory（训练时）

降低 batch_size，或：

```bash
export PYTORCH_CUDA_ALLOC_CONF=max_split_size_mb:128
```

### Q: CPU 模式下 "No CUDA GPUs are available" 警告

正常现象。CPU 版 PyTorch 会打印此警告但不影响推理。可以忽略，或在启动前设置：

```bash
export CUDA_VISIBLE_DEVICES=""
```
