# 🚀 Yachiyo 项目 - 快速实现指南

**快速参考**: 缺失功能与优先级速查表  
**最后更新**: 2026年4月3日

---

## ⚡ 30秒速览

### 项目现状
- ✅ **完成**: 用户认证、消息管理、6层审查框架
- ❌ **缺失**: OpenClaw集成、语音、动画、WebSocket实时推送
- 📊 **总体完整度**: 48%

### 立即要做的3件事
1. **实现 OpenClaw 网关** (OpenClawGateway.hpp/cpp)
2. **完善 WebSocket 控制器** (WebSocketController.hpp/cpp)
3. **创建虚拱形象响应服务** (AvatarResponseService.hpp/cpp)

---

## 🎯 缺失功能优先级表

### 🔴 第一优先级 - 立即实现 (本周)

| 功能 | 文件 | 工作量 | 关键性 |
|-----|------|--------|--------|
| **OpenClaw集成** | `services/OpenClawGateway.*` | 8h | 🔴🔴🔴 |
| **WebSocket推送** | `controllers/WebSocketController.*` | 5h | 🔴🔴🔴 |
| **虚拱响应服务** | `services/AvatarResponseService.*` | 8h | 🔴🔴🔴 |
| **配置文件** | `config/config.yaml` | 1h | 🔴 |

### 🟡 第二优先级 - 紧接实现 (下周)

| 功能 | 文件 | 工作量 | 关键性 |
|-----|------|--------|--------|
| **翻译服务** | `services/TranslationService.*` | 3h | 🟡🟡 |
| **GPT-SoVITS** | `services/GPTSoVITSService.*` | 4h | 🟡🟡 |
| **Live2D动画** | `services/Live2DAnimationService.*` | 5h | 🟡🟡 |
| **前端Chat视图** | `frontend/views/Chat.vue` | 4h | 🟡🟡 |

### 🟢 第三优先级 - 后续优化 (2周后)

| 功能 | 文件 | 工作量 | 关键性 |
|-----|------|--------|--------|
| **内容审查完善** | `services/ContentModerationService.*` | 6h | 🟢 |
| **缓存优化** | 多个文件 | 3h | 🟢 |
| **性能测试** | 测试套件 | 5h | 🟢 |

---

## 📝 第一阶段核心任务

### 任务 1: OpenClaw 网关 ⏰ 3-4小时

**文件**: `backend/include/services/OpenClawGateway.hpp`

```cpp
class OpenClawGateway {
public:
    // 初始化
    bool connect(const std::string& gatewayUrl, const std::string& apiKey);
    
    // 核心方法
    OpenClawResponse processMessage(const OpenClawRequest& req);
    std::future<OpenClawResponse> processMessageAsync(const OpenClawRequest& req);
    
    // 健康检查
    bool healthCheck();
};
```

**关键步骤**:
1. ✅ 定义 OpenClawRequest/Response 结构 → `dto/OpenClawDTO.hpp`
2. ✅ 实现 HTTP 客户端调用 OpenClaw API
3. ✅ 缓存响应结果 (Redis)
4. ✅ 错误处理与降级方案
5. ✅ 添加配置到 `config.yaml`

---

### 任务 2: WebSocket 控制器 ⏰ 4-5小时

**文件**: `backend/include/controllers/WebSocketController.hpp`

```cpp
class WebSocketController {
private:
    std::map<std::string, std::vector<WebSocketConnection*>> userConnections;
    
public:
    // 连接管理
    void handleNewConnection(const std::string& userId, WebSocketConnection* conn);
    void handleDisconnect(const std::string& userId);
    
    // 推送方法
    void broadcastAvatarResponse(const AvatarResponse& response);
    void streamAnimationFrame(const AnimationKeyframe& frame);
    void broadcastMessage(const Message& msg);
};
```

**关键步骤**:
1. ✅ 定义 WebSocket 消息格式 (JSON)
2. ✅ 实现连接维护与路由
3. ✅ 实现不同类型消息的广播
4. ✅ 处理连接断开与重连
5. ✅ 添加心跳检测

---

### 任务 3: 虚拱形象响应服务 ⏰ 2-3小时

