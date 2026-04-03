# Yachiyo 项目功能实现状态 — 代码级深度分析报告

> **生成日期**: 2026-04-03  
> **分析范围**: 全部前端 (18 文件) + 全部后端 (30+ 文件) + 基础设施配置  
> **方法**: 逐文件代码审计，识别真实实现 / Mock桩 / TODO占位

---

## 一、项目架构总览

```
用户浏览器
  ├─ 登录界面 (AuthDialog.vue)
  ├─ 直播间 (LiveStream.vue)
  │   ├─ Live2D 虚拟形象 (Live2DComponent.vue) ← 需要 Cubism SDK
  │   ├─ 实时消息窗口 ← WebSocket
  │   └─ 音频播放 + 口型同步 (useAudioPlayer.ts)
  └─ WebSocket 连接 (useWebSocket.ts)
        ↓
后端 C++ (Crow Framework, 端口 8080)
  ├─ AuthController → AuthServiceImpl → PostgreSQL
  ├─ WebSocketController → WebSocketService (内存管理)
  └─ AvatarResponseService (编排层)
        ├─ ① DeepSeek 内容审核 ← 当前是规则匹配Mock
        ├─ ② OpenClaw 网关 ← 真实HTTP调用 ✅
        ├─ ③ 翻译服务 ← 当前返回原文Mock
        ├─ ④ GPT-SoVITS 语音合成 ← 有endpoint时真实调用
        └─ ⑤ Live2D 动画服务 ← 表情映射真实/口型分析Mock
```

---

## 二、核心业务流程功能状态

### 2.1 主线流程：用户消息 → 虚拟人物响应

| 步骤 | 对应代码文件 | 实现状态 | 具体问题 |
|------|-------------|---------|---------|
| 1. 用户登录 | `AuthDialog.vue` → `AuthController.cpp` → `AuthServiceImpl.cpp` | ⚠️ 基本可用 | JWT签名使用XOR而非HMAC-SHA256(可伪造)；密码用裸SHA256无salt |
| 2. 进入直播间 | `LiveStream.vue` | ⚠️ 部分实现 | 用户ID硬编码为`'user_123'`，未使用真实登录用户 |
| 3. WebSocket连接 | `useWebSocket.ts` → `WebSocketService.cpp` | 🔴 **关键缺失** | 前端WebSocket真实；**后端无网络传输层**，消息只进内存队列不发送 |
| 4. 发送消息 | `LiveStream.vue` → `MessageSecurityService.cpp` | ✅ 基本真实 | 6层安全审查，但"AI审查"层实际是启发式规则(检查大写比例等) |
| 5. 内容审核 | `DeepSeekModerationService.cpp` | 🔴 **完全Mock** | 名叫DeepSeek但零API调用；只做中文敏感词字符串匹配 |
| 6. OpenClaw处理 | `OpenClawGateway.cpp` | ✅ **真实实现** | libcurl HTTP POST到`/process`端点；有缓存/健康检查/错误处理 |
| 7. 翻译服务 | `TranslationService.cpp` | 🔴 **完全Mock** | 三个翻译引擎(Ollama/OpenAI/Google)全返回原文或"未启用" |
| 8. GPT-SoVITS语音 | `GPTSoVITSService.cpp` | ⚠️ 条件性 | 配置endpoint时发真实HTTP请求；未配置时返回假audio URL |
| 9. Live2D动画 | `Live2DComponent.vue` + `Live2DAnimationService.cpp` | 🔴 **前后端均Mock** | 前端用Canvas画简笔画小人；后端口型分析用sin函数模拟 |
| 10. 音频播放+口型同步 | `useAudioPlayer.ts` | ✅ 真实 | Web Audio API频谱分析→mouthOpenY，代码完整 |

### 2.2 主线流程结论

**OpenClaw集成是唯一真正完整的核心环节。** WebSocket后端无网络层、DeepSeek审核是假的、Live2D未集成SDK、翻译全Mock——这4个环节阻断了完整的业务闭环。

---

## 三、逐文件功能实现状态

### 3.1 前端文件 (18个)

#### ✅ 真实实现 (8个)

