#include "services/DatabaseService.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <map>
#include <nlohmann/json.hpp>

namespace Yachiyo::Services {

namespace {
int64_t parseTimestampToEpoch(const pqxx::field_ref& field) {
    if (field.is_null()) {
        return 0;
    }

    const std::string raw = field.as<std::string>();
    try {
        return std::stoll(raw);
    } catch (...) {
    }

    std::string normalized = raw;
    auto dotPos = normalized.find('.');
    if (dotPos != std::string::npos) {
        normalized = normalized.substr(0, dotPos);
    }
    auto plusPos = normalized.find('+');
    if (plusPos != std::string::npos) {
        normalized = normalized.substr(0, plusPos);
    }
    auto minusPos = normalized.find('-', 10);
    if (minusPos != std::string::npos) {
        normalized = normalized.substr(0, minusPos);
    }

    std::tm tm = {};
    std::istringstream ss(normalized);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        return 0;
    }
    return static_cast<int64_t>(std::mktime(&tm));
}
} // namespace

// ============ DatabasePool 实现 ============

DatabasePool& DatabasePool::getInstance() {
    static DatabasePool instance;
    return instance;
}

bool DatabasePool::connect(const std::string& connection_string, size_t pool_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        connection_string_ = connection_string;
        pool_size_ = pool_size;
        closed_ = false;

        for (size_t i = 0; i < pool_size; ++i) {
            auto conn = std::make_shared<pqxx::connection>(connection_string);
            if (!conn->is_open()) {
                std::cerr << "[DatabasePool] Failed to open connection #" << i << std::endl;
                return false;
            }
            pool_.push(conn);
        }

        std::cout << "[DatabasePool] Connected to database (pool size: " << pool_size << ")" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[DatabasePool] Connection error: " << e.what() << std::endl;
        return false;
    }
}

bool DatabasePool::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pool_size_ > 0 && !closed_;
}

void DatabasePool::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    while (!pool_.empty()) {
        pool_.pop();
        // pqxx::connection 在析构时自动关闭，无需手动 disconnect
    }
    pool_size_ = 0;
    cv_.notify_all();
}

std::shared_ptr<pqxx::connection> DatabasePool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    // 等待有可用连接，最多 5 秒
    if (!cv_.wait_for(lock, std::chrono::seconds(5), [this]() {
        return !pool_.empty() || closed_;
    })) {
        throw std::runtime_error("[DatabasePool] Timeout waiting for available connection");
    }
    if (closed_ || pool_.empty()) {
        throw std::runtime_error("[DatabasePool] Pool is closed or empty");
    }
    auto conn = pool_.front();
    pool_.pop();

    // 检查连接是否仍然有效，否则重新创建
    if (!conn || !conn->is_open()) {
        try {
            conn = std::make_shared<pqxx::connection>(connection_string_);
        } catch (const std::exception& e) {
            // 归还一个空位，让其他等待者有机会
            cv_.notify_one();
            throw;
        }
    }
    return conn;
}

void DatabasePool::releaseConnection(std::shared_ptr<pqxx::connection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!closed_ && conn) {
        pool_.push(conn);
    }
    cv_.notify_one();
}

// ============ MessageDAO 实现 ============

Result<int64_t> MessageDAO::create(const Message& message) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            INSERT INTO user_messages 
            (user_id, original_message, message_length, review_status, created_at)
            VALUES ($1, $2, $3, $4, NOW())
            RETURNING id
        )";

        auto result = txn.exec_params(
            query,
            message.user_id,
            message.content,
            message.character_count,
            message.review_status
        );

        txn.commit();

        if (!result.empty()) {
            int64_t id = result[0]["id"].as<int64_t>();
            std::cout << "[MessageDAO] Created message: " << id << std::endl;
            return Result<int64_t>(id);
        }

        return Result<int64_t>::Error("Failed to create message");
    } catch (const std::exception& e) {
        std::cerr << "[MessageDAO] Error: " << e.what() << std::endl;
        return Result<int64_t>::Error(std::string(e.what()));
    }
}

Result<Message> MessageDAO::getById(int64_t message_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, user_id, original_message AS content, 'zh' AS language, message_length AS character_count, 
                   review_status, NULL AS moderation_result, NULL AS avatar_response, 
                   created_at, (review_status != 2) AS is_visible
            FROM user_messages
            WHERE id = $1
        )";

        auto result = txn.exec_params(query, message_id);
        txn.commit();

        if (!result.empty()) {
            return Result<Message>(parseRow(result[0]));
        }

        return Result<Message>::Error("Message not found");
    } catch (const std::exception& e) {
        return Result<Message>::Error(std::string(e.what()));
    }
}

