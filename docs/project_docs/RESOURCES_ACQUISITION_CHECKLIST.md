# 📦 Yachiyo 项目 - 外部资源获取清单

**优先级**: 🔴 立即行动  
**完成时间**: 3-5 天  
**阻塞项**: 参考音源

---

## 🎯 优先级排序

```
🔴 必须立即获取 (没有这些无法运行)
├─ 参考音源 (yachiyo_reference_voice.wav)
├─ PostgreSQL 数据库
├─ GPT-SoVITS 服务
└─ OpenAI API Key

🟡 需要部署 (可用本地模拟暂时替代)
├─ Redis 缓存
└─ OpenClaw 服务
```

---

## 📋 资源获取指南

### 1️⃣ 参考音源 🎤 [**最关键**]

**为什么需要**: GPT-SoVITS 通过参考音源学习虚拱的声音特征，没有它无法生成正确的音色。

#### 获取方案对比

| 方案 | 时间 | 难度 | 质量 | 成本 | 推荐 |
|------|------|------|------|------|------|
| Live2D 官方资源提取 | 1 小时 | 简单 | 高 | 0 | ⭐⭐⭐⭐ |
| Google Translate TTS | 30 分钟 | 简单 | 中 | 0 | ⭐⭐⭐ |
| 百度 AI 语音合成 | 30 分钟 | 简单 | 中 | 0 | ⭐⭐⭐ |
| 声优专业录制 | 3-7 天 | 复杂 | 极高 | ¥500-2000 | ⭐⭐ |
| 动画/游戏音源转换 | 2-3 天 | 复杂 | 中 | 0 | ⭐⭐ |

#### 方案 A: Live2D 官方资源 ⭐⭐⭐⭐ [**推荐**]

```bash
# 步骤 1: 找到角色的 Live2D 资源包
# 如果你有 Live2D 官方的八千代辉夜姬资源包
# 里面可能包含参考音源或关联的音频文件

# 步骤 2: 提取音频
# 在 Live2D 文件夹中寻找:
# - *.wav 文件
# - *.mp3 文件
# - audio/ 目录

# 步骤 3: 转换为标准格式
ffmpeg -i original_audio.wav \
  -acodec pcm_s16le \
  -ar 22050 \
  yachiyo_reference_voice.wav

# 步骤 4: 验证
ffmpeg -i yachiyo_reference_voice.wav
# 应该看到: Duration: 0:00:08 (8 秒)
#          Sample rate: 22050 Hz
```

#### 方案 B: Google Translate TTS ⭐⭐⭐ [**最简单**]

```bash
# 工具: google-cloud-text-to-speech 或在线 TTS

# 步骤 1: 选择日语女性声音

# 步骤 2: 合成 3-5 句日语
# 例句:
# - "おはようございます"
# - "いつもありがとうございます"
# - "皆さん、お疲れ様です"
# - "また明日ね"

# 步骤 3: 拼接音频 (5-10 秒总长)
ffmpeg -i audio1.wav -i audio2.wav -i audio3.wav \
  -filter_complex "[0][1][2]concat=n=3:v=0:a=1[out]" \
  -map "[out]" \
  -acodec pcm_s16le \
  -ar 22050 \
  yachiyo_reference_voice.wav

# 示例代码 (Python)
from google.cloud import texttospeech

client = texttospeech.TextToSpeechClient()
synthesis_input = texttospeech.SynthesisInput(text="おはようございます")
voice = texttospeech.VoiceSelectionParams(
    language_code="ja-JP",
    name="ja-JP-Standard-C"  # 女性声音
)
audio_config = texttospeech.AudioConfig(
    audio_encoding=texttospeech.AudioEncoding.LINEAR16,
    sample_rate_hertz=22050
)
response = client.synthesize_speech(
    input=synthesis_input, voice=voice, audio_config=audio_config
)

with open("output.wav", "wb") as out:
    out.write(response.audio_content)
```

#### 方案 C: 百度 AI 语音合成 ⭐⭐⭐