| 文件 | 功能 | 备注 |
|------|------|------|
| `main.ts` | Vue应用初始化 | `initializeAuth()`未被调用→刷新丢登录态 |
| `App.vue` | 根组件+导航栏 | 缺`/livestream`导航入口 |
| `api/index.ts` | Axios实例+拦截器 | Token刷新无并发锁→可能死循环 |
| `stores/auth.ts` | 认证状态管理 | `initializeAuth`未被调用；响应字段名可能与后端不匹配 |
| `stores/post.ts` | 帖子CRUD | 功能完整 |
| `types/index.ts` | TS类型定义 | `WebSocketMessage`从未使用 |
| `composables/useAudioPlayer.ts` | 音频+口型同步 | 功能完整 |
| `views/ContentView.vue` | 内容列表页 | 功能完整 |

#### ⚠️ 部分实现 (5个)

| 文件 | 功能 | 缺失部分 |
|------|------|---------|
| `router/index.ts` | 路由配置 | 3个Admin子页面文件**不存在**(`ModerationView`/`MessageManagement`/`UserManagement`) |
| `views/ChatView.vue` | AI聊天页 | 调用`/api/v1/ai/chat`但**后端无此端点**；会话历史加载是空壳 |
| `views/PostDetailView.vue` | 帖子详情 | 评论功能是假的(只弹toast不发请求)；评论列表永远为空 |
| `views/LiveStream.vue` | 直播间主页 | 用户ID/用户名硬编码；Live2D props与组件不匹配 |
| `views/AdminView.vue` | 管理后台 | 统计数据硬编码；设置保存只弹toast |

#### 🔴 桩/Mock实现 (3个)

| 文件 | 功能 | 问题 |
|------|------|------|
| `components/Live2DComponent.vue` | Live2D虚拟形象 | **零Live2D SDK集成**，用Canvas 2D画圆形头/椭圆嘴代替 |
| `views/HomeView.vue` | 首页 | 所有统计数据硬编码("10K+用户"等) |
| `views/ProfileView.vue` | 个人资料 | 统计硬编码/帖子列表为空/保存不发请求/头像上传缺失 |

#### 🔴 完全缺失的前端文件

| 文件 | 路由引用位置 | 功能 |
|------|-------------|------|
| `views/Admin/ModerationView.vue` | `/admin/moderation` | 内容审核管理 |
| `views/Admin/MessageManagement.vue` | `/admin/messages` | 消息管理 |
| `views/Admin/UserManagement.vue` | `/admin/users` | 用户管理 |

---

### 3.2 后端服务文件

#### ✅ 真实实现

| 文件 | 功能 | 备注 |
|------|------|------|
| `OpenClawGateway.cpp` | OpenClaw API客户端 | libcurl真实HTTP调用✅ |
| `AuthServiceImpl.cpp` | 注册/登录/JWT | 功能完整(但加密有安全隐患) |
| `DatabaseService.cpp` | PostgreSQL DAO | 4个完整DAO(用户/消息/对话/审核)，参数化查询 |
| `MessageSecurityService.cpp` | 6层消息安全审查 | 速率限制/IP黑名单/敏感词/行为分析→真实Redis+DB |
| `AvatarResponseService.cpp` | 响应编排层 | 正确的服务编排链(审核→AI→翻译→TTS→动画) |
| `RedisUtil.cpp` | Redis客户端 | 完整hiredis封装(SET/GET/发布订阅/连接池) |
| `DatabaseUtil.cpp` | 数据库连接池 | pqxx + 事务支持(但只是单连接非真正池) |
| `JwtUtil.cpp` | JWT工具 | ⚠️ **签名是假的**(XOR替代HMAC-SHA256) |
| `HashUtil.cpp` | 哈希工具 | ⚠️ 裸SHA256无salt(应用bcrypt/argon2) |

#### 🔴 Mock / TODO 实现

