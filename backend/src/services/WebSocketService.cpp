#include "WebSocketService.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>

namespace Yachiyo::Services {

WebSocketService::WebSocketService()
    : next_client_id_(1),
      heartbeat_interval_ms_(30000),
      idle_timeout_ms_(300000),
      max_connections_(1000),
      is_running_(false) {}

WebSocketService::~WebSocketService() {
    stop();
}

bool WebSocketService::start(const std::string& host, int port, 
                            const std::string& path) {
    if (is_running_) return true;

    host_ = host;
    port_ = port;
    path_ = path;
    is_running_ = true;

    // 启动心跳检测线程
    heartbeat_thread_ = std::thread([this]() {
        this->heartbeatLoop();
    });

    std::cout << "[WebSocketService] Started on " << host << ":" << port 
              << " path: " << path << std::endl;
    return true;
}

void WebSocketService::stop() {
    is_running_ = false;

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.clear();
    }

    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }

    std::cout << "[WebSocketService] Stopped" << std::endl;
}

int64_t WebSocketService::addClient(const std::string& user_id, 
                                   const ClientMetadata& metadata) {
    if (clients_.size() >= max_connections_) {
        return -1; // 连接数达到上限
    }

    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    int64_t client_id = next_client_id_++;
    ClientSession session;
    session.client_id = client_id;
    session.user_id = user_id;
    session.metadata = metadata;
    session.connected_at = std::chrono::system_clock::now();
    session.last_heartbeat = session.connected_at;
    session.is_active = true;

    clients_[client_id] = session;

    std::cout << "[WebSocketService] Client " << client_id << " (user: " 
              << user_id << ") connected" << std::endl;

    // 触发连接回调
    if (on_client_connect_) {
        on_client_connect_(client_id, metadata);
    }

    return client_id;
}

bool WebSocketService::removeClient(int64_t client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        return false;
    }

    auto& session = it->second;
    std::cout << "[WebSocketService] Client " << client_id 
              << " disconnected" << std::endl;

    // 触发断开回调
    if (on_client_disconnect_) {
        on_client_disconnect_(client_id, "normal");
    }

    clients_.erase(it);
    return true;
}

std::vector<ClientSession> WebSocketService::getClients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::vector<ClientSession> result;
    for (const auto& pair : clients_) {
        result.push_back(pair.second);
    }
    return result;
}

ClientSession* WebSocketService::getClient(int64_t client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool WebSocketService::broadcastMessage(const json& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    for (auto& pair : clients_) {
        auto& session = pair.second;
        if (session.is_active) {
            session.outgoing_queue.push(message);
        }
    }

    return true;
}

bool WebSocketService::broadcastToUser(const std::string& user_id,
                                      const json& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    for (auto& pair : clients_) {
        auto& session = pair.second;
        if (session.user_id == user_id && session.is_active) {
            session.outgoing_queue.push(message);
        }
    }

    return true;
}

bool WebSocketService::sendToClient(int64_t client_id,
                                   const json& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end() || !it->second.is_active) {
        return false;
    }

    it->second.outgoing_queue.push(message);
    return true;
}

