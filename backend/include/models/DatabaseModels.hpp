#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Yachiyo::Models {

// ============ 数据结构 ============

struct User {
    int64_t id;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string salt;
    std::string nickname;
    std::string avatar_url;
    int role = 1;        // 1=普通用户, 99=管理员
    int status = 1;      // 1=活跃, 2=禁用, 3=封禁
    json profile_data;
    json preferences;
    int64_t created_at;
    int64_t updated_at;
    int64_t last_login;
    bool is_active;
};

struct Session {
    int64_t id;
    int64_t user_id;
    std::string session_id;
    std::string access_token;
    std::string refresh_token;
    std::string ip_address;
    std::string user_agent;
    std::string device_type;
    int64_t created_at;
    int64_t expires_at;
    bool is_active;
};

struct Message {
    int64_t id;
    int64_t user_id;
    std::string content;
    std::string language;
    int character_count;
    std::string review_status;  // pending, approved, rejected
    json moderation_result;
    json avatar_response;
    int64_t created_at;
    bool is_visible;
};

struct ConversationContext {
    int64_t id;
    int64_t user_id;
    std::string session_id;
    std::string conversation_id;
    json context_data;
    json message_history;
    json user_profile;
    int message_count;
    int64_t created_at;
    int64_t updated_at;
    bool is_active;
};

struct ModerationLog {
    int64_t id;
    int64_t message_id;
    int64_t user_id;
    std::string violation_type;
    double severity_score;
    bool is_violation;
    json violation_details;
    double confidence_score;
    std::string action_taken;
    int64_t created_at;
};

struct UserStatistics {
    int64_t user_id;
    int total_messages;
    int total_characters;
    double average_message_length;
    int flagged_messages;
    int violation_count;
    int average_response_time_ms;
    int64_t updated_at;
};

// ============ 数据库结果类 ============

template<typename T>
struct Result {
    bool success;
    std::optional<T> data;
    std::string error_message;

    Result(const T& value) : success(true), data(value) {}
    Result(const std::string& error) : success(false), error_message(error) {}
    static Result Error(const std::string& msg) { return Result(msg); }
};

// Result<void> 特化 — 用于不返回数据的操作 (delete, update等)
template<>
struct Result<void> {
    bool success;
    std::string error_message;

    Result() : success(true) {}
    Result(const std::string& error) : success(false), error_message(error) {}
    static Result Error(const std::string& msg) { return Result(msg); }
};

} // namespace Yachiyo::Models
