# Yachiyo Web 前端应用

现代化的Vue 3 + TypeScript虚拟主播平台Web前端。

## 📋 项目结构

```
YachiyoWeb/
├── src/
│   ├── components/           # Vue 组件
│   │   └── AuthDialog.vue    # 认证对话框
│   ├── views/                # 页面视图
│   │   ├── Home.vue          # 首页
│   │   ├── Posts.vue         # 内容列表
│   │   ├── PostDetail.vue    # 内容详情
│   │   ├── Chat.vue          # AI聊天
│   │   ├── Profile.vue       # 个人资料
│   │   └── Admin.vue         # 管理后台
│   ├── router/               # 路由配置
│   │   └── index.ts          # 路由定义
│   ├── stores/               # Pinia 状态管理
│   │   ├── auth.ts           # 认证状态
│   │   └── post.ts           # 内容状态
│   ├── api/                  # API 客户端
│   │   └── client.ts         # Axios 配置
│   ├── types/                # TypeScript 类型定义
│   │   └── index.ts          # 类型声明
│   ├── styles/               # 全局样式
│   │   └── main.css          # 主样式文件
│   ├── App.vue               # 根组件
│   └── main.ts               # 应用入口
├── index.html                # HTML 模板
├── package.json              # 项目配置
├── vite.config.ts            # Vite 配置
├── tsconfig.json             # TypeScript 配置
└── README.md                 # 本文件
```

## 🚀 快速开始

### 前置要求

- Node.js >= 16.0.0
- npm >= 8.0.0 或 yarn >= 3.0.0

### 安装依赖

```bash
cd YachiyoWeb
npm install
# 或
yarn install
```

### 开发服务

```bash
npm run dev
# 或
yarn dev
```

应用将在 `http://localhost:5173` 启动。

### 构建生产版本

```bash
npm run build
# 或
yarn build
```

### 预览生产构建

```bash
npm run preview
```

## 📁 核心功能模块

### 1. 认证系统 (Authentication)

**相关文件**: `stores/auth.ts`, `components/AuthDialog.vue`

功能特性：
- 用户注册与登录
- JWT 令牌管理
- 自动令牌刷新
- 登出功能

### 2. 内容管理 (Content Management)

**相关文件**: `stores/post.ts`, `views/Posts.vue`, `views/PostDetail.vue`

功能特性：
- 浏览内容列表（支持分页）
- 发布新内容
- 查看内容详情
- 点赞和收藏
- 内容评论

### 3. AI 聊天 (Chat)

**相关文件**: `views/Chat.vue`

功能特性：
- 实时AI对话
- 会话历史管理
- 流式响应显示
- 多会话支持

### 4. 用户资料 (Profile)

**相关文件**: `views/Profile.vue`

功能特性：
- 查看个人信息
- 编辑用户资料
- 我的内容统计
- 发布历史

### 5. 管理后台 (Admin)

**相关文件**: `views/Admin.vue`

功能特性：
- 待审核内容管理
- 平台统计数据
- 系统设置配置

## 🔗 API 集成

Web前端通过Axios与后端API通信。所有请求自动包含JWT认证头。

### 环境配置

创建 `.env.local` 文件（开发环境）:

```bash
VITE_API_BASE_URL=http://localhost:8080/api/v1
```

### API 端点映射

| 功能 | 方法 | 端点 |
|------|------|------|
| 注册 | POST | `/auth/register` |
| 登录 | POST | `/auth/login` |
| 获取内容 | GET | `/posts` |
| 发布内容 | POST | `/posts` |
| 获取内容详情 | GET | `/posts/:id` |
| AI聊天 | POST | `/chat` |
| 获取资料 | GET | `/users/profile` |
| 管理后台 | GET | `/admin/*` |

详见: `API_INTEGRATION_GUIDE.md`

## 🎨 UI 框架

- **Element Plus**: 企业级UI组件库
- **自定义样式**: 基于Element Plus主题定制

### 主题色

```css
主色: #667eea
辅色: #764ba2
成功: #67c23a
警告: #e6a23c
危险: #f56c6c
```

## 📦 依赖管理

### 核心依赖

| 包名 | 版本 | 用途 |
|------|------|------|
| Vue | ^3.3.0 | UI框架 |
| Vue Router | ^4.2.0 | 路由管理 |
| Pinia | ^2.1.0 | 状态管理 |
| Axios | ^1.4.0 | HTTP客户端 |
| Element Plus | ^2.3.0 | UI组件库 |

### 开发依赖

| 包名 | 版本 | 用途 |
|------|------|------|
| Vite | ^4.3.0 | 构建工具 |
| TypeScript | ^5.0.0 | 类型检查 |
| Vue TSC | ^1.8.0 | Vue类型检查 |

## 🔐 认证流程

1. 用户在登录对话框输入凭证
2. 前端调用 `/auth/login` 获取令牌
3. 令牌保存到本地存储和Pinia状态
4. 每个API请求自动附加 `Authorization: Bearer <token>` 头
5. 令牌过期时自动调用 `/auth/refresh` 刷新
6. 刷新失败则清除认证状态并重定向到首页

## 🗂️ 状态管理

### Auth Store (`stores/auth.ts`)

```typescript
// 核心状态
user: User | null           // 当前用户
accessToken: string         // 访问令牌
refreshToken: string        // 刷新令牌
isLoggedIn: boolean         // 是否已登录

// 核心方法
login(username, password)   // 用户登录
register(...)              // 用户注册
logout()                   // 用户注销
refreshAccessToken()       // 刷新令牌
```

### Post Store (`stores/post.ts`)

```typescript
// 核心状态
posts: Post[]              // 内容列表
currentPost: Post | null   // 当前内容
loading: boolean           // 加载状态

// 核心方法
fetchPosts()              // 获取内容列表
fetchPostDetail()         // 获取内容详情
createPost()              // 创建内容
likePost()                // 点赞
favoritePost()            // 收藏
```

## 🧪 测试

### 单元测试（待补充）

```bash
npm run test
```

### E2E 测试（待补充）

```bash
npm run test:e2e
```

## 🌍 浏览器支持

- Chrome (最新)
- Firefox (最新)
- Safari (最新)
- Edge (最新)

## 📝 环境变量

创建 `.env.local` 文件：

```bash
# API 基础URL
VITE_API_BASE_URL=http://localhost:8080/api/v1

# 开发环境调试
VITE_DEBUG=false

# 应用标题
VITE_APP_TITLE=Yachiyo
```

## 🐛 故障排除

### 无法连接到后端API

检查：
1. 后端服务是否运行（`http://localhost:8080`）
2. CORS 配置是否正确
3. 环境变量 `VITE_API_BASE_URL` 是否设置

### 令牌过期问题

1. 清除本地存储: `localStorage.clear()`
2. 重新登录
3. 检查后端令牌过期时间配置

### 样式不显示

```bash
# 清除node_modules和缓存
rm -rf node_modules
npm install

# 重新启动开发服务
npm run dev
```

## 📚 相关文档

- [后端API集成指南](../YachiyoCPP/API_INTEGRATION_GUIDE.md)
- [快速参考卡](../YachiyoCPP/WEB_OPENCLAW_INTEGRATION_QUICK_REFERENCE.md)
- [项目完成报告](../YachiyoCPP/PROJECT_COMPLETION_SUMMARY.md)

## 📄 许可证

MIT License

## 👥 贡献

欢迎提交Issue和Pull Request！

---

**版本**: 2.0.0
**最后更新**: 2024-01-15
**状态**: ✅ 功能完整