bool WebSocketService::handleClientMessage(int64_t client_id,
                                          const json& message) {
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        auto it = clients_.find(client_id);
        if (it == clients_.end()) {
            return false;
        }
        it->second.last_heartbeat = std::chrono::system_clock::now();
    }

    // 获取消息类型
    if (!message.contains("type")) {
        return false;
    }

    std::string msg_type = message["type"];

    if (msg_type == "ping") {
        // 心跳响应
        json pong_response = {
            {"type", "pong"},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        };
        sendToClient(client_id, pong_response);
        return true;
    }
    else if (msg_type == "user_message") {
        // 用户消息
        if (on_message_received_) {
            on_message_received_(client_id, message);
        }
        return true;
    }
    else if (msg_type == "typing_indicator") {
        // 输入指示器
        if (on_typing_indicator_) {
            on_typing_indicator_(client_id, message);
        }
        return true;
    }
    else if (msg_type == "status_request") {
        // 状态请求
        json status_response = {
            {"type", "status"},
            {"status", "connected"},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        };
        sendToClient(client_id, status_response);
        return true;
    }

    return false;
}

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

    // 添加情感标签
    if (!response.emotions.empty()) {
        json emotions_json = json::array();
        for (const auto& emotion : response.emotions) {
            emotions_json.push_back(emotion);
        }
        message["data"]["emotions"] = emotions_json;
    }

    // 添加动作
    if (!response.actions.empty()) {
        json actions_json = json::array();
        for (const auto& action : response.actions) {
            actions_json.push_back(action);
        }
        message["data"]["actions"] = actions_json;
    }

    // 添加动画命令
    if (!response.animation_commands.empty()) {
        json animations = json::array();
        for (const auto& cmd : response.animation_commands) {
            animations.push_back(cmd);
        }
        message["data"]["animation_commands"] = animations;
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

std::optional<json> WebSocketService::getOutgoingMessage(int64_t client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_id);
    if (it == clients_.end() || it->second.outgoing_queue.empty()) {
        return std::nullopt;
    }

    json msg = it->second.outgoing_queue.front();
    it->second.outgoing_queue.pop();
    return msg;
}

size_t WebSocketService::getClientCount() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

Statistics WebSocketService::getStatistics() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    Statistics stats;
    stats.total_clients = clients_.size();
    stats.active_clients = 0;
    stats.total_messages_sent = 0;
    stats.total_messages_received = 0;

    for (const auto& pair : clients_) {
        if (pair.second.is_active) {
            stats.active_clients++;
        }
        stats.total_messages_sent += pair.second.outgoing_queue.size();
    }

    return stats;
}

void WebSocketService::setHeartbeatInterval(int milliseconds) {
    heartbeat_interval_ms_ = milliseconds;
}

void WebSocketService::setIdleTimeout(int milliseconds) {
    idle_timeout_ms_ = milliseconds;
}

void WebSocketService::setMaxConnections(size_t max_connections) {
    max_connections_ = max_connections;
}

void WebSocketService::onClientConnect(
    std::function<void(int64_t, const ClientMetadata&)> callback) {
    on_client_connect_ = callback;
}

void WebSocketService::onClientDisconnect(
    std::function<void(int64_t, const std::string&)> callback) {
    on_client_disconnect_ = callback;
}

void WebSocketService::onMessageReceived(
    std::function<void(int64_t, const json&)> callback) {
    on_message_received_ = callback;
}

void WebSocketService::onTypingIndicator(
    std::function<void(int64_t, const json&)> callback) {
    on_typing_indicator_ = callback;
}

void WebSocketService::heartbeatLoop() {
    while (is_running_) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(heartbeat_interval_ms_)
        );

        auto now = std::chrono::system_clock::now();

        {
            std::lock_guard<std::mutex> lock(clients_mutex_);

            std::vector<int64_t> dead_clients;

            for (auto& pair : clients_) {
                auto& session = pair.second;
                auto time_since_heartbeat = std::chrono::duration_cast<
                    std::chrono::milliseconds>(
                    now - session.last_heartbeat
                ).count();

                if (time_since_heartbeat > idle_timeout_ms_) {
                    // 客户端空闲超时
                    dead_clients.push_back(pair.first);
                    session.is_active = false;
                }
                else {
                    // 发送心跳
                    json ping = {
                        {"type", "ping"},
                        {"timestamp", now.time_since_epoch().count()}
                    };
                    session.outgoing_queue.push(ping);
                }
            }

            // 移除死亡的客户端
            for (auto client_id : dead_clients) {
                std::cout << "[WebSocketService] Client " << client_id 
                          << " idle timeout" << std::endl;
                if (on_client_disconnect_) {
                    on_client_disconnect_(client_id, "idle_timeout");
                }
                clients_.erase(client_id);
            }
        }
    }
}

} // namespace Yachiyo::Services
