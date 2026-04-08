# Yachiyo API 参考

> 本文档包含所有 HTTP API 端点、WebSocket 端点、数据库表结构和 Redis 数据结构。

---

## 目录

- [认证 API](#认证-api)
- [用户 API](#用户-api)
- [AI 对话 API](#ai-对话-api)
- [消息 API](#消息-api)
- [其他 API](#其他-api)
- [数据库设计](#数据库设计)
- [Redis 数据结构](#redis-数据结构)

---

## 认证 API (`/api/v1/auth`)

| 方法 | 路径 | 说明 | 认证 | 请求体 |
|------|------|------|------|--------|
| POST | `/api/v1/auth/register` | 用户注册 | ❌ | `{ username, email, password }` |
| POST | `/api/v1/auth/login` | 用户登录 | ❌ | `{ username, password }` |
| POST | `/api/v1/auth/refresh` | 刷新 Token | ❌ | `{ refresh_token }` |
| POST | `/api/v1/auth/logout` | 注销登录 | ✅ | - |
| GET | `/api/v1/auth/me` | 获取当前用户信息 | ✅ | - |
| GET | `/api/v1/auth/profile` | 获取当前用户信息 (别名) | ✅ | - |

### 注册请求示例

```json
POST /api/v1/auth/register
{
  "username": "testuser",
  "email": "test@example.com",
  "password": "SecureP@ss123"
}
```

### 登录响应示例

```json
{
  "code": 200,
  "data": {
    "token": "eyJhbGciOi...",
    "refresh_token": "rt_abc123...",
    "user": {
      "id": 1,
      "username": "testuser",
      "email": "test@example.com",
      "role": 1
    }
  }
}
```

---

## 用户 API (`/api/v1/users`)

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/v1/users` | 用户列表 | ✅ |
| GET | `/api/v1/users/:id` | 用户详情 | ✅ |
| PUT | `/api/v1/users/:id` | 更新用户信息 | ✅ |

---

## AI 对话 API (`/api/v2/ai`)

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/v2/ai/chat` | AI 文本对话 | ✅ |
| POST | `/api/v2/ai/tts` | TTS 语音合成 | ✅ |
| POST | `/api/v2/ai/stt` | 语音识别 | ✅ |
| GET | `/api/v2/ai/models` | 可用模型列表 | ✅ |
| GET | `/api/v2/ai/history` | 聊天历史 | ✅ |
| DELETE | `/api/v2/ai/history/:id` | 删除聊天记录 | ✅ |

---

## 消息 API (`/api/v1/messages`)

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/v1/messages` | 消息列表 | ✅ |

---

## 其他 API

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/v1/health` | 健康检查 | ❌ |
| WS | `ws://host:9001` | WebSocket 实时通信 | ✅ (identify) |

---

## 数据库设计

使用 PostgreSQL 15，数据库名为 `yachiyo`，初始化脚本在 `database/init.sql`。

### users 表

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | BIGSERIAL PK | 自增主键 |
| `username` | VARCHAR(50) UNIQUE | 用户名 |
| `email` | VARCHAR(100) UNIQUE | 邮箱 |
| `password_hash` | VARCHAR(255) | SHA-256 密码哈希 |
| `salt` | VARCHAR(32) | 密码盐值 |
| `nickname` | VARCHAR(50) | 昵称 |
| `role` | SMALLINT | 角色 (1=普通用户, 99=管理员) |
| `status` | SMALLINT | 状态 (1=活跃, 2=禁用, 3=封禁) |
| `messages_sent` | BIGINT | 发送消息数 |
| `messages_rejected` | BIGINT | 被拒消息数 |
| `warnings_count` | SMALLINT | 警告次数 |
| `is_banned` | BOOLEAN | 是否封禁 |
| `created_at` | TIMESTAMP | 创建时间 |
| `last_login_at` | TIMESTAMP | 最后登录时间 |
| `last_login_ip` | VARCHAR(45) | 最后登录 IP |

### messages 表

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | BIGSERIAL PK | 自增主键 |
| `user_id` | BIGINT FK | 关联用户 |
| `original_message` | TEXT | 原始消息内容 |
| `review_status` | VARCHAR(20) | 审核状态 (approved/pending_review/rejected) |
| `spam_score` | DECIMAL(5,4) | 垃圾评分 (0.0-1.0) |
| `created_at` | TIMESTAMP | 创建时间 |

### sessions 表

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | BIGSERIAL PK | 自增主键 |
| `user_id` | BIGINT FK | 关联用户 |
| `session_id` | VARCHAR(255) UNIQUE | 会话标识 |
| `access_token` | VARCHAR(1024) | JWT Token |
| `refresh_token` | VARCHAR(1024) | 刷新 Token |

### conversation_contexts 表

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | BIGSERIAL PK | 自增主键 |
| `user_id` | BIGINT FK | 关联用户 |
| `context_data` | JSONB | 对话上下文数据 |
| `message_history` | JSONB | 消息历史 |

---

## Redis 数据结构

| Key Pattern | 类型 | TTL | 说明 |
|-------------|------|-----|------|
| `rate_limit:{userId}` | Counter | 60s | 速率限制计数器 |
| `user_activity:{userId}` | Counter | 300s | 用户活跃度计数 |
| `token_blacklist:{token}` | String | Token有效期 | JWT 黑名单 |
| `cache:translation:*` | String | 3600s | 翻译结果缓存 |
| `cache:tts:*` | String | 3600s | TTS 音频缓存 |
