#include "services/WebSocketService.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace Yachiyo::Services {

// ========================================================================
// 构造 / 析构
// ========================================================================

WebSocketService::WebSocketService()
    : port_(9001),
      heartbeat_interval_ms_(30000),
      idle_timeout_ms_(300000),
      max_connections_(1000),
      is_running_(false),
      next_client_id_(1) {}

WebSocketService::~WebSocketService() {
    stop();
}

// ========================================================================
// 生命周期
// ========================================================================

bool WebSocketService::start(const std::string& host, int port,
                             const std::string& path) {
    if (is_running_) return true;

    host_ = host;
    port_ = port;
    path_ = path;

    app_ = std::make_unique<crow::SimpleApp>();

    // ---- 注册 WebSocket 路由 ----
    CROW_WEBSOCKET_ROUTE((*app_), "/")
        .onopen([this](crow::websocket::connection& conn) {
            handleOpen(conn);
        })
        .onmessage([this](crow::websocket::connection& conn,
                          const std::string& data, bool is_binary) {
            handleMessage(conn, data, is_binary);
        })
        .onclose([this](crow::websocket::connection& conn,
                        const std::string& reason) {
            handleClose(conn, reason);
        });

    // 简单的 HTTP 健康检查端点（可选）
    CROW_ROUTE((*app_), "/health")
    ([]() {
        return crow::response(200, "WebSocket service is running");
    });

    is_running_ = true;

    // 启动心跳检测线程
    heartbeat_thread_ = std::thread([this]() { heartbeatLoop(); });

    // 启动 Crow（阻塞调用 —— 应在独立线程中调用 start()）
    std::cout << "[WebSocketService] Starting on " << host << ":" << port << std::endl;
    app_->loglevel(crow::LogLevel::Warning);
    app_->bindaddr(host).port(port).run();

    // Crow::run() 返回意味着服务已停止
    is_running_ = false;
    return true;
}

void WebSocketService::stop() {
    if (!is_running_) return;

    is_running_ = false;

    // 关闭所有客户端连接
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& [id, session] : clients_) {
            if (session.conn) {
                try {
                    session.conn->close("server_shutdown");
                } catch (...) {}
            }
        }
        clients_.clear();
        conn_to_client_.clear();
    }

    // 停止 Crow 应用
    if (app_) {
        app_->stop();
    }

    // 等待心跳线程结束
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }

    std::cout << "[WebSocketService] Stopped" << std::endl;
}

// ========================================================================
// Crow WebSocket 事件处理
// ========================================================================

void WebSocketService::handleOpen(crow::websocket::connection& conn) {
    // user_id 将通过客户端发送的 identify 消息设置
    // Crow WebSocket 不直接暴露 HTTP 升级请求的 query string，
    // 因此这里不尝试从 URL 解析 user_id，而是等待客户端发送 identify 消息

    std::lock_guard<std::mutex> lock(clients_mutex_);

    if (clients_.size() >= max_connections_) {
        std::cout << "[WebSocketService] Max connections reached, rejecting" << std::endl;
        conn.close("max_connections_reached");
        return;
    }

    int64_t client_id = next_client_id_++;

    ClientSession session;
    session.client_id = client_id;
    session.user_id = "";  // 将在第一条 identify 消息或 user_message 中设置
    session.metadata.device_type = "web";
    session.connected_at = std::chrono::system_clock::now();
    session.last_heartbeat = session.connected_at;
    session.is_active = true;
    session.conn = &conn;

    clients_[client_id] = session;
    conn_to_client_[&conn] = client_id;

    std::cout << "[WebSocketService] Client " << client_id << " connected from "
              << conn.get_remote_ip() << std::endl;

    // 发送欢迎消息（包含分配的 client_id）
    json welcome = {
        {"type", "welcome"},
        {"data", {
            {"client_id", client_id},
            {"message", "Connected to Yachiyo WebSocket service"},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        }}
    };

    try {
        conn.send_text(welcome.dump());
    } catch (...) {}
}

void WebSocketService::handleMessage(crow::websocket::connection& conn,
                                     const std::string& data,
                                     bool /*is_binary*/) {
    total_messages_received_++;

    int64_t client_id = -1;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        auto it = conn_to_client_.find(&conn);
        if (it == conn_to_client_.end()) return;
        client_id = it->second;

        // 更新心跳时间
        auto cit = clients_.find(client_id);
        if (cit != clients_.end()) {
            cit->second.last_heartbeat = std::chrono::system_clock::now();
        }
    }

    // 解析 JSON
    try {
        json message = json::parse(data);
        processClientMessage(client_id, message);
    } catch (const json::parse_error& e) {
        std::cerr << "[WebSocketService] JSON parse error from client "
                  << client_id << ": " << e.what() << std::endl;
        json error_resp = {
            {"type", "error"},
            {"data", {{"message", "Invalid JSON format"}}}
        };
        try { conn.send_text(error_resp.dump()); } catch (...) {}
    }
}