```bash
# 费用: 免费 (每月 10,000 字免费额度)

# 步骤 1: 注册百度 AI 开放平台
# https://ai.baidu.com

# 步骤 2: 创建应用获取 API Key

# 步骤 3: 调用 TTS API
import requests

def baidu_tts(text):
    url = "https://tsn.baidu.com/text2audio"
    params = {
        "tex": text,
        "token": YOUR_TOKEN,
        "cuid": "your_client_id",
        "langu": "zh",
        "spd": "5",  # 速度
        "pit": "5",  # 音高
        "vol": "5",  # 音量
        "per": "1",  # 发音人 (1=女性)
    }
    response = requests.get(url, params=params)
    if response.status_code == 200:
        with open("audio.wav", "wb") as f:
            f.write(response.content)

# 步骤 4: 拼接多个音频成 8-10 秒
```

#### 方案 D: 专业声优录制 ⭐⭐ [**最好但耗时**]

```bash
# 费用: ¥500-2000
# 时间: 3-7 天

# 步骤 1: 准备台词脚本
scripts = [
    "おはようございます",
    "皆さん、こんにちは",
    "いつもありがとうございます",
    "また明日ね"
]

# 步骤 2: 找声优录制
# 可以通过:
# - 专业配音工作室
# - 自由职业平台 (Fiverr, Upwork)
# - 本地日语学校推荐

# 步骤 3: 获取录音文件
# - 格式: WAV
# - 采样率: 22050 Hz
# - 总时长: 8-10 秒

# 步骤 4: 剪辑和处理
ffmpeg -i raw_recording.wav \
  -af "aecho=0.8:0.9:1000:0.3" \  # 可选: 添加混响
  -acodec pcm_s16le \
  -ar 22050 \
  yachiyo_reference_voice.wav
```

#### 验证参考音源

```bash
# 确保文件符合规格
ffmpeg -i yachiyo_reference_voice.wav

# 输出应该包含:
# Duration: 0:00:08.XX  (8 秒左右)
# Sample rate: 22050 Hz
# Channels: mono (单声道)
# PCM signed 16-bit

# 如果不符合, 进行转换:
ffmpeg -i input_audio.wav \
  -acodec pcm_s16le \
  -ar 22050 \
  -ac 1 \
  yachiyo_reference_voice.wav
```

**立即行动**: 选择上述任一方案，今日内完成

---

### 2️⃣ PostgreSQL 数据库 🗄️

**所需时间**: 30 分钟  
**难度**: 简单  
**成本**: 0 (开源) 或 ¥50-200/月 (云服务)

#### 方式 A: Docker 本地部署 ✅ [**推荐**]

```bash
# 步骤 1: 拉取镜像
docker pull postgres:15

# 步骤 2: 启动容器
docker run --name yachiyo-postgres \
  -e POSTGRES_PASSWORD=password \
  -e POSTGRES_DB=yachiyo_db \
  -p 5432:5432 \
  -v yachiyo_db_data:/var/lib/postgresql/data \
  -d postgres:15

# 步骤 3: 等待启动 (30 秒)
sleep 30

# 步骤 4: 验证连接
psql -h localhost -U postgres -d yachiyo_db -c "SELECT 1;"

# 步骤 5: 初始化数据库
psql -h localhost -U postgres -d yachiyo_db \
  -f config/avatar_database_migration.sql

# 步骤 6: 验证表创建
psql -h localhost -U postgres -d yachiyo_db -c "\dt"

# 应该看到:
# messages, avatar_responses, avatar_actions, 等 10 张表
```

#### 方式 B: 云数据库服务

```
AWS RDS PostgreSQL:
- 费用: ¥300-500/月
- 优点: 自动备份, 高可用
- URL: https://aws.amazon.com/rds/

Azure Database:
- 费用: ¥250-400/月
- 优点: 与 Azure 集成
- URL: https://azure.microsoft.com/

阿里云 RDS:
- 费用: ¥200-300/月
- 优点: 国内速度快
- URL: https://www.aliyun.com/

配置连接字符串:
postgresql://postgres:password@host:5432/yachiyo_db
```

**立即行动**: 使用 Docker 方式，10 分钟完成

---

### 3️⃣ GPT-SoVITS 服务 🎙️

