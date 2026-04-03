#pragma once

#include "models/DatabaseModels.hpp"
#include <string>
#include <vector>
#include <memory>
#include <pqxx/pqxx>

namespace Yachiyo::Services {

using namespace Yachiyo::Models;

/**
 * 数据库连接池 (简化版)
 */
class DatabasePool {
public:
    static DatabasePool& getInstance();

    bool connect(const std::string& connection_string);
    bool isConnected() const;
    void close();

    std::shared_ptr<pqxx::connection> getConnection();

private:
    DatabasePool() = default;
    std::string connection_string_;
    std::shared_ptr<pqxx::connection> connection_;
};

/**
 * 数据访问对象 (DAO) - 消息表
 */
class MessageDAO {
public:
    MessageDAO(std::shared_ptr<pqxx::connection> conn) : conn_(conn) {}

    // 创建消息
    Result<int64_t> create(const Message& message);

    // 查询消息
    Result<Message> getById(int64_t message_id);

    // 查询用户的消息历史
    Result<std::vector<Message>> getByUserId(int64_t user_id, int limit = 50, int offset = 0);

    // 查询待审核消息
    Result<std::vector<Message>> getPendingReview(int limit = 100);

    // 更新审核结果
    Result<void> updateModerationResult(int64_t message_id,
                                        const std::string& status,
                                        const json& moderation_data);

    // 更新 Avatar 响应
    Result<void> updateAvatarResponse(int64_t message_id, const json& response_data);

    // 删除消息
    Result<void> delete_(int64_t message_id);

private:
    std::shared_ptr<pqxx::connection> conn_;

    Message parseRow(const pqxx::row& row);
};

/**
 * 数据访问对象 (DAO) - 对话上下文表
 */
class ConversationContextDAO {
public:
    ConversationContextDAO(std::shared_ptr<pqxx::connection> conn) : conn_(conn) {}

    // 创建对话上下文
    Result<int64_t> create(const ConversationContext& context);

    // 获取上下文
    Result<ConversationContext> getById(int64_t context_id);

    // 获取用户的所有上下文
    Result<std::vector<ConversationContext>> getByUserId(int64_t user_id);

    // 获取用户的活跃上下文
    Result<ConversationContext> getActiveContext(int64_t user_id);

    // 更新上下文
    Result<void> update(const ConversationContext& context);

    // 添加消息到历史
    Result<void> addMessageToHistory(int64_t context_id, const json& message_data);

private:
    std::shared_ptr<pqxx::connection> conn_;

    ConversationContext parseRow(const pqxx::row& row);
};

/**
 * 数据访问对象 (DAO) - 用户表
 */
class UserDAO {
public:
    UserDAO(std::shared_ptr<pqxx::connection> conn) : conn_(conn) {}

    // 创建用户
    Result<int64_t> create(const User& user);

    // 查询用户
    Result<User> getById(int64_t user_id);
    Result<User> getByUsername(const std::string& username);
    Result<User> getByEmail(const std::string& email);

    // 更新用户信息
    Result<void> updateProfile(int64_t user_id, const json& profile_data);
    Result<void> updatePreferences(int64_t user_id, const json& preferences);
    Result<void> updateLastLogin(int64_t user_id);

    // 删除用户
    Result<void> delete_(int64_t user_id);

private:
    std::shared_ptr<pqxx::connection> conn_;

    User parseRow(const pqxx::row& row);
};

/**
 * 数据访问对象 (DAO) - 审核日志表
 */
class ModerationLogDAO {
public:
    ModerationLogDAO(std::shared_ptr<pqxx::connection> conn) : conn_(conn) {}

    // 创建审核日志
    Result<int64_t> create(const ModerationLog& log);

    // 查询审核日志
    Result<std::vector<ModerationLog>> getByMessageId(int64_t message_id);
    Result<std::vector<ModerationLog>> getByUserId(int64_t user_id);

    // 查询高风险消息
    Result<std::vector<ModerationLog>> getHighRiskMessages(double severity_threshold = 0.8);

private:
    std::shared_ptr<pqxx::connection> conn_;

    ModerationLog parseRow(const pqxx::row& row);
};

/**
 * 统一的数据库服务
 */
class DatabaseService {
public:
    static DatabaseService& getInstance();

    bool initialize(const std::string& connection_string);
    bool isInitialized() const { return initialized_; }

    // DAO 访问
    MessageDAO& messageDAO() { return message_dao_; }
    ConversationContextDAO& contextDAO() { return context_dao_; }
    UserDAO& userDAO() { return user_dao_; }
    ModerationLogDAO& moderationDAO() { return moderation_dao_; }

    // 高级操作
    Result<std::vector<Message>> buildConversationHistory(int64_t user_id, int limit = 20);

    Result<json> buildOpenClawContext(int64_t user_id);

private:
    DatabaseService();

    bool initialized_;
    std::shared_ptr<pqxx::connection> connection_;

    MessageDAO message_dao_;
    ConversationContextDAO context_dao_;
    UserDAO user_dao_;
    ModerationLogDAO moderation_dao_;
};

} // namespace Yachiyo::Services