void WebSocketService::handleClose(crow::websocket::connection& conn,
                                   const std::string& reason) {
    int64_t client_id = -1;
    std::string user_id;
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);

        auto it = conn_to_client_.find(&conn);
        if (it == conn_to_client_.end()) return;

        client_id = it->second;

        auto cit = clients_.find(client_id);
        if (cit != clients_.end()) {
            user_id = cit->second.user_id;
            clients_.erase(cit);
        }

        conn_to_client_.erase(it);
    }
    // 锁外调用回调，避免死锁

    std::cout << "[WebSocketService] Client " << client_id
              << " (user: " << user_id << ") disconnected: " << reason << std::endl;

    if (on_client_disconnect_) {
        on_client_disconnect_(client_id, reason);
    }
}

// ========================================================================
// 消息处理
// ========================================================================

void WebSocketService::processClientMessage(int64_t client_id, const json& message) {
    if (!message.contains("type")) return;

    std::string msg_type = message["type"];

    if (msg_type == "pong") {
        // 心跳响应 —— 已在 handleMessage 中更新了 last_heartbeat
        return;
    }

    if (msg_type == "identify") {
        // 客户端身份识别：{ type: "identify", data: { user_id: "xxx" } }
        ClientMetadata metadata_copy;
        bool should_notify = false;
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            auto it = clients_.find(client_id);
            if (it != clients_.end() && message.contains("data")) {
                if (message["data"].contains("user_id")) {
                    it->second.user_id = message["data"]["user_id"].get<std::string>();
                    std::cout << "[WebSocketService] Client " << client_id
                              << " identified as user: " << it->second.user_id << std::endl;
                }
                if (message["data"].contains("device_type")) {
                    it->second.metadata.device_type = message["data"]["device_type"].get<std::string>();
                }
                metadata_copy = it->second.metadata;
                should_notify = true;
            }
        }
        // 锁外调用回调，避免死锁（与 handleClose 保持一致）
        if (should_notify && on_client_connect_) {
            on_client_connect_(client_id, metadata_copy);
        }
        return;
    }

    if (msg_type == "user_message") {
        // 用户发送弹幕/消息
        // 如果 user_id 尚未设置，尝试从消息中提取
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            auto it = clients_.find(client_id);
            if (it != clients_.end() && it->second.user_id.empty()) {
                if (message.contains("data") && message["data"].contains("user_id")) {
                    it->second.user_id = message["data"]["user_id"].get<std::string>();
                }
            }
        }

        if (on_message_received_) {
            on_message_received_(client_id, message);
        }
        return;
    }

    if (msg_type == "typing_indicator") {
        // 打字指示器 —— 可转发给其他客户端，目前仅记录
        return;
    }

    if (msg_type == "status_request") {
        // 状态请求
        json status_resp = {
            {"type", "status"},
            {"data", {
                {"status", "connected"},
                {"client_count", getClientCount()},
                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
            }}
        };
        sendToClient(client_id, status_resp);
        return;
    }

    std::cout << "[WebSocketService] Unknown message type: " << msg_type
              << " from client " << client_id << std::endl;
}

// ========================================================================
// 客户端管理
// ========================================================================

int64_t WebSocketService::findClientByConn(crow::websocket::connection* conn) const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = conn_to_client_.find(conn);
    return (it != conn_to_client_.end()) ? it->second : -1;
}

std::vector<ClientSession> WebSocketService::getClients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::vector<ClientSession> result;
    result.reserve(clients_.size());
    for (const auto& [id, session] : clients_) {
        result.push_back(session);
    }
    return result;
}

size_t WebSocketService::getClientCount() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

// ========================================================================
// 消息发送 —— 直接通过 Crow 连接发送
// ========================================================================

bool WebSocketService::broadcastMessage(const json& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::string payload = message.dump();

    for (auto& [id, session] : clients_) {
        if (session.is_active && session.conn) {
            try {
                session.conn->send_text(payload);
                total_messages_sent_++;
            } catch (const std::exception& e) {
                std::cerr << "[WebSocketService] Failed to send to client "
                          << id << ": " << e.what() << std::endl;
            }
        }
    }
    return true;
}

bool WebSocketService::broadcastToUser(const std::string& user_id,
                                       const json& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::string payload = message.dump();
    bool sent = false;

    for (auto& [id, session] : clients_) {
        if (session.user_id == user_id && session.is_active && session.conn) {
            try {
                session.conn->send_text(payload);
                total_messages_sent_++;
                sent = true;
            } catch (const std::exception& e) {
                std::cerr << "[WebSocketService] Failed to send to client "
                          << id << ": " << e.what() << std::endl;
            }
        }
    }
    return sent;
}

bool WebSocketService::sendToClient(int64_t client_id, const json& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end() || !it->second.is_active || !it->second.conn) {
        return false;
    }

    try {
        it->second.conn->send_text(message.dump());
        total_messages_sent_++;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[WebSocketService] Failed to send to client "
                  << client_id << ": " << e.what() << std::endl;
        return false;
    }
}

