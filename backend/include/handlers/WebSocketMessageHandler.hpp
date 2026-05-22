#pragma once

#include <memory>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

// 前向声明
namespace Yachiyo::Services { class WebSocketService; }
namespace yachiyo::core { struct PipelineContext; }
namespace spdlog { class logger; }

namespace yachiyo::handlers {

/**
 * @brief WebSocket 消息处理器
 *
 * 将 WebSocket 消息处理逻辑从 Application.cpp 中提取出来，
 * 负责认证验证、6层内容审查、消息广播、Avatar 管线调用。
 */
class WebSocketMessageHandler {
public:
    /**
     * @param wsService  WebSocket 服务实例（全局单例）
     * @param pipeline   管线上下文（持有所有服务引用）
     * @param logger     日志器
     */
    WebSocketMessageHandler(
        std::shared_ptr<Yachiyo::Services::WebSocketService> wsService,
        std::shared_ptr<yachiyo::core::PipelineContext> pipeline,
        std::shared_ptr<spdlog::logger> logger
    );

    /**
     * @brief 注册到 WebSocketService 的消息回调
     *
     * 调用此方法后，WebSocket 收到的 user_message 将由此处理器接管。
     */
    void registerCallbacks();

private:
    std::shared_ptr<Yachiyo::Services::WebSocketService> ws_;
    std::shared_ptr<yachiyo::core::PipelineContext> pipeline_;
    std::shared_ptr<spdlog::logger> log_;

    /**
     * @brief 处理单条用户消息
     */
    void handleUserMessage(int64_t clientId, const nlohmann::json& message);

    /**
     * @brief 从消息中提取并验证 userId
     * @return 验证通过返回 userId 字符串，失败返回空
     */
    std::string authenticateAndGetUserId(int64_t clientId, const nlohmann::json& message);

    /**
     * @brief 执行 6 层内容审查 + 持久化
     * @return true=通过, false=被拒绝
     */
    bool moderateAndPersist(int64_t clientId, const std::string& userId,
                            const std::string& text, const std::string& senderName,
                            const std::string& userIp);

    /**
     * @brief 广播用户消息给所有在线客户端
     */
    void broadcastUserMessage(const std::string& userId, const std::string& senderName,
                              const std::string& text);

    /**
     * @brief 调用 Avatar 管线并将结果推送给客户端
     */
    void processAvatarPipeline(int64_t clientId, const std::string& userId,
                               const std::string& text, const std::string& language);

    /**
     * @brief 发送错误消息给指定客户端
     */
    void sendError(int64_t clientId, const std::string& message, const std::string& code);
};

} // namespace yachiyo::handlers
