# Yachiyo 前端

Vue 3 + TypeScript + Vite 构建的 AI 虚拟形象直播平台前端。

---

## 技术栈

| 依赖 | 版本 | 用途 |
| --- | --- | --- |
| Vue 3 | ^3.3.0 | Composition API 框架 |
| TypeScript | ^5.0.0 | 类型系统 |
| Vite | ^4.3.0 | 构建工具 + 开发服务器 |
| Element Plus | ^2.3.0 | UI 组件库 |
| Pinia | ^2.1.0 | 状态管理 |
| Vue Router | ^4.2.0 | 路由 |
| Axios | ^1.4.0 | HTTP 客户端 (Token 自动刷新) |
| pixi.js | ^7.3.0 | 2D 渲染引擎 |
| pixi-live2d-display | ^0.4.0 | Live2D 模型渲染 |

---

## 快速开始

```bash
npm install
npm run dev     # http://localhost:5173
npm run build   # 生产构建
npm run preview # 预览构建结果
```

---

## 项目结构

```
src/
├── api/
│   └── client.ts              # Axios 实例 (Token 拦截器 + 401 自动刷新)
├── components/
│   ├── AuthDialog.vue         # 登录/注册对话框
│   └── Live2DComponent.vue    # Live2D 渲染组件 (pixi-live2d-display)
├── composables/
│   ├── useWebSocket.ts        # WebSocket 连接管理 + 消息处理
│   └── useAudioPlayer.ts      # Web Audio API 音频播放 + 口型同步
├── views/
│   ├── Home.vue               # 首页
│   ├── LiveStream.vue         # 直播间 ⭐ (Live2D + 弹幕 + 字幕 + 音频)
│   ├── Chat.vue               # AI 文字聊天 (HTTP API)
│   ├── Posts.vue              # 内容列表
│   ├── PostDetail.vue         # 内容详情
│   ├── Profile.vue            # 个人资料
│   ├── Admin.vue              # 管理后台
│   └── Admin/
│       ├── Moderation.vue     # 内容审核
│       ├── MessageManagement.vue  # 消息管理
│       └── UserManagement.vue     # 用户管理
├── router/                    # 路由配置 (含认证守卫)
├── stores/
│   ├── auth.ts                # Pinia 认证状态 (token 管理)
│   └── post.ts                # 内容状态
├── types/
│   └── index.ts               # TypeScript 类型定义
├── styles/                    # 全局样式
├── App.vue                    # 根组件 (导航栏 + 路由)
└── main.ts                    # 入口 (createApp + 插件)
```

---

## 核心页面

### LiveStream.vue（直播间）

核心页面，包含：

- **Live2DComponent** — pixi-live2d-display 渲染虚拟形象
- **字幕覆盖层** — 与 TTS 音频同步显示翻译后文本
- **弹幕输入框** — 50 字限制
- **实时消息框** — 所有用户弹幕（昵称 + 文本）
- **音频播放** — useAudioPlayer 通过 Web Audio API 播放 + AnalyserNode 驱动口型

### 认证流程

- Token 存储: `localStorage` + Pinia store
- 请求拦截器: 自动附加 `Authorization: Bearer <token>`
- 响应拦截器: 401 → 自动 `/auth/refresh` → 重试
- 路由守卫: 未登录 → 重定向首页 + 弹出登录框

---

## Live2D 开发模式

`vite.config.ts` 中的 `serve-live2d-resources` 插件自动代理 `/resources/live2d/*` 到项目根目录的 `resources/live2d/`，开发时无需复制模型文件到 `public/`。

---

## 可替换资源

| 资源 | 路径 | 说明 |
| --- | --- | --- |
| Logo | `public/images/logo.svg` | 直接替换 |
| 背景图 | `public/images/bg.jpg` | 放入即生效 |
| Live2D 模型 | `../resources/live2d/` | 替换模型文件 |

---

## 相关文档

- [项目 README](../README.md)
- [架构设计](../docs/架构设计.md) — 前端页面说明、Live2D 集成
- [开发指南](../docs/开发指南.md) — 前端开发规范、添加新页面
- [API 参考](../docs/API参考.md) — HTTP API 端点