// ========================================================================
// 高级推送
// ========================================================================

bool WebSocketService::pushAvatarResponse(int64_t client_id,
                                          const AvatarResponse& response) {
    json message = {
        {"type", "avatar_response"},
        {"data", {
            {"request_id", response.request_id},
            {"text", response.text},
            {"audio_url", response.audio_url},
            {"audio_duration_ms", response.audio_duration_ms},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        }}
    };

    if (!response.emotions.empty()) {
        message["data"]["emotions"] = response.emotions;
    }
    if (!response.actions.empty()) {
        message["data"]["actions"] = response.actions;
    }
    if (!response.animation_commands.empty()) {
        message["data"]["animation_commands"] = response.animation_commands;
    }

    return sendToClient(client_id, message);
}

bool WebSocketService::pushStatusUpdate(int64_t client_id,
                                        const StatusUpdate& status) {
    json message = {
        {"type", "status"},
        {"data", {
            {"status", status.status},
            {"progress", status.progress},
            {"message", status.message},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        }}
    };
    if (!status.details.empty()) {
        message["data"]["details"] = status.details;
    }
    return sendToClient(client_id, message);
}

bool WebSocketService::pushNotification(int64_t client_id,
                                        const Notification& notification) {
    json message = {
        {"type", "notification"},
        {"data", {
            {"level", notification.level},
            {"title", notification.title},
            {"message", notification.message},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        }}
    };
    if (!notification.details.empty()) {
        message["data"]["details"] = notification.details;
    }
    return sendToClient(client_id, message);
}

// ========================================================================
// 统计
// ========================================================================

Statistics WebSocketService::getStatistics() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    Statistics stats;
    stats.total_clients = clients_.size();
    stats.active_clients = 0;
    for (const auto& [id, session] : clients_) {
        if (session.is_active) stats.active_clients++;
    }
    stats.total_messages_sent = total_messages_sent_.load();
    stats.total_messages_received = total_messages_received_.load();
    return stats;
}

// ========================================================================
// 配置
// ========================================================================

void WebSocketService::setHeartbeatInterval(int milliseconds) {
    heartbeat_interval_ms_ = milliseconds;
}

void WebSocketService::setIdleTimeout(int milliseconds) {
    idle_timeout_ms_ = milliseconds;
}

void WebSocketService::setMaxConnections(size_t max_connections) {
    max_connections_ = max_connections;
}

// ========================================================================
// 回调
// ========================================================================

void WebSocketService::onClientConnect(
    std::function<void(int64_t, const ClientMetadata&)> callback) {
    on_client_connect_ = std::move(callback);
}

void WebSocketService::onClientDisconnect(
    std::function<void(int64_t, const std::string&)> callback) {
    on_client_disconnect_ = std::move(callback);
}

void WebSocketService::onMessageReceived(
    std::function<void(int64_t, const json&)> callback) {
    on_message_received_ = std::move(callback);
}

// ========================================================================
// 心跳检测
// ========================================================================

void WebSocketService::heartbeatLoop() {
    while (is_running_) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(heartbeat_interval_ms_));

        if (!is_running_) break;

        auto now = std::chrono::system_clock::now();
        std::vector<crow::websocket::connection*> dead_conns;

        {
            std::lock_guard<std::mutex> lock(clients_mutex_);

            for (auto& [id, session] : clients_) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - session.last_heartbeat).count();

                if (elapsed > idle_timeout_ms_) {
                    // 空闲超时
                    session.is_active = false;
                    if (session.conn) {
                        dead_conns.push_back(session.conn);
                    }
                    std::cout << "[WebSocketService] Client " << id
                              << " idle timeout (" << elapsed << "ms)" << std::endl;
                } else {
                    // 发送 ping
                    if (session.conn) {
                        json ping = {
                            {"type", "ping"},
                            {"timestamp", now.time_since_epoch().count()}
                        };
                        try {
                            session.conn->send_text(ping.dump());
                        } catch (...) {
                            dead_conns.push_back(session.conn);
                        }
                    }
                }
            }
        }

        // 关闭超时连接（在锁外执行，因为 close 会触发 handleClose）
        for (auto* conn : dead_conns) {
            try {
                conn->close("idle_timeout");
            } catch (...) {}
        }
    }
}

// ========================================================================
// 工具方法
// ========================================================================

std::string WebSocketService::parseUserId(const std::string& url) {
    // 简单解析 ?user_id=xxx 或 &user_id=xxx
    auto pos = url.find("user_id=");
    if (pos == std::string::npos) return "";

    pos += 8; // strlen("user_id=")
    auto end = url.find('&', pos);
    if (end == std::string::npos) end = url.size();

    return url.substr(pos, end - pos);
}

} // namespace Yachiyo::Services