Result<std::vector<Message>> MessageDAO::getByUserId(int64_t user_id, int limit, int offset) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, user_id, original_message AS content, 'zh' AS language, message_length AS character_count, 
                   review_status, NULL AS moderation_result, NULL AS avatar_response, 
                   created_at, (review_status != 2) AS is_visible
            FROM user_messages
            WHERE user_id = $1 AND review_status != 2
            ORDER BY created_at DESC
            LIMIT $2 OFFSET $3
        )";

        auto result = txn.exec_params(query, user_id, limit, offset);
        txn.commit();

        std::vector<Message> messages;
        for (auto row : result) {
            messages.push_back(parseRow(row));
        }

        return Result<std::vector<Message>>(messages);
    } catch (const std::exception& e) {
        return Result<std::vector<Message>>::Error(std::string(e.what()));
    }
}

Result<std::vector<Message>> MessageDAO::getPendingReview(int limit) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, user_id, original_message AS content, 'zh' AS language, message_length AS character_count, 
                   review_status::text, NULL AS moderation_result, NULL AS avatar_response, 
                   created_at, (review_status != 2) AS is_visible
            FROM user_messages
            WHERE review_status = 3
            ORDER BY created_at ASC
            LIMIT $1
        )";

        auto result = txn.exec_params(query, limit);
        txn.commit();

        std::vector<Message> messages;
        for (auto row : result) {
            messages.push_back(parseRow(row));
        }

        return Result<std::vector<Message>>(messages);
    } catch (const std::exception& e) {
        return Result<std::vector<Message>>::Error(std::string(e.what()));
    }
}

Result<void> MessageDAO::updateModerationResult(int64_t message_id,
                                               const std::string& status,
                                               const json& moderation_data) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            UPDATE user_messages
            SET review_status = $1::smallint, review_reason = $2
            WHERE id = $3
        )";

        txn.exec_params(
            query,
            status,
            moderation_data.dump(),
            message_id
        );

        txn.commit();

        std::cout << "[MessageDAO] Updated moderation result for message: " << message_id << std::endl;
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<void> MessageDAO::updateAvatarResponse(int64_t message_id, const json& response_data) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        // user_messages 表没有 avatar_response 列，写入 avatar_responses 表
        std::string query = R"(
            INSERT INTO avatar_responses (user_id, message_id, response_data, created_at)
            SELECT user_id, $1, $2::jsonb, NOW()
            FROM user_messages WHERE id = $1
            ON CONFLICT (message_id) DO UPDATE SET response_data = $2::jsonb
        )";

        txn.exec_params(query, message_id, response_data.dump());
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<void> MessageDAO::delete_(int64_t message_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = "UPDATE user_messages SET review_status = 2 WHERE id = $1";
        txn.exec_params(query, message_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<std::map<int64_t, nlohmann::json>> MessageDAO::getAvatarResponses(const std::vector<int64_t>& message_ids) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::map<int64_t, nlohmann::json> result;
        if (message_ids.empty()) return result;

        // 构建 IN 子句
        std::ostringstream placeholders;
        for (size_t i = 0; i < message_ids.size(); ++i) {
            if (i > 0) placeholders << ",";
            placeholders << "$" << (i + 1);
        }

        std::string query = "SELECT message_id, response_data::text FROM avatar_responses WHERE message_id IN (" + placeholders.str() + ")";
        pqxx::params params;
        for (auto id : message_ids) params.append(id);

        auto rows = txn.exec_params(query, params);
        txn.commit();

        for (auto row : rows) {
            int64_t mid = row[0].as<int64_t>();
            std::string data_str = row[1].as<std::string>();
            try {
                result[mid] = nlohmann::json::parse(data_str);
            } catch (...) {
                // 解析失败则跳过
            }
        }
        return result;
    } catch (const std::exception& e) {
        return Result<std::map<int64_t, nlohmann::json>>::Error(std::string(e.what()));
    }
}

