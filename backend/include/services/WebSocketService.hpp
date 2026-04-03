#pragma once

#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <optional>
#include <functional>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Yachiyo::Services {

// 客户端元数据
struct ClientMetadata {
    std::string device_type;      // web, mobile, desktop
    std::string app_version;
    std::string user_agent;
    json extra_data;
};

// 客户端会话
struct ClientSession {
    int64_t client_id;
    std::string user_id;
    ClientMetadata metadata;
    std::chrono::system_clock::time_point connected_at;
    std::chrono::system_clock::time_point last_heartbeat;
    bool is_active;
    std::queue<json> outgoing_queue;
};

// Avatar 响应
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

class WebSocketService {
public:
    WebSocketService();
    ~WebSocketService();

    // 启动/停止服务
    bool start(const std::string& host, int port, const std::string& path);
    void stop();

    // 客户端管理
    int64_t addClient(const std::string& user_id, const ClientMetadata& metadata);
    bool removeClient(int64_t client_id);
    std::vector<ClientSession> getClients() const;
    ClientSession* getClient(int64_t client_id);
    size_t getClientCount() const;

    // 消息路由
    bool broadcastMessage(const json& message);
    bool broadcastToUser(const std::string& user_id, const json& message);
    bool sendToClient(int64_t client_id, const json& message);
    std::optional<json> getOutgoingMessage(int64_t client_id);

    // 消息处理
    bool handleClientMessage(int64_t client_id, const json& message);

    // 高级推送方法
    bool pushAvatarResponse(int64_t client_id, const AvatarResponse& response);
    bool pushStatusUpdate(int64_t client_id, const StatusUpdate& status);
    bool pushNotification(int64_t client_id, const Notification& notification);

    // 统计信息
    Statistics getStatistics() const;

    // 配置
    void setHeartbeatInterval(int milliseconds);
    void setIdleTimeout(int milliseconds);
    void setMaxConnections(size_t max_connections);

    // 事件回调
    void onClientConnect(std::function<void(int64_t, const ClientMetadata&)> callback);
    void onClientDisconnect(std::function<void(int64_t, const std::string&)> callback);
    void onMessageReceived(std::function<void(int64_t, const json&)> callback);
    void onTypingIndicator(std::function<void(int64_t, const json&)> callback);

private:
    void heartbeatLoop();

    // 配置
    std::string host_;
    int port_;
    std::string path_;
    int heartbeat_interval_ms_;
    int idle_timeout_ms_;
    size_t max_connections_;
    bool is_running_;

    // 状态
    int64_t next_client_id_;
    mutable std::mutex clients_mutex_;
    std::unordered_map<int64_t, ClientSession> clients_;
    std::thread heartbeat_thread_;

    // 回调
    std::function<void(int64_t, const ClientMetadata&)> on_client_connect_;
    std::function<void(int64_t, const std::string&)> on_client_disconnect_;
    std::function<void(int64_t, const json&)> on_message_received_;
    std::function<void(int64_t, const json&)> on_typing_indicator_;
};

} // namespace Yachiyo::Services
