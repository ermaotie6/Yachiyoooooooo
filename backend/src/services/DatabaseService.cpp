#include "DatabaseService.hpp"
#include <iostream>
#include <chrono>

namespace Yachiyo::Services {

// ============ DatabasePool 实现 ============

DatabasePool& DatabasePool::getInstance() {
    static DatabasePool instance;
    return instance;
}

bool DatabasePool::connect(const std::string& connection_string) {
    try {
        connection_string_ = connection_string;
        connection_ = std::make_shared<pqxx::connection>(connection_string);
        
        if (!connection_->is_open()) {
            std::cerr << "[DatabasePool] Failed to open connection" << std::endl;
            return false;
        }

        std::cout << "[DatabasePool] Connected to database" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[DatabasePool] Connection error: " << e.what() << std::endl;
        return false;
    }
}

bool DatabasePool::isConnected() const {
    return connection_ && connection_->is_open();
}

void DatabasePool::close() {
    if (connection_) {
        connection_->disconnect();
        connection_.reset();
    }
}

std::shared_ptr<pqxx::connection> DatabasePool::getConnection() {
    return connection_;
}

// ============ MessageDAO 实现 ============

Result<int64_t> MessageDAO::create(const Message& message) {
    try {
        pqxx::work txn(*conn_);

        std::string query = R"(
            INSERT INTO messages 
            (user_id, content, language, character_count, review_status, created_at)
            VALUES ($1, $2, $3, $4, $5, $6)
            RETURNING id
        )";

        auto result = txn.exec_params(
            query,
            message.user_id,
            message.content,
            message.language,
            message.character_count,
            message.review_status,
            message.created_at
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, user_id, content, language, character_count, 
                   review_status, moderation_result, avatar_response, 
                   created_at, is_visible
            FROM messages
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, user_id, content, language, character_count, 
                   review_status, moderation_result, avatar_response, 
                   created_at, is_visible
            FROM messages
            WHERE user_id = $1 AND is_visible = TRUE
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, user_id, content, language, character_count, 
                   review_status, moderation_result, avatar_response, 
                   created_at, is_visible
            FROM messages
            WHERE review_status = 'pending'
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            UPDATE messages
            SET review_status = $1, moderation_result = $2, updated_at = NOW()
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            UPDATE messages
            SET avatar_response = $1, updated_at = NOW()
            WHERE id = $2
        )";

        txn.exec_params(query, response_data.dump(), message_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<void> MessageDAO::delete_(int64_t message_id) {
    try {
        pqxx::work txn(*conn_);

        std::string query = "UPDATE messages SET is_visible = FALSE WHERE id = $1";
        txn.exec_params(query, message_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Message MessageDAO::parseRow(const pqxx::row& row) {
    Message msg;
    msg.id = row["id"].as<int64_t>();
    msg.user_id = row["user_id"].as<int64_t>();
    msg.content = row["content"].as<std::string>();
    msg.language = row["language"].as<std::string>();
    msg.character_count = row["character_count"].as<int>();
    msg.review_status = row["review_status"].as<std::string>();

    if (!row["moderation_result"].is_null()) {
        msg.moderation_result = json::parse(row["moderation_result"].as<std::string>());
    }

    if (!row["avatar_response"].is_null()) {
        msg.avatar_response = json::parse(row["avatar_response"].as<std::string>());
    }

    msg.created_at = row["created_at"].as<int64_t>();
    msg.is_visible = row["is_visible"].as<bool>();

    return msg;
}

// ============ ConversationContextDAO 实现 ============

Result<int64_t> ConversationContextDAO::create(const ConversationContext& context) {
    try {
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            UPDATE conversation_contexts
            SET context_data = $1, message_history = $2, user_profile = $3,
                message_count = $4, updated_at = NOW()
            WHERE id = $5
        )";

        txn.exec_params(
            query,
            context.context_data.dump(),
            context.message_history.dump(),
            context.user_profile.dump(),
            context.message_count,
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
            message_history = json::array(
                message_history.begin() + message_history.size() - 20,
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

ConversationContext ConversationContextDAO::parseRow(const pqxx::row& row) {
    ConversationContext ctx;
    ctx.id = row["id"].as<int64_t>();
    ctx.user_id = row["user_id"].as<int64_t>();
    ctx.session_id = row["session_id"].as<std::string>();
    ctx.conversation_id = row["conversation_id"].as<std::string>();
    ctx.context_data = json::parse(row["context_data"].as<std::string>());
    ctx.message_history = json::parse(row["message_history"].as<std::string>());
    ctx.user_profile = json::parse(row["user_profile"].as<std::string>());
    ctx.message_count = row["message_count"].as<int>();
    ctx.created_at = row["created_at"].as<int64_t>();
    ctx.updated_at = row["updated_at"].as<int64_t>();
    ctx.is_active = row["is_active"].as<bool>();
    return ctx;
}

// ============ DatabaseService 实现 ============

DatabaseService& DatabaseService::getInstance() {
    static DatabaseService instance;
    return instance;
}

DatabaseService::DatabaseService()
    : initialized_(false),
      message_dao_(nullptr),
      context_dao_(nullptr),
      user_dao_(nullptr),
      moderation_dao_(nullptr) {}

bool DatabaseService::initialize(const std::string& connection_string) {
    try {
        if (!DatabasePool::getInstance().connect(connection_string)) {
            return false;
        }

        connection_ = DatabasePool::getInstance().getConnection();

        // 初始化所有 DAO
        new (&message_dao_) MessageDAO(connection_);
        new (&context_dao_) ConversationContextDAO(connection_);
        new (&user_dao_) UserDAO(connection_);
        new (&moderation_dao_) ModerationLogDAO(connection_);

        initialized_ = true;
        std::cout << "[DatabaseService] Initialized successfully" << std::endl;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseService] Initialization error: " << e.what() << std::endl;
        return false;
    }
}

Result<std::vector<Message>> DatabaseService::buildConversationHistory(int64_t user_id, int limit) {
    return message_dao_.getByUserId(user_id, limit, 0);
}

Result<json> DatabaseService::buildOpenClawContext(int64_t user_id) {
    try {
        json context;
        context["user_id"] = user_id;
        context["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

        // 获取最近的对话上下文
        auto ctx_result = context_dao_.getActiveContext(user_id);
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            INSERT INTO users 
            (username, email, password_hash, profile_data, preferences, is_active)
            VALUES ($1, $2, $3, $4, $5, $6)
            RETURNING id
        )";

        auto result = txn.exec_params(
            query,
            user.username,
            user.email,
            user.password_hash,
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, username, email, password_hash, profile_data, 
                   preferences, created_at, updated_at, last_login, is_active
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, username, email, password_hash, profile_data, 
                   preferences, created_at, updated_at, last_login, is_active
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
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT id, username, email, password_hash, profile_data, 
                   preferences, created_at, updated_at, last_login, is_active
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

Result<void> UserDAO::updatePreferences(int64_t user_id, const json& preferences) {
    try {
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
        pqxx::work txn(*conn_);

        std::string query = "UPDATE users SET last_login = NOW() WHERE id = $1";
        txn.exec_params(query, user_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

Result<void> UserDAO::delete_(int64_t user_id) {
    try {
        pqxx::work txn(*conn_);

        std::string query = "UPDATE users SET is_active = FALSE WHERE id = $1";
        txn.exec_params(query, user_id);
        txn.commit();

        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::Error(std::string(e.what()));
    }
}

User UserDAO::parseRow(const pqxx::row& row) {
    User user;
    user.id = row["id"].as<int64_t>();
    user.username = row["username"].as<std::string>();
    user.email = row["email"].as<std::string>();
    user.password_hash = row["password_hash"].as<std::string>();
    user.profile_data = json::parse(row["profile_data"].as<std::string>());
    user.preferences = json::parse(row["preferences"].as<std::string>());
    user.created_at = row["created_at"].as<int64_t>();
    user.updated_at = row["updated_at"].as<int64_t>();
    user.last_login = row["last_login"].is_null() ? 0 : row["last_login"].as<int64_t>();
    user.is_active = row["is_active"].as<bool>();
    return user;
}

// ============ ModerationLogDAO 实现 ============

Result<int64_t> ModerationLogDAO::create(const ModerationLog& log) {
    try {
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

ModerationLog ModerationLogDAO::parseRow(const pqxx::row& row) {
    ModerationLog log;
    log.id = row["id"].as<int64_t>();
    log.message_id = row["message_id"].as<int64_t>();
    log.user_id = row["user_id"].as<int64_t>();
    log.violation_type = row["violation_type"].as<std::string>();
    log.severity_score = row["severity_score"].as<double>();
    log.is_violation = row["is_violation"].as<bool>();
    log.violation_details = json::parse(row["violation_details"].as<std::string>());
    log.confidence_score = row["confidence_score"].as<double>();
    log.action_taken = row["action_taken"].as<std::string>();
    log.created_at = row["created_at"].as<int64_t>();
    return log;
}

} // namespace Yachiyo::Services