**所需时间**: 1-2 小时  
**难度**: 中等  
**成本**: 0 (开源)

#### 部署步骤

```bash
# 步骤 1: 克隆项目 (10 分钟)
git clone https://github.com/RVC-Boss/GPT-SoVITS.git
cd GPT-SoVITS

# 步骤 2: 创建虚拟环境 (5 分钟)
python3 -m venv venv
source venv/bin/activate  # Linux/Mac
# 或
venv\Scripts\activate  # Windows

# 步骤 3: 安装依赖 (20-30 分钟)
pip install -r requirements.txt

# 如果遇到问题, 尝试:
pip install --upgrade pip
pip install torch torchaudio -f https://download.pytorch.org/whl/torch_stable.html

# 步骤 4: 下载模型 (30-45 分钟)
# 首次会自动下载 ~2GB 模型
python3 download_models.py

# 如果手动下载:
# 访问: https://huggingface.co/RVC-Boss/GPT-SoVITS
# 下载模型到 models/ 目录

# 步骤 5: 启动服务 (启动)
python3 app.py \
  --server_name 0.0.0.0 \
  --server_port 9000 \
  --share False \
  --no_gradio_queue True

# 输出应该包含:
# Running on http://0.0.0.0:9000
# Uvicorn running on http://127.0.0.1:9000

# 步骤 6: 验证服务 (在另一个终端)
curl http://localhost:9000/api/v1/status

# 应该返回:
# {"status": "ok", "version": "..."}
```

#### 配置持久化

```bash
# 为了服务启动后能在后台运行, 使用 systemd (Linux)

# 创建服务文件
sudo nano /etc/systemd/system/gpt-sovits.service

# 内容:
[Unit]
Description=GPT-SoVITS Voice Synthesis Service
After=network.target

[Service]
Type=simple
User=your_user
WorkingDirectory=/path/to/GPT-SoVITS
ExecStart=/path/to/GPT-SoVITS/venv/bin/python3 app.py \
  --server_port 9000
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target

# 启用服务
sudo systemctl enable gpt-sovits
sudo systemctl start gpt-sovits
sudo systemctl status gpt-sovits
```

#### 验证正常工作

```bash
# 测试文本转语音
curl -X POST http://localhost:9000/api/v1/synthesis \
  -H "Content-Type: application/json" \
  -d '{
    "text": "おはようございます",
    "language": "ja",
    "reference_audio_path": "/path/to/yachiyo_reference_voice.wav"
  }'

# 应该返回音频文件数据
```

**立即行动**: 今日部署，明天早上验证

---

### 4️⃣ OpenAI API Key 🔑

**所需时间**: 10 分钟  
**难度**: 简单  
**成本**: 按用量计费 (~$0.0005/次调用)

#### 获取步骤

```bash
# 步骤 1: 访问 OpenAI 官网
https://platform.openai.com/account/api-keys

# 步骤 2: 登录或注册
# - 需要邮箱 + 手机号验证

# 步骤 3: 点击 "Create new secret key"

# 步骤 4: 复制生成的 API Key
# 格式: sk-proj-xxxxxxxxxxxxxxxxxxxxxxxxxxxx

# 步骤 5: 保存到配置文件
# avatar_config.yaml
translation:
  engine: "openai"
  openai:
    api_key: "sk-proj-xxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    model: "gpt-3.5-turbo"
```

#### 成本估算

```
翻译成本:
- 单条翻译: 50 tokens → $0.00001
- 每日翻译 1000 条: $0.01
- 每月 30,000 条: $0.30

聊天成本:
- 单条对话: 200 tokens → $0.00004
- 每日对话 500 条: $0.02
- 每月 15,000 条: $0.60

总月度预算建议:
- 低用量: $5 (充足)
- 中等用量: $10-20 (推荐)
- 高用量: $50+

免费额度:
- 新用户: $5 (3 个月内有效)
```

#### 设置费用限制

```
1. 访问: https://platform.openai.com/account/billing/limits
2. 设置月度上限: 例如 $20
3. 设置硬限制: 防止超支
4. 启用通知: 达到 80% 时提醒
```