Message MessageDAO::parseRow(pqxx::row_ref row) {
    Message msg;
    msg.id = row.at("id").as<int64_t>();
    msg.user_id = row.at("user_id").as<int64_t>();
    msg.content = row.at("content").as<std::string>();
    msg.language = row.at("language").as<std::string>();
    msg.character_count = row.at("character_count").as<int>();
    msg.review_status = row.at("review_status").as<std::string>();

    if (!row.at("moderation_result").is_null()) {
        msg.moderation_result = json::parse(row.at("moderation_result").as<std::string>());
    }

    if (!row.at("avatar_response").is_null()) {
        msg.avatar_response = json::parse(row.at("avatar_response").as<std::string>());
    }

    msg.created_at = parseTimestampToEpoch(row.at("created_at"));
    msg.is_visible = row.at("is_visible").as<bool>();

    return msg;
}

// ============ ConversationContextDAO 实现 ============

Result<int64_t> ConversationContextDAO::create(const ConversationContext& context) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            INSERT INTO conversation_contexts 
            (user_id, session_id, conversation_id, context_data, message_history, 
             user_profile, message_count, created_at, is_active)
            VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)
            RETURNING id
        )";

        auto result = txn.exec_params(
            query,
            context.user_id,
            context.session_id,
            context.conversation_id,
            context.context_data.dump(),
            context.message_history.dump(),
            context.user_profile.dump(),
            context.message_count,
            context.created_at,
            context.is_active
        );

        txn.commit();

        if (!result.empty()) {
            int64_t id = result[0]["id"].as<int64_t>();
            return Result<int64_t>(id);
        }

        return Result<int64_t>::Error("Failed to create context");
    } catch (const std::exception& e) {
        return Result<int64_t>::Error(std::string(e.what()));
    }
}

Result<ConversationContext> ConversationContextDAO::getById(int64_t context_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, user_id, session_id, conversation_id, context_data,
                   message_history, user_profile, message_count, created_at, 
                   updated_at, is_active
            FROM conversation_contexts
            WHERE id = $1
        )";

        auto result = txn.exec_params(query, context_id);
        txn.commit();

        if (!result.empty()) {
            return Result<ConversationContext>(parseRow(result[0]));
        }

        return Result<ConversationContext>::Error("Context not found");
    } catch (const std::exception& e) {
        return Result<ConversationContext>::Error(std::string(e.what()));
    }
}

Result<std::vector<ConversationContext>> ConversationContextDAO::getByUserId(int64_t user_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, user_id, session_id, conversation_id, context_data,
                   message_history, user_profile, message_count, created_at, 
                   updated_at, is_active
            FROM conversation_contexts
            WHERE user_id = $1 AND is_active = TRUE
            ORDER BY updated_at DESC
        )";

        auto result = txn.exec_params(query, user_id);
        txn.commit();

        std::vector<ConversationContext> contexts;
        for (auto row : result) {
            contexts.push_back(parseRow(row));
        }

        return Result<std::vector<ConversationContext>>(contexts);
    } catch (const std::exception& e) {
        return Result<std::vector<ConversationContext>>::Error(std::string(e.what()));
    }
}

Result<ConversationContext> ConversationContextDAO::getActiveContext(int64_t user_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, user_id, session_id, conversation_id, context_data,
                   message_history, user_profile, message_count, created_at, 
                   updated_at, is_active
            FROM conversation_contexts
            WHERE user_id = $1 AND is_active = TRUE
            ORDER BY updated_at DESC
            LIMIT 1
        )";

        auto result = txn.exec_params(query, user_id);
        txn.commit();

        if (!result.empty()) {
            return Result<ConversationContext>(parseRow(result[0]));
        }

        return Result<ConversationContext>::Error("No active context found");
    } catch (const std::exception& e) {
        return Result<ConversationContext>::Error(std::string(e.what()));
    }
}

