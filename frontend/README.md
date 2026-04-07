# 🎨 Yachiyo 前端 - AI 虚拟形象直播应用

**Yachiyo AI 虚拟形象直播平台的前端应用，使用 Vue 3、TypeScript 和 Vite 构建，提供实时虚拟形象展示和与 OpenClaw 助理的交互界面。**

![Version](https://img.shields.io/badge/version-1.0.0-brightgreen)
![Framework](https://img.shields.io/badge/framework-Vue%203-4FC08D)
![Language](https://img.shields.io/badge/language-TypeScript-3178C6)
![Status](https://img.shields.io/badge/status-Production%20Ready-success)

---

---

## 📖 项目概述

前端应用提供：
- ✅ 用户认证和授权界面
- ✅ 实时 AI 聊天体验
- ✅ Live2D 动画集成
- ✅ 用户个人资料管理
- ✅ 内容浏览和管理
- ✅ 响应式设计
- ✅ 多语言支持

---

## 📁 项目结构

```
frontend/
├── src/
│   ├── api/                     # API 调用
│   │   └── client.ts           # Axios 配置 (api + apiV2 实例)
│   ├── components/              # Vue 组件
│   │   ├── AuthDialog.vue      # 认证对话框
│   │   └── Live2DComponent.vue # Live2D 虚拟形象组件
│   ├── composables/             # Vue 组合式函数
│   ├── views/                   # 页面视图
│   │   ├── Home.vue            # 首页
│   │   ├── Chat.vue            # AI 聊天页面
│   │   ├── Profile.vue         # 用户资料页
│   │   ├── Posts.vue           # 帖子列表
│   │   ├── PostDetail.vue      # 帖子详情
│   │   ├── LiveStream.vue      # 直播页面
│   │   ├── Admin.vue           # 管理后台
│   │   └── Admin/              # 管理后台子页面
│   ├── router/                  # 路由配置
│   │   └── index.ts            # 路由定义
│   ├── stores/                  # Pinia 状态管理
│   │   ├── auth.ts             # 认证状态
│   │   └── post.ts             # 帖子状态
│   ├── types/                   # TypeScript 类型
│   │   └── index.ts            # 类型定义
│   ├── styles/                  # 样式文件
│   ├── App.vue                 # 根组件
│   └── main.ts                 # 应用入口
├── public/                      # 静态资源
├── index.html                  # HTML 模板
├── package.json                # 项目依赖配置
├── vite.config.ts              # Vite 构建配置
├── tsconfig.json               # TypeScript 配置
├── .env.example                # 环境变量模板
└── README.md                   # 本文件
```

---

## 🛠 系统要求

- **Node.js**: 16.0.0 或更高
- **npm**: 8.0.0 或更高 (或 yarn 3.0.0+)
- **现代浏览器**: Chrome, Firefox, Safari, Edge

---

## 🚀 快速开始

### 安装依赖

```bash
npm install
# 或
yarn install
```

### 开发模式

```bash
npm run dev
# 或
yarn dev
```

前端将在 `http://localhost:5173` 运行

### 生产构建

```bash
npm run build
# 或
yarn build
```

### 预览构建结果

```bash
npm run preview
# 或
yarn preview
```

---

## ⚙️ 环境配置

创建 `.env.local` 文件：

```env
VITE_API_URL=http://localhost:8080
VITE_APP_NAME=Yachiyo
VITE_LANGUAGE=zh
```

---

## 📦 主要依赖

| 依赖 | 版本 | 用途 |
| ---|---|---|
| Vue | 3.x | 前端框架 |
| TypeScript | 5.x | 类型系统 |
| Vite | 4.x | 构建工具 |
| Pinia | 2.x | 状态管理 |
| Axios | 1.x | HTTP 客户端 |
| Tailwind CSS | 3.x | CSS 框架 |
| Vue Router | 4.x | 路由管理 |

---

## 🎨 核心功能

### 1. 用户认证
- 用户注册
- 用户登录
- 令牌刷新
- 登出功能

### 2. AI 聊天
- 实时消息发送
- WebSocket 连接
- 聊天历史记录
- 多语言支持

### 3. 虚拟助理
- Live2D 集成
- 动画同步
- 表情控制
- 语音播放

### 4. 用户管理
- 个人资料查看
- 信息编辑
- 设置管理
- 账户安全

---

## 💻 开发指南

### 代码规范

```typescript
// ✅ 好的代码
const fetchUserData = async (userId: string): Promise<User> => {
  try {
    const response = await apiClient.get(`/users/${userId}`);
    return response.data;
  } catch (error) {
    handleError(error);
  }
};

// ❌ 避免
const getData = async (id) => {
  let data = fetch(`/users/${id}`);
  return data;
};
```

### 代码格式化

```bash
npm run lint
npm run format
```

---

## 🧪 测试

### 运行测试

```bash
npm run test
```

### 生成覆盖率报告

```bash
npm run test:coverage
```

---

## 🐳 Docker 部署

### 构建 Docker 镜像

```bash
docker build -t yachiyo-frontend:1.0.0 .
```

### 运行容器

```bash
docker run -d \
  -p 80:80 \
  --name yachiyo-web \
  yachiyo-frontend:1.0.0
```

---

## 📊 性能优化

### 已实现的优化

1. **代码分割**: 按需加载组件
2. **懒加载**: 路由级别的代码分割
3. **资源优化**: 图片压缩和 WebP 支持
4. **缓存策略**: 浏览器缓存和 CDN
5. **包体积**: Tree-shaking 和压缩

### 性能指标

```
首屏加载时间: < 2s
可交互时间 (TTI): < 3s
首次内容绘制 (FCP): < 1s
最大内容绘制 (LCP): < 2.5s
```

---

## 🔍 故障排查

### 常见问题

**Q: 无法连接到后端 API**

A: 检查 `.env.local` 中的 `VITE_API_URL` 是否正确

```bash
# 验证后端是否运行
curl http://localhost:8080/api/health
```

**Q: Live2D 模型无法加载**

A: 检查浏览器控制台，确保模型文件路径正确

**Q: 聊天消息无法发送**

A: 检查网络连接和 WebSocket 连接状态

---

## 📚 更多文档

- [API 文档](../docs/API_INTEGRATION_GUIDE.md)
- [部署指南](../docs/DEPLOYMENT_GUIDE.md)
- [代码审核标准](../docs/CODE_REVIEW_STANDARD.md)

---

## 🤝 贡献

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/my-feature`)
3. 提交更改 (`git commit -m "feat: description"`)
4. 推送分支 (`git push origin feature/my-feature`)
5. 创建 Pull Request

---

**最后更新**: 2026-04-03
**版本**: 1.0.0
**状态**: ✅ 功能完整