| 文件 | 标称功能 | 实际状态 |
|------|---------|---------|
| `DeepSeekModerationService.cpp` | DeepSeek内容审核 | **零API调用**；纯本地敏感词匹配 |
| `WebSocketService.cpp` | WebSocket服务 | 有会话管理/心跳检测，但**无网络传输层** |
| `ChatService.cpp` | 聊天会话管理 | **全部方法返回硬编码假数据**("张三""李四""王五") |
| `UserService.cpp` | 用户信息服务 | **全部方法返回硬编码模拟数据** |
| `AIService.cpp` | AI能力(TTS/STT/图像) | **全部返回`example.com`假URL**；TODO集成Azure |
| `TranslationService.cpp` | 翻译(3引擎) | 框架完整但**Ollama/OpenAI/Google全返回Mock** |
| `Live2DAnimationService.cpp` | Live2D动画控制 | 表情映射真实；**口型分析用sin模拟**(TODO: FFT) |
| `GPTSoVITSService.cpp` | 语音合成 | 有endpoint时真实调用；**健康检查/注册音频/克隆全是TODO** |

#### 后端入口文件问题

| 文件 | 问题 |
|------|------|
| `Application.cpp` | WebSocket启动**被注释掉**；Redis初始化**被注释掉**；AI服务**被注释掉** |
| `Application.hpp` | 头文件声明(Singleton+pImpl)与实现文件(命名空间+直接成员)**完全不匹配** |

---

## 四、基础设施与配置问题

### 4.1 Docker 部署 — 无法启动

| 问题 | 严重性 | 说明 |
|------|--------|------|
| `nginx.conf` 不存在 | 🔴 阻断 | docker-compose引用但文件缺失→Nginx容器启动失败 |
| `prometheus.yml` 不存在 | 🔴 阻断 | Prometheus容器启动失败 |
| `database/init.sql` 不存在 | 🔴 阻断 | compose挂载此文件做DB初始化→数据库为空 |
| 后端`config.yaml`数据库类型写成`mysql` | 🔴 阻断 | 项目用PostgreSQL，端口/用户/密码全错 |
| SQL文件使用MySQL语法 | 🔴 阻断 | `INDEX idx_name (column)`在PostgreSQL中报错 |
| 前端Dockerfile用`npm ci`但无`package-lock.json` | 🔴 阻断 | .gitignore排除了lock文件 |
| 运行阶段未安装curl | ⚠️ 功能 | 健康检查始终失败 |

### 4.2 端口不一致

| 配置来源 | WebSocket端口 |
|---------|--------------|
| `docker-compose.yml` | 8081 |
| `config/config.yaml` | 9001 |
| `backend/config/config.yaml` | 8001 |
| 前端 `useWebSocket.ts` | 9001 |

→ **四处定义三个不同端口**

### 4.3 数据库配置不一致

| 配置来源 | 用户名 | 密码 |
|---------|--------|------|
| `docker-compose.yml` | `postgres` | `postgres` |
| `config/config.yaml` | `yachiyo_app` | `${DB_PASSWORD}` |
| `backend/config/config.yaml` | `root` | `password` |
| `database/init_complete.sql` | `yachiyo_user` | — |

### 4.4 Live2D 资源

**✅ 模型文件存在**: `resources/live2d/八千代辉夜姬/` 下有完整的:
- `八千代辉夜姬.model3.json` (模型定义)
- `八千代辉夜姬.moc3` (模型二进制)
- `八千代辉夜姬.physics3.json` (物理效果)
- 4个表情文件: `泪珠.exp3.json`, `眯眯眼.exp3.json`, `眼泪.exp3.json`, `笑咪咪.exp3.json`
- 8K纹理: `texture_00.png`, `texture_01.png`

**❌ 缺失**: 无动作文件(`.motion3.json`)，但配置中定义了`m_greet`/`m_nod`/`m_wave`等动作

---

## 五、未实现功能清单与实现方法

### 5.1 🔴 P0 — 阻断核心业务流程

#### 1. WebSocket 后端网络传输层

**当前状态**: `WebSocketService.cpp` 只有内存中的会话管理和消息队列，消息放入 `outgoing_queue` 后无代码将其发送到真实的网络socket。

**实现方法**:
- Crow框架本身支持WebSocket。在 `Application.cpp` 中使用 `CROW_WEBSOCKET_ROUTE` 宏注册WebSocket路由
- 将 `crow::websocket::connection&` 存储到 `WebSocketService` 的客户端会话中
- 在 `onmessage` 回调中调用 `handleClientMessage()`
- 启动一个消费线程，轮询 `outgoing_queue` 并通过 `conn.send_text()` 发送
- 需修改文件: `Application.cpp`(添加WS路由), `WebSocketService.hpp/cpp`(添加connection成员)