**文件**: `backend/include/services/AvatarResponseService.hpp`

```cpp
class AvatarResponseService {
    // 整合 OpenClaw + 翻译 + 语音 + 动画
    Result<AvatarResponse> generateResponse(
        const OpenClawResponse& openClawResponse,
        int64_t userId
    );
};
```

**处理流程**:
1. ✅ 接收 OpenClaw 输出
2. ✅ 调用翻译服务 (中文 → 日文)
3. ✅ 调用语音合成服务 (GPT-SoVITS)
4. ✅ 转换为动画指令 (Live2D 参数)
5. ✅ 推送给前端

---

## 🔧 配置要点

### backend/config.yaml 必要改动

```yaml
# NEW - OpenClaw 配置
openclaw:
  enabled: true
  gateway_url: "http://localhost:18789"
  api_key: "${OPENCLAW_API_KEY}"
  cache:
    enabled: true
    ttl_seconds: 3600

# NEW - WebSocket 配置
server:
  websocket:
    enabled: true
    path: "/ws/chat"
    max_connections: 1000

# NEW - GPT-SoVITS 配置
gpt_sovits:
  enabled: true
  service_url: "http://localhost:5000"

# NEW - 翻译服务
translation:
  enabled: true
  primary_engine: "openai"
```

---

## 🧪 测试检查清单

### Phase 1 验收标准

- [ ] OpenClaw 网关能成功连接并获取响应
- [ ] WebSocket 连接能正常建立和断开
- [ ] 消息能通过 WebSocket 推送到前端
- [ ] OpenClaw 响应能被正确序列化为 JSON
- [ ] 缓存能正常工作
- [ ] 错误处理有降级方案
- [ ] 日志记录充分
- [ ] 单元测试通过

---

## 📊 进度跟踪

### Week 1 里程碑

```
Mon  │ ████░░░░░  需求分析
Tue  │ ██████░░░  OpenClaw 网关开发
Wed  │ ████████░  WebSocket 控制器开发  
Thu  │ ██████░░░  虚拱响应服务
Fri  │ ██████░░░  集成测试
```

### 关键日期

- **2026-04-04 EOD**: OpenClaw 网关框架完成
- **2026-04-05 EOD**: WebSocket 基础功能完成
- **2026-04-06 EOD**: Phase 1 集成测试通过

---

## 🆘 常见问题

### Q: 为什么优先 OpenClaw 而不是 Live2D?
**A**: OpenClaw 是整个系统的大脑，没有它就无法处理用户消息。Live2D 只是呈现层，可以后续添加。

### Q: WebSocket 和 REST API 的区别?
**A**: 
- REST: 用于登录、历史查询等（请求/响应模式）
- WebSocket: 用于实时消息流、动画帧、语音推送（持久连接）

### Q: OpenClaw 必须本地部署吗?
**A**: 推荐本地部署以获得低延迟和隐私保护。也可以使用云端 OpenClaw 网关。

### Q: 如何处理 OpenClaw 不可用的情况?
**A**: 使用本地模型降级（Ollama）或返回静态回复。

---

## 📚 相关文档

| 文档 | 描述 |
|------|------|
| `MISSING_FEATURES_AND_IMPLEMENTATION_ANALYSIS.md` | 完整的功能分析报告 |
| `OPENCLAW_INTEGRATION_FOR_FRAMEWORK.md` | OpenClaw 集成详解 |
| `backend/README.md` | 后端开发指南 |
| `frontend/README.md` | 前端开发指南 |

---

## 💡 下一步建议

### 今天
- [ ] 读完 `MISSING_FEATURES_AND_IMPLEMENTATION_ANALYSIS.md`
- [ ] 理解 OpenClaw 的正确角色
- [ ] 创建 OpenClawGateway.hpp 框架

### 本周
- [ ] 完成 OpenClaw 网关实现
- [ ] 完成 WebSocket 控制器
- [ ] 完成虚拱响应服务
- [ ] 第一次集成测试

### 下周
- [ ] 实现翻译服务
- [ ] 实现 GPT-SoVITS 集成
- [ ] 实现 Live2D 动画
- [ ] 前端完全改写

---

**这是您的路线图。祝编码顺利！** 🚀
