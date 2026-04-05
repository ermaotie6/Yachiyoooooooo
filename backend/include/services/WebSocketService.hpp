#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <vector>
#include <atomic>
#include <nlohmann/json.hpp>
#include <crow.h>

using json = nlohmann::json;

namespace Yachiyo::Services {

// 客户端元数据
struct ClientMetadata {
    std::string device_type;      // web, mobile, desktop
    std::string app_version;
    std::string user_agent;
    json extra_data;
};

// 客户端会话 —— 每个 WebSocket 连接对应一个 ClientSession
struct ClientSession {
    int64_t client_id;
    std::string user_id;
    ClientMetadata metadata;
    std::chrono::system_clock::time_point connected_at;
    std::chrono::system_clock::time_point last_heartbeat;
    bool is_active;
    crow::websocket::connection* conn;   // Crow WebSocket 连接指针
};

// Avatar 响应（从 AvatarResponseService 推送到前端）
struct AvatarResponse {
    std::string request_id;
    std::string text;
    std::string audio_url;
    int64_t audio_duration_ms;
    std::vector<std::string> emotions;
    std::vector<std::string> actions;
    std::vector<json> animation_commands;
};

// 状态更新
struct StatusUpdate {
    std::string status;  // processing, completed, error, etc.
    int progress;        // 0-100
    std::string message;
    json details;
};

// 通知
struct Notification {
    std::string level;   // info, warning, error
    std::string title;
    std::string message;
    json details;
};

// 统计信息
struct Statistics {
    size_t total_clients;
    size_t active_clients;
    size_t total_messages_sent;
    size_t total_messages_received;
};

/**
 * @brief WebSocket 服务
 * 
 * 基于 Crow WebSocket 的实时通信服务。
 * 在独立端口（默认 9001）运行一个 Crow 实例，接受浏览器 WebSocket 连接。
 * 前端通过 ws://host:port?user_id=xxx 连接。
 * 
 * 职责：
 * - 管理 WebSocket 客户端连接的生命周期
 * - 将 AvatarResponse / StatusUpdate / Notification 实时推送给前端
 * - 心跳检测与空闲超时清理
 */
class WebSocketService {
public:
    WebSocketService();
    ~WebSocketService();

    // ==================== 生命周期 ====================

    /**
     * @brief 启动 WebSocket 服务（阻塞调用，应在独立线程中运行）
     * @param host 监听地址
     * @param port 监听端口
     * @param path WebSocket 路径（前端直连时为 "/"）
     */
    bool start(const std::string& host, int port, const std::string& path = "/");

    /**
     * @brief 停止服务
     */
    void stop();

    // ==================== 客户端管理 ====================

    /**
     * @brief 根据 Crow 连接指针查找 client_id
     * @return client_id，未找到返回 -1
     */
    int64_t findClientByConn(crow::websocket::connection* conn) const;

    /**
     * @brief 获取所有客户端列表（线程安全拷贝）
     */
    std::vector<ClientSession> getClients() const;

    /**
     * @brief 获取客户端数量
     */
    size_t getClientCount() const;

    // ==================== 消息发送 ====================

    /**
     * @brief 广播消息给所有活跃客户端
     */
    bool broadcastMessage(const json& message);

    /**
     * @brief 向指定用户的所有连接发送消息
     */
    bool broadcastToUser(const std::string& user_id, const json& message);

    /**
     * @brief 向指定客户端发送消息
     */
    bool sendToClient(int64_t client_id, const json& message);

    // ==================== 高级推送 ====================

    bool pushAvatarResponse(int64_t client_id, const AvatarResponse& response);
    bool pushStatusUpdate(int64_t client_id, const StatusUpdate& status);
    bool pushNotification(int64_t client_id, const Notification& notification);

    // ==================== 统计 ====================

    Statistics getStatistics() const;

    // ==================== 配置 ====================

    void setHeartbeatInterval(int milliseconds);
    void setIdleTimeout(int milliseconds);
    void setMaxConnections(size_t max_connections);

    // ==================== 事件回调 ====================

    void onClientConnect(std::function<void(int64_t, const ClientMetadata&)> callback);
    void onClientDisconnect(std::function<void(int64_t, const std::string&)> callback);
    void onMessageReceived(std::function<void(int64_t, const json&)> callback);

private:
    // ==================== 内部方法 ====================

    /**
     * @brief Crow WebSocket 连接建立时调用
     */
    void handleOpen(crow::websocket::connection& conn);

    /**
     * @brief Crow WebSocket 消息到达时调用
     */
    void handleMessage(crow::websocket::connection& conn, const std::string& data, bool is_binary);

    /**
     * @brief Crow WebSocket 连接关闭时调用
     */
    void handleClose(crow::websocket::connection& conn, const std::string& reason);

    /**
     * @brief 处理客户端发来的 JSON 消息
     */
    void processClientMessage(int64_t client_id, const json& message);

    /**
     * @brief 心跳检测循环
     */
    void heartbeatLoop();

    /**
     * @brief 从 URL query string 中解析 user_id
     */
    static std::string parseUserId(const std::string& url);

    // ==================== 数据成员 ====================

    // 配置
    std::string host_;
    int port_;
    std::string path_;
    int heartbeat_interval_ms_;
    int idle_timeout_ms_;
    size_t max_connections_;
    std::atomic<bool> is_running_;

    // Crow 应用实例
    std::unique_ptr<crow::SimpleApp> app_;

    // 客户端管理
    int64_t next_client_id_;
    mutable std::mutex clients_mutex_;
    std::unordered_map<int64_t, ClientSession> clients_;

    // 反向映射：connection* -> client_id （用于 onmessage/onclose 快速查找）
    std::unordered_map<crow::websocket::connection*, int64_t> conn_to_client_;

    // 心跳线程
    std::thread heartbeat_thread_;

    // 统计
    std::atomic<size_t> total_messages_sent_{0};
    std::atomic<size_t> total_messages_received_{0};

    // 回调
    std::function<void(int64_t, const ClientMetadata&)> on_client_connect_;
    std::function<void(int64_t, const std::string&)> on_client_disconnect_;
    std::function<void(int64_t, const json&)> on_message_received_;
};

} // namespace Yachiyo::Services
