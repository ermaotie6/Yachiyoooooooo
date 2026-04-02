# 代码修正行动计划

## 🔴 需要删除的文件

这些文件基于对 OpenClaw 的错误理解而创建：

```
❌ YachiyoCPP/include/utils/OpencalwClient.hpp
   - 错误地将 OpenClaw 作为内容审查客户端
   - 应该删除

❌ YachiyoCPP/src/utils/OpencalwClient.cpp
   - 错误的 HTTP 实现
   - 应该删除

❌ YachiyoCPP/OPENCLAW_INTEGRATION.md
   - 基于错误理解的集成指南
   - 应该删除

❌ Yachiyo/OPENCLAW_QUICK_INTEGRATION.md
   - 基于错误理解的快速指南
   - 应该删除

❌ Yachiyo/OPENCLAW_QUICK_START.md
   - 基于错误理解的快速开始
   - 应该删除
```

## 🟡 需要修改的文件

### 1. 更新 README.md 和项目文档

**问题**: 所有文档都提到"Openclaw AI 集成"作为内容审查层

```diff
- ✅ **AI 集成** - Openclaw 框架支持，自动降级
+ ✅ **AI 集成** - [待选定内容审查方案]

- Layer 4 → AI 审查 (Openclaw)
+ Layer 4 → AI 审查 (Perspective API / 本地模型)
```

### 2. 从 CMakeLists.txt 中删除 OpencalwClient

**位置**: `YachiyoCPP/src/utils/CMakeLists.txt`

```diff
- include(utils/OpencalwClient.cpp)  ❌ 删除这行
+ # 内容审查由独立的内容审查服务处理
```

### 3. 从消息服务中移除 OpencalwClient 调用

**位置**: `YachiyoCPP/src/services/ChatService.cpp`

```cpp
// ❌ 错误代码：
if (opencalwClient.available()) {
    auto result = opencalwClient->reviewContent(message);
    if (!result.isAllowed) {
        return error("Content rejected");
    }
}

// ✅ 正确代码：使用真正的内容审查 API
auto moderationResult = moderationService->moderate(message);
if (!moderationResult.isAllowed) {
    return error("Content rejected by moderation");
}
```

### 4. 将 Layer 4 改为真实的内容审查

**新的 Layer 4 实现**（选择一个）：

#### 选项 A: Perspective API（推荐）

```cpp
// include/utils/ModerationClient.hpp
class ModerationClient {
public:
    struct ModerationResult {
        bool isAllowed;
        double score;  // toxicity score 0.0-1.0
        std::vector<std::string> languages;
    };
    
    Result<ModerationResult> moderate(const std::string& text);
};

// 实现调用 Google Perspective API
```

#### 选项 B: 本地 BERT 模型

```cpp
// include/utils/LocalModerationClient.hpp
class LocalModerationClient {
public:
    struct ModerationResult {
        bool isAllowed;
        double toxicityScore;
        double spamScore;
    };
    
    Result<ModerationResult> moderate(const std::string& text);
};
```

## 📋 修改检查清单

### Phase 1: 清理代码（1小时）

- [ ] 删除 `OpencalwClient.hpp`
- [ ] 删除 `OpencalwClient.cpp`  
- [ ] 删除 OPENCLAW_* 文档
- [ ] 从 CMakeLists.txt 中移除引用
- [ ] 从所有服务类中移除 OpencalwClient 调用
- [ ] 编译测试（确保无错误）

### Phase 2: 选择内容审查方案（1天）

**选项 1: Perspective API (推荐)**
- 成本: $5-20/月（低流量）
- 精度: 最高
- 部署: 无需本地部署
- 时间: 最快

**选项 2: 本地 BERT 模型**
- 成本: 0（开源）
- 精度: 较高
- 部署: 需要本地模型服务
- 时间: 中等

**选项 3: 阿里云内容审查**
- 成本: 低（按调用量）
- 精度: 高
- 部署: 云服务
- 时间: 快速

**推荐**: 先用 Perspective API（最简单），后期可迁移到本地模型

### Phase 3: 实现新的 Layer 4（2-3天）