Result<void> ConversationContextDAO::update(const ConversationContext& context) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            UPDATE conversation_contexts
            SET context_data = $1, message_history = $2, user_profile = $3,
                message_count = $4, is_active = $5, updated_at = NOW()
            WHERE id = $6
        )";

        txn.exec_params(
            query,
            context.context_data.dump(),
            context.message_history.dump(),
            context.user_profile.dump(),
            context.message_count,
            context.is_active,
            context.id
        );

        txn.commit();
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<void> ConversationContextDAO::addMessageToHistory(int64_t context_id, const json& message_data) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        // 先获取当前的消息历史
        std::string select_query = "SELECT message_history FROM conversation_contexts WHERE id = $1";
        auto result = txn.exec_params(select_query, context_id);

        if (result.empty()) {
            return Result<void>::Error("Context not found");
        }

        json message_history = json::parse(result[0]["message_history"].as<std::string>());
        
        // 添加新消息
        if (!message_history.is_array()) {
            message_history = json::array();
        }
        message_history.push_back(message_data);

        // 只保留最后 20 条消息
        if (message_history.size() > 20) {
            message_history = json(
                message_history.begin() + static_cast<int>(message_history.size()) - 20,
                message_history.end()
            );
        }

        // 更新
        std::string update_query = R"(
            UPDATE conversation_contexts
            SET message_history = $1, updated_at = NOW()
            WHERE id = $2
        )";

        txn.exec_params(update_query, message_history.dump(), context_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

ConversationContext ConversationContextDAO::parseRow(pqxx::row_ref row) {
    ConversationContext ctx;
    ctx.id = row.at("id").as<int64_t>();
    ctx.user_id = row.at("user_id").as<int64_t>();
    ctx.session_id = row.at("session_id").is_null() ? "" : row.at("session_id").as<std::string>();
    ctx.conversation_id = row.at("conversation_id").is_null() ? "" : row.at("conversation_id").as<std::string>();
    ctx.context_data = row.at("context_data").is_null() ? json::object() : json::parse(row.at("context_data").as<std::string>("{}"));
    ctx.message_history = row.at("message_history").is_null() ? json::object() : json::parse(row.at("message_history").as<std::string>("{}"));
    ctx.user_profile = row.at("user_profile").is_null() ? json::object() : json::parse(row.at("user_profile").as<std::string>("{}"));
    ctx.message_count = row.at("message_count").as<int>();
    ctx.created_at = parseTimestampToEpoch(row.at("created_at"));
    ctx.updated_at = parseTimestampToEpoch(row.at("updated_at"));
    ctx.is_active = row.at("is_active").as<bool>();
    return ctx;
}

// ============ DatabaseService 实现 ============

DatabaseService& DatabaseService::getInstance() {
    static DatabaseService instance;
    return instance;
}

DatabaseService::DatabaseService()
    : initialized_(false) {}

bool DatabaseService::initialize(const std::string& connection_string, size_t pool_size) {
    try {
        if (!DatabasePool::getInstance().connect(connection_string, pool_size)) {
            return false;
        }

        connection_ = DatabasePool::getInstance().getConnection();

        // 初始化所有 DAO
        message_dao_.emplace(connection_);
        context_dao_.emplace(connection_);
        user_dao_.emplace(connection_);
        moderation_dao_.emplace(connection_);

        initialized_ = true;
        std::cout << "[DatabaseService] Initialized successfully" << std::endl;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseService] Initialization error: " << e.what() << std::endl;
        return false;
    }
}

Result<std::vector<Message>> DatabaseService::buildConversationHistory(int64_t user_id, int limit) {
    return message_dao_->getByUserId(user_id, limit, 0);
}

Result<json> DatabaseService::buildOpenClawContext(int64_t user_id) {
    try {
        json context;
        context["user_id"] = user_id;
        context["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

        // 获取最近的对话上下文
        auto ctx_result = context_dao_->getActiveContext(user_id);
        if (ctx_result.success && ctx_result.data) {
            context["conversation"] = ctx_result.data->context_data;
            context["message_history"] = ctx_result.data->message_history;
            context["user_profile"] = ctx_result.data->user_profile;
        }

        // 获取消息历史
        auto msg_result = buildConversationHistory(user_id, 10);
        if (msg_result.success) {
            json messages = json::array();
            for (const auto& msg : msg_result.data.value()) {
                messages.push_back({
                    {"role", "user"},
                    {"content", msg.content},
                    {"created_at", msg.created_at}
                });

                if (!msg.avatar_response.is_null()) {
                    messages.push_back({
                        {"role", "assistant"},
                        {"content", msg.avatar_response},
                        {"created_at", msg.created_at}
                    });
                }
            }
            context["recent_messages"] = messages;
        }

        return Result<json>(context);
    } catch (const std::exception& e) {
        return Result<json>::Error(std::string(e.what()));
    }
}

// ============ UserDAO 实现 ============

Result<int64_t> UserDAO::create(const User& user) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            INSERT INTO users 
            (username, email, password_hash, salt, nickname, avatar_url, role, status,
             profile_data, preferences, is_active)
            VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)
            RETURNING id
        )";

        auto result = txn.exec_params(
            query,
            user.username,
            user.email,
            user.password_hash,
            user.salt,
            user.nickname,
            user.avatar_url,
            user.role,
            user.status,
            user.profile_data.dump(),
            user.preferences.dump(),
            user.is_active
        );

        txn.commit();

        if (!result.empty()) {
            int64_t id = result[0]["id"].as<int64_t>();
            std::cout << "[UserDAO] Created user: " << id << std::endl;
            return Result<int64_t>(id);
        }

        return Result<int64_t>::Error("Failed to create user");
    } catch (const std::exception& e) {
        return Result<int64_t>::Error(std::string(e.what()));
    }
}