#### 2. Live2D SDK 前端集成

**当前状态**: `Live2DComponent.vue` 用Canvas 2D手绘简笔画小人代替。

**实现方法**:
- 安装 `pixi.js` (v7) + `pixi-live2d-display` (v0.4+)
- 在 `Live2DComponent.vue` 中:
  ```
  1. 创建 PIXI.Application
  2. 使用 Live2DModel.from('/resources/live2d/八千代辉夜姬/八千代辉夜姬.model3.json')
  3. model.expression('笑咪咪') 设置表情
  4. model.motion('idle') 播放动作
  5. model.internalModel.coreModel.setParameterValueById('ParamMouthOpenY', value) 口型同步
  ```
- 需要将 `resources/live2d/` 目录通过Vite静态资源或public目录暴露给前端
- 表情映射需对齐: 配置中`happy`→实际文件名`笑咪咪.exp3.json`

#### 3. DeepSeek 内容审核 — 真实API集成

**当前状态**: `DeepSeekModerationService.cpp` 中 `callDeepSeekAPI()` 方法完全是本地敏感词匹配。

**实现方法**:
- DeepSeek API端点: `https://api.deepseek.com/v1/chat/completions`
- 使用系统提示词要求模型进行内容审核分析，返回JSON格式的审核结果
- 请求格式:
  ```json
  {
    "model": "deepseek-chat",
    "messages": [
      {"role": "system", "content": "你是内容审核助手。分析以下文本是否包含暴力、色情、仇恨言论等不当内容。返回JSON格式: {\"verdict\": \"pass/review/block\", \"categories\": {...}, \"reason\": \"...\"}"},
      {"role": "user", "content": "<待审核文本>"}
    ],
    "response_format": {"type": "json_object"}
  }
  ```
- 需要 `DEEPSEEK_API_KEY` 环境变量
- 在 `callDeepSeekAPI()` 中使用现有的libcurl基础设施发送请求
- 保留当前的本地规则匹配作为API不可用时的降级方案

**接口说明**:
| 项目 | 值 |
|------|-----|
| 端点 | `https://api.deepseek.com/v1/chat/completions` |
| 方法 | POST |
| 认证 | `Authorization: Bearer <DEEPSEEK_API_KEY>` |
| 模型 | `deepseek-chat` |
| 输入 | 待审核文本(string) |
| 输出 | `{"verdict": "pass|review|block", "categories": {"violence": 0.1, "adult": 0.0, ...}, "reason": "string"}` |
| 超时 | 建议 10s |
| 降级 | API失败时回退到本地敏感词规则 |

#### 4. JWT 安全性修复

**当前状态**: `JwtUtil.cpp` 中的 `hmacSHA256()` 实际是简单XOR操作。

**实现方法**:
- 使用OpenSSL的 `HMAC()` 函数替换:
  ```cpp
  #include <openssl/hmac.h>
  std::string hmacSHA256(const std::string& data, const std::string& key) {
      unsigned char* digest = HMAC(EVP_sha256(), 
          key.c_str(), key.length(),
          (unsigned char*)data.c_str(), data.length(), 
          nullptr, nullptr);
      // 转hex字符串返回
  }
  ```
- 密码哈希改用 `bcrypt` 或 `argon2`:
  ```cpp
  // 使用 bcrypt 库
  std::string hash = bcrypt::generateHash(password, 12); // 12轮
  bool valid = bcrypt::validatePassword(password, hash);
  ```

---

### 5.2 ⚠️ P1 — 重要功能缺失

#### 5. GPT-SoVITS 语音合成 — 完整集成说明

**当前状态**: `GPTSoVITSService.cpp` 在有endpoint时可发真实HTTP请求，但健康检查/声音注册/克隆都是TODO。

**GPT-SoVITS 接口说明**:

GPT-SoVITS 是一个开源语音合成项目，需自行部署。