- [ ] 创建 `ModerationClient` 类
- [ ] 集成选定的内容审查 API
- [ ] 实现错误处理和降级
- [ ] 添加缓存机制（Redis）
- [ ] 编写单元测试
- [ ] 集成测试

### Phase 4: 正确集成 OpenClaw（可选，1周）

- [ ] 安装 OpenClaw Gateway
- [ ] 创建虚拟主播代理配置
- [ ] 实现 Yachiyo ↔ OpenClaw 网关 API
- [ ] 测试数据同步
- [ ] 测试多平台消息转发

---

## 🔧 具体代码修改示例

### 修改前（错误）

```cpp
// YachiyoCPP/src/services/ChatService.cpp

#include "utils/OpencalwClient.hpp"

class ChatService {
private:
    std::unique_ptr<OpencalwClient> opencalwClient;
    
public:
    Result<void> sendMessage(const Message& msg) {
        // Layer 4: AI 审查 (错误实现)
        auto review = opencalwClient->reviewContent(msg.content);
        if (!review.isAllowed) {
            // 拒绝消息
        }
        // ...其他逻辑
    }
};
```

### 修改后（正确）

```cpp
// YachiyoCPP/src/services/ChatService.cpp

#include "utils/ModerationClient.hpp"

class ChatService {
private:
    std::unique_ptr<ModerationClient> moderationClient;
    std::shared_ptr<RedisCache> cache;
    
public:
    Result<void> sendMessage(const Message& msg) {
        // Layer 1-3: 快速检查 (现有逻辑)
        if (isRateLimited(msg.userId)) return error("Rate limited");
        if (isIPBlacklisted(msg.ipAddress)) return error("IP blocked");
        if (containsSensitiveWords(msg.content)) return error("Sensitive words");
        
        // Layer 4: AI 审查 (现在正确)
        auto cached = cache->get("moderation:" + md5(msg.content));
        auto modResult = cached ? 
            json::parse(cached.value()) : 
            moderationClient->moderate(msg.content);
            
        if (!modResult.isAllowed) {
            return error("Content rejected", modResult.score);
        }
        
        // Layer 5-6: 人工审核 (现有逻辑)
        if (modResult.score > 0.7) {
            flagForHumanReview(msg);
        }
        
        // 保存消息
        saveMessage(msg);
        return success();
    }
};
```

---

## 📊 迁移时间表

| 任务 | 优先级 | 时间 | 负责人 |
|-----|------|------|-------|
| 删除错误代码 | 🔴 关键 | 1h | 开发者 |
| 选择内容审查方案 | 🔴 关键 | 1d | PM |
| 实现新 Layer 4 | 🔴 关键 | 2-3d | 开发者 |
| 测试和验证 | 🟡 重要 | 2d | QA |
| 部署到测试环境 | 🟡 重要 | 1d | DevOps |
| 部署到生产环境 | 🟡 重要 | 1d | DevOps |
| 实现 OpenClaw 集成 | 🟢 可选 | 1w | 开发者 |

---

## ✅ 验证清单

完成所有修改后，验证以下内容：

- [ ] 项目编译无错误
- [ ] 所有单元测试通过
- [ ] 集成测试通过
- [ ] 内容审查 Layer 4 正常工作
- [ ] 无 OpencalwClient 引用
- [ ] 文档已更新
- [ ] API 文档已更新
- [ ] 性能测试 OK
- [ ] 安全审计 OK

---

## 💡 长期规划

### 短期（1-2 周）
1. ✅ 移除错误的 OpenClaw 代码
2. ✅ 实现真正的内容审查
3. ✅ 上线新的内容审查系统

### 中期（2-4 周）
1. 将 OpenClaw 作为可选功能
2. 允许用户启用虚拟主播的 OpenClaw 代理
3. 实现虚拟主播 → OpenClaw 数据同步

### 长期（1-2 月）
1. 完整的 OpenClaw 集成
2. 多平台虚拟主播活动
3. 虚拟主播自动化技能库

---

## 🆘 需要帮助？

如果需要：
1. **内容审查 API 推荐**: 使用 Perspective API（最简单）
2. **代码审查**: 提交 PR 进行代码审查
3. **测试帮助**: 准备测试数据和场景