Result<User> UserDAO::getById(int64_t user_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, username, email, password_hash, salt, nickname, avatar_url,
                   role, status, profile_data, preferences,
                   created_at, updated_at, last_login_at, is_active
            FROM users
            WHERE id = $1
        )";

        auto result = txn.exec_params(query, user_id);
        txn.commit();

        if (!result.empty()) {
            return Result<User>(parseRow(result[0]));
        }

        return Result<User>::Error("User not found");
    } catch (const std::exception& e) {
        return Result<User>::Error(std::string(e.what()));
    }
}

Result<User> UserDAO::getByUsername(const std::string& username) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, username, email, password_hash, salt, nickname, avatar_url,
                   role, status, profile_data, preferences,
                   created_at, updated_at, last_login_at, is_active
            FROM users
            WHERE username = $1
        )";

        auto result = txn.exec_params(query, username);
        txn.commit();

        if (!result.empty()) {
            return Result<User>(parseRow(result[0]));
        }

        return Result<User>::Error("User not found");
    } catch (const std::exception& e) {
        return Result<User>::Error(std::string(e.what()));
    }
}

Result<User> UserDAO::getByEmail(const std::string& email) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, username, email, password_hash, salt, nickname, avatar_url,
                   role, status, profile_data, preferences,
                   created_at, updated_at, last_login_at, is_active
            FROM users
            WHERE email = $1
        )";

        auto result = txn.exec_params(query, email);
        txn.commit();

        if (!result.empty()) {
            return Result<User>(parseRow(result[0]));
        }

        return Result<User>::Error("User not found");
    } catch (const std::exception& e) {
        return Result<User>::Error(std::string(e.what()));
    }
}

Result<void> UserDAO::updateProfile(int64_t user_id, const json& profile_data) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            UPDATE users
            SET profile_data = $1, updated_at = NOW()
            WHERE id = $2
        )";

        txn.exec_params(query, profile_data.dump(), user_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<void> UserDAO::updateRole(int64_t user_id, int role) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            UPDATE users
            SET role = $1, updated_at = NOW()
            WHERE id = $2
        )";

        txn.exec_params(query, role, user_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<void> UserDAO::updatePreferences(int64_t user_id, const json& preferences) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            UPDATE users
            SET preferences = $1, updated_at = NOW()
            WHERE id = $2
        )";

        txn.exec_params(query, preferences.dump(), user_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<void> UserDAO::updateLastLogin(int64_t user_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = "UPDATE users SET last_login_at = NOW() WHERE id = $1";
        txn.exec_params(query, user_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<void> UserDAO::delete_(int64_t user_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = "UPDATE users SET is_active = FALSE WHERE id = $1";
        txn.exec_params(query, user_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

User UserDAO::parseRow(pqxx::row_ref row) {
    User user;
    user.id = row.at("id").as<int64_t>();
    user.username = row.at("username").as<std::string>();
    user.email = row.at("email").as<std::string>();
    user.password_hash = row.at("password_hash").as<std::string>();
    user.salt = row.at("salt").is_null() ? "" : row.at("salt").as<std::string>();
    user.nickname = row.at("nickname").is_null() ? "" : row.at("nickname").as<std::string>();
    user.avatar_url = row.at("avatar_url").is_null() ? "" : row.at("avatar_url").as<std::string>();
    user.role = row.at("role").is_null() ? 1 : row.at("role").as<int>();
    user.status = row.at("status").is_null() ? 1 : row.at("status").as<int>();
    // profile_data 和 preferences 可能为 NULL，需要安全处理
    user.profile_data = row.at("profile_data").is_null() ? json::object() : json::parse(row.at("profile_data").as<std::string>("{}"));
    user.preferences = row.at("preferences").is_null() ? json::object() : json::parse(row.at("preferences").as<std::string>("{}"));
    user.created_at = parseTimestampToEpoch(row.at("created_at"));
    user.updated_at = parseTimestampToEpoch(row.at("updated_at"));
    user.last_login = parseTimestampToEpoch(row.at("last_login_at"));
    user.is_active = row.at("is_active").as<bool>();
    return user;
}

// ============ ModerationLogDAO 实现 ============

Result<int64_t> ModerationLogDAO::create(const ModerationLog& log) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            INSERT INTO moderation_logs 
            (message_id, user_id, violation_type, severity_score, is_violation, 
             violation_details, confidence_score, action_taken, created_at)
            VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)
            RETURNING id
        )";

        auto result = txn.exec_params(
            query,
            log.message_id,
            log.user_id,
            log.violation_type,
            log.severity_score,
            log.is_violation,
            log.violation_details.dump(),
            log.confidence_score,
            log.action_taken,
            log.created_at
        );

        txn.commit();

        if (!result.empty()) {
            int64_t id = result[0]["id"].as<int64_t>();
            std::cout << "[ModerationLogDAO] Created moderation log: " << id << std::endl;
            return Result<int64_t>(id);
        }

        return Result<int64_t>::Error("Failed to create moderation log");
    } catch (const std::exception& e) {
        return Result<int64_t>::Error(std::string(e.what()));
    }
}

Result<std::vector<ModerationLog>> ModerationLogDAO::getByMessageId(int64_t message_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, message_id, user_id, violation_type, severity_score, 
                   is_violation, violation_details, confidence_score, action_taken, created_at
            FROM moderation_logs
            WHERE message_id = $1
            ORDER BY created_at DESC
        )";

        auto result = txn.exec_params(query, message_id);
        txn.commit();

        std::vector<ModerationLog> logs;
        for (auto row : result) {
            logs.push_back(parseRow(row));
        }

        return Result<std::vector<ModerationLog>>(logs);
    } catch (const std::exception& e) {
        return Result<std::vector<ModerationLog>>::Error(std::string(e.what()));
    }
}

