# Yachiyo 前端

Vue 3 + TypeScript + Vite 构建的 AI 虚拟形象直播平台前端。

---

## 技术栈

| 依赖 | 用途 |
|------|------|
| Vue 3 (Composition API) | 框架 |
| TypeScript | 类型系统 |
| Vite | 构建工具 + 开发服务器 |
| Element Plus | UI 组件库 |
| Pinia | 状态管理 |
| Vue Router | 路由 |
| Axios | HTTP 客户端 (Token 自动刷新) |
| pixi.js + pixi-live2d-display | Live2D 渲染 |

---

## 快速开始

```bash
npm install
npm run dev     # http://localhost:5173
npm run build   # 生产构建
```

---

## 项目结构

```
src/
├── api/
│   └── client.ts                  # Axios (Token 拦截 + 401 自动刷新)
├── components/
│   ├── AuthDialog.vue             # 登录/注册对话框
│   ├── Live2DComponent.vue        # Live2D 渲染 (pixi-live2d-display)
│   └── live/
│       ├── AvatarStage.vue        # 虚拟形象 + 字幕 + 状态指示
│       └── ChatPanel.vue          # 消息列表 + 输入框 + emoji 选择
├── composables/
│   ├── useWebSocket.ts            # WebSocket 连接 + 消息收发
│   ├── useAudioPlayer.ts          # Web Audio API 播放 + 口型同步
│   ├── useChatMessages.ts         # 消息列表状态管理
│   └── useSubtitle.ts             # 字幕渐进显示逻辑
├── views/
│   ├── Home.vue                   # 首页
│   ├── LiveStream.vue             # 直播间 ⭐ (编排层, ~200行)
│   ├── Chat.vue                   # AI 文字聊天
│   ├── Profile.vue                # 个人资料
│   └── Admin/                     # 管理后台
├── router/                        # 路由配置 (含认证守卫)
├── stores/
│   └── auth.ts                    # Pinia 认证状态
├── styles/                        # 全局样式
├── App.vue                        # 根组件
└── main.ts                        # 入口
```

---

## 核心页面：LiveStream.vue

直播间通过 WebSocket 与后端实时通信：

1. **发送弹幕** → `useWebSocket.sendUserMessage()`
2. **接收广播** → 其他用户的消息 (`user_broadcast`)
3. **接收回复** → AI 回复 (`avatar_response`) → 文本 + 字幕 + 音频 + Live2D 动画
4. **字幕** → `useSubtitle` 根据音频时长逐字刻出
5. **口型同步** → `useAudioPlayer` 通过 `AnalyserNode` 驱动 `ParamMouthOpenY`

### 组件关系

```
LiveStream.vue (编排, 200行)
  ├── AvatarStage.vue     ← Live2DComponent + 字幕覆盖层 + 连接状态
  └── ChatPanel.vue       ← 消息列表 + 输入框 + emoji 选择器
       ├── useChatMessages  (消息状态)
       └── useSubtitle      (字幕逻辑)
```

---

## 认证流程

- Token 存储: `localStorage` + Pinia
- 请求拦截器: 自动附加 `Authorization: Bearer <token>`
- 响应拦截器: 401 → 自动 `/auth/refresh` → 重试
- 路由守卫: 需认证页面未登录 → 重定向首页 + 弹登录框

---

## Live2D 开发模式

`vite.config.ts` 插件自动代理 `/resources/live2d/*` 到项目根目录，开发时无需复制模型文件。

---

## 可替换资源

| 资源 | 路径 | 替换方式 |
|------|------|---------|
| Logo | `public/images/logo.svg` | 直接替换文件 |
| 背景图 | `public/images/bg.jpg` | 直接替换文件 |
| Live2D 模型 | `../resources/live2d/` | 替换模型文件目录 |