**立即行动**: 立即申请，10 分钟完成

---

### 5️⃣ Redis 缓存 ⚡

**所需时间**: 10 分钟  
**难度**: 简单  
**成本**: 0 (开源) 或 ¥20-50/月 (云服务)

#### Docker 本地部署

```bash
# 启动 Redis
docker run --name yachiyo-redis \
  -p 6379:6379 \
  -v redis_data:/data \
  -d redis:7

# 验证连接
redis-cli ping

# 应该返回: PONG
```

**立即行动**: 10 分钟完成

---

### 6️⃣ OpenClaw 服务 🤖

**所需时间**: 1-2 小时  
**难度**: 中等  
**成本**: 0 (开源框架)

#### 安装步骤

```bash
# 步骤 1: 全局安装 (2 分钟)
npm install -g openclaw@latest

# 步骤 2: 初始化 (5 分钟)
openclaw onboard --install-daemon

# 步骤 3: 获取 API Key
# 访问 https://openclaw.io
# 注册并创建应用
# 复制 API Key

# 步骤 4: 配置环境变量 (5 分钟)
export OPENCLAW_API_KEY="your_key_here"
export OPENCLAW_PROFILE_ID="yachiyo_avatar_001"

# 步骤 5: 启动网关 (5 分钟)
openclaw gateway --start

# 验证
openclaw status

# 应该看到:
# Gateway: running
# Status: healthy
```

**立即行动**: 可选，后续增强功能

---

## ✅ 完整获取清单

```
第一天 (今日):
□ 参考音源获取          预计 1-2 小时
□ PostgreSQL 部署       预计 30 分钟
□ OpenAI API Key 申请   预计 10 分钟
□ Redis 部署            预计 10 分钟
─────────────────────────────────
小计: 2-3 小时

第二天 (明日):
□ GPT-SoVITS 部署       预计 1-2 小时
□ OpenClaw 部署         预计 1-2 小时
─────────────────────────────────
小计: 2-4 小时

第三天:
□ 编译后端              预计 30 分钟
□ 初始化数据库          预计 10 分钟
□ 启动服务              预计 5 分钟
□ 集成测试              预计 1-2 小时
─────────────────────────────────
小计: 2-3 小时
```

---

## 📊 资源依赖图

```
Yachiyo 系统
├── 参考音源 ❌ 缺失
│   └── GPT-SoVITS 服务 ❌ 需部署
│       └── 语音合成 ✅ 代码就绪
│
├── OpenAI API Key ❌ 需申请
│   └── 翻译服务 ✅ 代码就绪
│
├── PostgreSQL ❌ 需部署
│   └── 数据持久化 ✅ 代码就绪
│
├── Redis ⚠️ 可选部署
│   └── 缓存加速 ✅ 代码就绪
│
├── OpenClaw 服务 ❌ 需部署
│   └── 虚拱自动回复 ✅ 代码就绪
│
└── Live2D 模型 ✅ 已就绪
    └── 虚拱渲染 ✅ 代码就绪
```

---

## 🚨 关键提醒

1. **参考音源最关键** - 没有它，GPT-SoVITS 无法工作
2. **不要共享 API Key** - 防止账户被盗用和滥用
3. **设置 API 限额** - 防止意外扣费
4. **定期备份数据库** - 使用 `pg_dump`
5. **监控服务健康** - 定期检查所有服务状态

---

## 📞 获取帮助

如果遇到问题:

1. **参考音源问题**
   - 参考: COMPLETENESS_QA_ANSWERS.md (第 5 部分)

2. **GPT-SoVITS 部署**
   - 官方文档: https://github.com/RVC-Boss/GPT-SoVITS
   - Issues: https://github.com/RVC-Boss/GPT-SoVITS/issues

3. **OpenAI API**
   - 官方文档: https://platform.openai.com/docs
   - 配额管理: https://platform.openai.com/account/billing/overview

4. **OpenClaw**
   - 官方网站: https://openclaw.io
   - 文档: https://docs.openclaw.io

---

**启动时间**: 立即  
**完成时间**: 3-5 天  
**预计上线**: 一周内