Result<std::vector<ModerationLog>> ModerationLogDAO::getByUserId(int64_t user_id) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, message_id, user_id, violation_type, severity_score, 
                   is_violation, violation_details, confidence_score, action_taken, created_at
            FROM moderation_logs
            WHERE user_id = $1
            ORDER BY created_at DESC
        )";

        auto result = txn.exec_params(query, user_id);
        txn.commit();

        std::vector<ModerationLog> logs;
        for (auto row : result) {
            logs.push_back(parseRow(row));
        }

        return Result<std::vector<ModerationLog>>(logs);
    } catch (const std::exception& e) {
        return Result<std::vector<ModerationLog>>::Error(std::string(e.what()));
    }
}

Result<std::vector<ModerationLog>> ModerationLogDAO::getHighRiskMessages(double severity_threshold) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, message_id, user_id, violation_type, severity_score, 
                   is_violation, violation_details, confidence_score, action_taken, created_at
            FROM moderation_logs
            WHERE severity_score >= $1 AND is_violation = TRUE
            ORDER BY severity_score DESC, created_at DESC
            LIMIT 100
        )";

        auto result = txn.exec_params(query, severity_threshold);
        txn.commit();

        std::vector<ModerationLog> logs;
        for (auto row : result) {
            logs.push_back(parseRow(row));
        }

        return Result<std::vector<ModerationLog>>(logs);
    } catch (const std::exception& e) {
        return Result<std::vector<ModerationLog>>::Error(std::string(e.what()));
    }
}

ModerationLog ModerationLogDAO::parseRow(pqxx::row_ref row) {
    ModerationLog log;
    log.id = row.at("id").as<int64_t>();
    log.message_id = row.at("message_id").as<int64_t>();
    log.user_id = row.at("user_id").as<int64_t>();
    log.violation_type = row.at("violation_type").is_null() ? "" : row.at("violation_type").as<std::string>();
    log.severity_score = row.at("severity_score").is_null() ? 0.0 : row.at("severity_score").as<double>();
    log.is_violation = row.at("is_violation").is_null() ? false : row.at("is_violation").as<bool>();
    log.violation_details = row.at("violation_details").is_null() ? json::object() : json::parse(row.at("violation_details").as<std::string>("{}"));
    log.confidence_score = row.at("confidence_score").is_null() ? 0.0 : row.at("confidence_score").as<double>();
    log.action_taken = row.at("action_taken").is_null() ? "" : row.at("action_taken").as<std::string>();
    log.created_at = parseTimestampToEpoch(row.at("created_at"));
    return log;
}

} // namespace Yachiyo::Services