| 项目 | 值 |
|------|-----|
| 部署方式 | 本地Docker或裸机部署 (https://github.com/RVC-Boss/GPT-SoVITS) |
| API端点 | `http://<host>:9880/` (默认端口9880) |
| 合成接口 | `POST /` 或 `POST /tts` |

**请求格式**:
```json
{
  "text": "你好，我是八千代辉夜姬",
  "text_lang": "zh",
  "ref_audio_path": "/path/to/reference_audio.wav",
  "prompt_text": "参考音频对应的文本",
  "prompt_lang": "zh",
  "text_split_method": "cut5",
  "speed_factor": 1.0
}
```

**响应**: 直接返回WAV音频流 (`Content-Type: audio/wav`)

**集成方式**:
1. 在 `GPTSoVITSService.cpp` 的 `synthesize()` 中:
   - 构建上述JSON请求体
   - 通过libcurl POST到GPT-SoVITS端点
   - 接收二进制WAV数据 → 保存到临时文件或对象存储
   - 返回可访问的音频URL
2. 参考音频: 使用 `resources/live2d/八千代辉夜姬/` 下的音频样本(如有)，或另行录制
3. 情感控制: 通过调整 `speed_factor` 和更换不同情感的参考音频实现

#### 6. 翻译服务实现

**当前状态**: 三个翻译引擎全部返回Mock。

**说明**: 翻译不在OpenClaw范畴内，需独立接口。

**推荐方案 — 百度翻译API** (项目配置中已有):

| 项目 | 值 |
|------|-----|
| 端点 | `https://fanyi-api.baidu.com/api/trans/vip/translate` |
| 方法 | GET/POST |
| 参数 | `q`=文本, `from`=源语言, `to`=目标语言, `appid`=应用ID, `salt`=随机数, `sign`=MD5(appid+q+salt+密钥) |
| 认证 | 百度翻译开放平台申请appid和密钥 |
| 免费额度 | 标准版每月5万字符免费 |

**实现**: 在 `TranslationService.cpp` 的对应引擎方法中用libcurl发送请求。

#### 7. Application.cpp 中被注释掉的服务启动

**当前状态**: WebSocket `run()`、Redis初始化、AI服务实例化均被注释掉。

**实现方法**: 逐个取消注释并确保依赖服务配置正确:
```cpp
// 取消注释这些行:
g_webSocketService->run();   // 需要先实现WS网络层
redisPool = ...;             // 需要Redis服务可用
aiService = ...;             // 需要配置API密钥
```

#### 8. 前端用户ID硬编码

**当前状态**: `LiveStream.vue` 中 `currentUser` 硬编码为 `{id: 'user_123', name: '用户'}`

**实现方法**:
```typescript
import { useAuthStore } from '@/stores/auth'
const authStore = useAuthStore()
const currentUser = computed(() => ({
  id: authStore.user?.id || 'anonymous',
  name: authStore.user?.username || '匿名用户'
}))
```

#### 9. 前端 initializeAuth 未调用

**当前状态**: `stores/auth.ts` 定义了 `initializeAuth()` 从localStorage恢复Token，但`main.ts`未调用。

**实现方法**: 在 `main.ts` 中:
```typescript
const authStore = useAuthStore(pinia)
authStore.initializeAuth()
app.mount('#app')
```

---

### 5.3 🟡 P2 — 功能完善

#### 10. 评论系统

**当前状态**: `PostDetailView.vue` 中 `postComment()` 只显示toast不发请求。

**实现方法**:
- 后端: 添加 `CommentController` + `CommentService`，表结构已在 `init_complete.sql` 中(如有)或需新建
- 前端: `postComment()` 中 `await api.post(\`/posts/${postId}/comments\`, { content })`
- 获取评论: `onMounted` 中 `const { data } = await api.get(\`/posts/${postId}/comments\`)`

#### 11. 用户资料页真实数据

**当前状态**: `ProfileView.vue` 统计/帖子/保存全是假的。

**实现方法**:
- 后端 `UserService.cpp`: 将Mock替换为真实的 `DatabaseService` 查询
- 前端: `onMounted` 中调用 `GET /api/v1/users/profile` 获取真实数据

#### 12. Admin子页面

**当前状态**: 路由引用的3个文件不存在。

**实现方法**: 创建以下文件:
- `frontend/src/views/Admin/ModerationView.vue` — 内容审核列表+操作
- `frontend/src/views/Admin/MessageManagement.vue` — 消息管理
- `frontend/src/views/Admin/UserManagement.vue` — 用户管理(封禁/角色)

#### 13. ChatView 修复

**当前状态**: 调用不存在的API端点 `/api/v1/ai/chat`。

**实现方法**:
- 后端已有 `/api/v1/chat/send`(由ChatController注册)，前端需改为调用此端点
- 或: 将ChatView改造为通过WebSocket通信(与直播间一致)

#### 14. ChatService.cpp 真实实现

**当前状态**: 全部6个方法返回硬编码数据。

**实现方法**: 使用已存在的 `DatabaseService` 中的 `ConversationDAO` 和 `MessageDAO` 替换Mock数据。

#### 15. UserService.cpp 真实实现

**当前状态**: 全部方法返回硬编码数据。

**实现方法**: 使用 `DatabaseService` 中的 `UserDAO` 进行真实数据库查询。

---

### 5.4 🟢 P3 — 基础设施修复

#### 16. Docker部署修复

需要:
- 创建 `nginx.conf` 文件(反向代理+静态资源)
- 创建 `backend/config/prometheus.yml`
- 创建正确的 `database/init.sql` (合并两套schema，改为PostgreSQL语法)
- 修复 `backend/config/config.yaml` 数据库类型为`postgresql`，端口为`5432`
- 统一WebSocket端口为一个值(建议8081)
- 统一数据库用户名密码
- 前端Dockerfile改 `npm ci` 为 `npm install`
- 后端Dockerfile运行阶段添加 `curl` 安装

#### 17. SQL Schema 修复

- 合并 `schema.sql` 和 `init_complete.sql` 为一套
- 将 `INDEX idx_name (column)` 改为 `CREATE INDEX idx_name ON table(column)`
- 统一用户表结构

---

## 六、关于 OpenClaw 的重要说明

**OpenClaw 不是 AI 模型，而是一款可集成 AI Agent 的自主人工智能虚拟助理软件。**

- OpenClaw 可部署在本地设备上
- 能够调用其他 AI 大模型与应用程序接口（API）
- 在本地存储配置数据和交互历史，拥有持久记忆能力
- 不仅是对话式聊天机器人，而是可代替用户执行任务的自主智能助理
- 详情请查看 OpenClaw 官网

**在本项目中的角色**: OpenClaw 作为核心编排引擎，接收用户消息文本，调用自身集成的AI能力进行处理，输出包含**表情提示词的响应文本**和**基本动作命令**，由后端网关(`OpenClawGateway.cpp`)通过HTTP与之通信。

**当前集成方式** (`OpenClawGateway.cpp`):
```
请求: POST <openclaw_endpoint>/process
{
  "request_id": "req_user123_1712345678",
  "text": "用户输入的消息",
  "context": "对话上下文",
  "emotion_hints": ["happy", "excited"],
  "max_tokens": 1000,
  "temperature": 0.7
}

响应:
{
  "text": "OpenClaw生成的回复文本(含表情提示词)",
  "emotions": ["happy", "surprised"],
  "actions": ["wave", "nod"],
  "animation_commands": ["m_greet"]
}
```

---

## 七、实现优先级总结

```
P0 (必须先做，否则核心流程不通):
  ├── WebSocket 后端网络传输层
  ├── Live2D SDK 前端集成 (pixi-live2d-display)
  ├── DeepSeek 内容审核真实 API 调用
  └── JWT/密码安全修复

P1 (核心功能完善):
  ├── GPT-SoVITS 完整集成
  ├── 翻译服务实现
  ├── Application.cpp 取消注释启用服务
  ├── 前端用户ID从auth store获取
  └── initializeAuth 调用修复

P2 (业务功能补全):
  ├── 评论系统
  ├── 用户资料真实数据
  ├── Admin 3个子页面
  ├── ChatView API端点修复
  ├── ChatService 真实数据库实现
  └── UserService 真实数据库实现

P3 (基础设施):
  ├── Docker 配置修复 (7项)
  ├── SQL Schema 合并修复
  └── 端口/数据库配置统一
```

---

> **文档结束** | 生成工具: 代码级逐文件审计 | 2026-04-03
