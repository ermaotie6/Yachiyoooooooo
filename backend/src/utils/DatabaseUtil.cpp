#include "utils/DatabaseUtil.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <cstring>

namespace Yachiyo {
namespace Utils {

DatabaseUtil::DatabaseUtil() 
    : redisCtx_(nullptr) {
}

DatabaseUtil::~DatabaseUtil() {
    disconnect();
}

std::string DatabaseUtil::buildConnectionString() const {
    std::string connStr = "host=" + config.host +
                          " port=" + std::to_string(config.port) +
                          " dbname=" + config.database +
                          " user=" + config.username;
    if (!config.password.empty()) {
        connStr += " password=" + config.password;
    }
    connStr += " connect_timeout=" + std::to_string(config.connectionTimeout);
    return connStr;
}

bool DatabaseUtil::connect(const DatabaseConfig& cfg) {
    std::lock_guard<std::mutex> lock(mutex);
    
    this->config = cfg;
    connectionString_ = buildConnectionString();
    
    try {
        // 测试连接是否可用
        pqxx::connection conn(connectionString_);
        if (!testConnection(conn)) {
            std::cerr << "Failed to test database connection" << std::endl;
            return false;
        }
        
        std::cout << "Database connected successfully to " 
                  << config.host << ":" << config.port 
                  << "/" << config.database << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        connectionString_.clear();
        return false;
    }
}

bool DatabaseUtil::connectRedis(const RedisConfig& cfg) {
    std::lock_guard<std::mutex> lock(mutex);
    
    redisConfig_ = cfg;
    
    // 断开现有连接
    if (redisCtx_) {
        redisFree(redisCtx_);
        redisCtx_ = nullptr;
    }
    
    // 设置连接超时
    struct timeval timeout = {cfg.timeoutSeconds, 0};
    
    // 连接到Redis
    redisCtx_ = redisConnectWithTimeout(cfg.host.c_str(), cfg.port, timeout);
    if (!redisCtx_ || redisCtx_->err) {
        if (redisCtx_) {
            std::cerr << "Redis connection error: " << redisCtx_->errstr << std::endl;
            redisFree(redisCtx_);
            redisCtx_ = nullptr;
        } else {
            std::cerr << "Failed to allocate Redis context" << std::endl;
        }
        return false;
    }
    
    // 如果有密码，进行认证
    if (!cfg.password.empty()) {
        redisReply* reply = (redisReply*)redisCommand(redisCtx_, "AUTH %s", cfg.password.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "Redis authentication failed" << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(redisCtx_);
            redisCtx_ = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }
    
    // 选择数据库
    if (cfg.database != 0) {
        redisReply* reply = (redisReply*)redisCommand(redisCtx_, "SELECT %d", cfg.database);
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "Failed to select Redis database" << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(redisCtx_);
            redisCtx_ = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }
    
    std::cout << "Redis connected successfully to " << cfg.host << ":" << cfg.port 
              << " (db=" << cfg.database << ")" << std::endl;
    return true;
}

void DatabaseUtil::disconnect() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 清空连接字符串，表示断开
    connectionString_.clear();
    
    // 断开Redis连接
    if (redisCtx_) {
        redisFree(redisCtx_);
        redisCtx_ = nullptr;
    }
    
    std::cout << "All database connections closed" << std::endl;
}

bool DatabaseUtil::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex);
    return !connectionString_.empty();
}

bool DatabaseUtil::isRedisConnected() const {
    std::lock_guard<std::mutex> lock(mutex);
    return redisCtx_ != nullptr;
}

pqxx::connection DatabaseUtil::getConnection() {
    std::string connStr;
    {
        std::lock_guard<std::mutex> lock(mutex);
        connStr = connectionString_;
    }
    
    if (connStr.empty()) {
        throw std::runtime_error("Database not connected");
    }
    
    try {
        return pqxx::connection(connStr);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to create connection: " + std::string(e.what()));
    }
}

bool DatabaseUtil::testConnection(pqxx::connection& conn) {
    try {
        pqxx::work txn(conn);
        txn.exec("SELECT 1");
        txn.commit();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void DatabaseUtil::setupConnection(pqxx::connection& /*conn*/) {
    // 预留: 设置连接参数 (如 search_path, timezone 等)
}

bool DatabaseUtil::isStatementPrepared(const std::string& name) {
    return preparedStatements.find(name) != preparedStatements.end();
}

bool DatabaseUtil::executeQuery(const std::string& queryStr) {
    try {
        auto conn = getConnection();
        pqxx::work txn(conn);
        txn.exec(queryStr);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Query execution failed: " << e.what() << std::endl;
        return false;
    }
}

pqxx::result DatabaseUtil::executeQueryWithResult(const std::string& queryStr) {
    try {
        auto conn = getConnection();
        pqxx::work txn(conn);
        auto result = txn.exec(queryStr);
        txn.commit();
        return result;
    } catch (const std::exception& e) {
        throw std::runtime_error("Query execution failed: " + std::string(e.what()));
    }
}

bool DatabaseUtil::executeTransaction(std::function<bool(pqxx::work&)> transactionFunc) {
    try {
        auto conn = getConnection();
        pqxx::work txn(conn);
        
        if (transactionFunc(txn)) {
            txn.commit();
            return true;
        } else {
            txn.abort();
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Transaction failed: " << e.what() << std::endl;
        return false;
    }
}

// ==================== 参数化查询接口 ====================

std::vector<std::map<std::string, std::string>> DatabaseUtil::query(
    const std::string& sql,
    const std::vector<std::string>& params
) {
    std::vector<std::map<std::string, std::string>> results;
    try {
        auto conn = getConnection();
        pqxx::work txn(conn);

        pqxx::result res;
        if (params.empty()) {
            res = txn.exec(sql);
        } else {
            pqxx::params p;
            for (const auto& param : params) {
                p.append(param);
            }
            res = txn.exec_params(sql, p);
        }
        txn.commit();

        for (const auto& row : res) {
            std::map<std::string, std::string> rowMap;
            for (int col = 0; col < static_cast<int>(row.size()); ++col) {
                std::string colName = res.column_name(col);
                rowMap[colName] = row[col].is_null() ? "" : row[col].as<std::string>();
            }
            results.push_back(std::move(rowMap));
        }
    } catch (const std::exception& e) {
        std::cerr << "Parameterized query failed: " << e.what() << std::endl;
    }
    return results;
}

std::vector<std::map<std::string, std::string>> DatabaseUtil::insert(
    const std::string& sql,
    const std::vector<std::string>& params
) {
    return query(sql, params);
}

int DatabaseUtil::execute(
    const std::string& sql,
    const std::vector<std::string>& params
) {
    try {
        auto conn = getConnection();
        pqxx::work txn(conn);

        pqxx::result res;
        if (params.empty()) {
            res = txn.exec(sql);
        } else {
            pqxx::params p;
            for (const auto& param : params) {
                p.append(param);
            }
            res = txn.exec_params(sql, p);
        }
        txn.commit();
        return static_cast<int>(res.affected_rows());
    } catch (const std::exception& e) {
        std::cerr << "Execute failed: " << e.what() << std::endl;
        return -1;
    }
}

bool DatabaseUtil::executePreparedStatement(const std::string& name, 
                                           const std::vector<std::string>& params) {
    try {
        auto conn = getConnection();
        pqxx::work txn(conn);
        
        if (!isStatementPrepared(name)) {
            std::stringstream ss;
            ss << "PREPARE " << name << " AS ";
            ss << "SELECT * FROM table WHERE ";
            
            for (size_t i = 0; i < params.size(); ++i) {
                if (i > 0) ss << " AND ";
                ss << "field" << i + 1 << " = $" << i + 1;
            }
            
            txn.exec(ss.str());
            preparedStatements.insert(name);
        }
        
        std::stringstream execCmd;
        execCmd << "EXECUTE " << name << "(";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) execCmd << ", ";
            execCmd << txn.quote(params[i]);
        }
        execCmd << ")";
        
        txn.exec(execCmd.str());
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Prepared statement execution failed: " << e.what() << std::endl;
        return false;
    }
}

// ==================== Redis操作实现 ====================

bool DatabaseUtil::redisSet(const std::string& key, const std::string& value, int expireSeconds) {
    if (!redisCtx_) return false;
    
    redisReply* reply = nullptr;
    if (expireSeconds > 0) {
        reply = (redisReply*)redisCommand(redisCtx_, "SET %s %s EX %d", 
                                         key.c_str(), value.c_str(), expireSeconds);
    } else {
        reply = (redisReply*)redisCommand(redisCtx_, "SET %s %s", 
                                         key.c_str(), value.c_str());
    }
    
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return ok;
}

std::string DatabaseUtil::redisGet(const std::string& key) {
    if (!redisCtx_) return "";
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "GET %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string result = reply->str ? reply->str : "";
    freeReplyObject(reply);
    return result;
}

bool DatabaseUtil::redisDelete(const std::string& key) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "DEL %s", key.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return ok;
}

bool DatabaseUtil::redisExists(const std::string& key) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "EXISTS %s", key.c_str());
    bool exists = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return exists;
}

bool DatabaseUtil::redisExpire(const std::string& key, int seconds) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "EXPIRE %s %d", key.c_str(), seconds);
    bool ok = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return ok;
}

// 哈希表操作
bool DatabaseUtil::redisHSet(const std::string& key, const std::string& field, const std::string& value) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "HSET %s %s %s", 
                                                 key.c_str(), field.c_str(), value.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return ok;
}

std::string DatabaseUtil::redisHGet(const std::string& key, const std::string& field) {
    if (!redisCtx_) return "";
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "HGET %s %s", 
                                                 key.c_str(), field.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string result = reply->str ? reply->str : "";
    freeReplyObject(reply);
    return result;
}

bool DatabaseUtil::redisHDel(const std::string& key, const std::string& field) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "HDEL %s %s", 
                                                 key.c_str(), field.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return ok;
}

std::vector<std::string> DatabaseUtil::redisHGetAll(const std::string& key) {
    std::vector<std::string> result;
    if (!redisCtx_) return result;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "HGETALL %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return result;
    }
    
    for (size_t i = 0; i < reply->elements; ++i) {
        if (reply->element[i]->str) {
            result.push_back(reply->element[i]->str);
        }
    }
    
    freeReplyObject(reply);
    return result;
}

// 列表操作
bool DatabaseUtil::redisLPush(const std::string& key, const std::string& value) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "LPUSH %s %s", 
                                                 key.c_str(), value.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return ok;
}

bool DatabaseUtil::redisRPush(const std::string& key, const std::string& value) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "RPUSH %s %s", 
                                                 key.c_str(), value.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return ok;
}

std::string DatabaseUtil::redisLPop(const std::string& key) {
    if (!redisCtx_) return "";
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "LPOP %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string result = reply->str ? reply->str : "";
    freeReplyObject(reply);
    return result;
}

std::string DatabaseUtil::redisRPop(const std::string& key) {
    if (!redisCtx_) return "";
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "RPOP %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string result = reply->str ? reply->str : "";
    freeReplyObject(reply);
    return result;
}

std::vector<std::string> DatabaseUtil::redisLRange(const std::string& key, int start, int stop) {
    std::vector<std::string> result;
    if (!redisCtx_) return result;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "LRANGE %s %d %d", 
                                                 key.c_str(), start, stop);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return result;
    }
    
    for (size_t i = 0; i < reply->elements; ++i) {
        if (reply->element[i]->str) {
            result.push_back(reply->element[i]->str);
        }
    }
    
    freeReplyObject(reply);
    return result;
}

// 集合操作
bool DatabaseUtil::redisSAdd(const std::string& key, const std::string& member) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "SADD %s %s", 
                                                 key.c_str(), member.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return ok;
}

bool DatabaseUtil::redisSRem(const std::string& key, const std::string& member) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "SREM %s %s", 
                                                 key.c_str(), member.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return ok;
}

bool DatabaseUtil::redisSIsMember(const std::string& key, const std::string& member) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "SISMEMBER %s %s", 
                                                 key.c_str(), member.c_str());
    bool isMember = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return isMember;
}

std::vector<std::string> DatabaseUtil::redisSMembers(const std::string& key) {
    std::vector<std::string> result;
    if (!redisCtx_) return result;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "SMEMBERS %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return result;
    }
    
    for (size_t i = 0; i < reply->elements; ++i) {
        if (reply->element[i]->str) {
            result.push_back(reply->element[i]->str);
        }
    }
    
    freeReplyObject(reply);
    return result;
}

// 有序集合操作
bool DatabaseUtil::redisZAdd(const std::string& key, double score, const std::string& member) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "ZADD %s %f %s", 
                                                 key.c_str(), score, member.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return ok;
}

bool DatabaseUtil::redisZRem(const std::string& key, const std::string& member) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "ZREM %s %s", 
                                                 key.c_str(), member.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return ok;
}

std::vector<std::string> DatabaseUtil::redisZRange(const std::string& key, int start, int stop, bool withScores) {
    std::vector<std::string> result;
    if (!redisCtx_) return result;
    
    redisReply* reply = nullptr;
    if (withScores) {
        reply = (redisReply*)redisCommand(redisCtx_, "ZRANGE %s %d %d WITHSCORES", 
                                         key.c_str(), start, stop);
    } else {
        reply = (redisReply*)redisCommand(redisCtx_, "ZRANGE %s %d %d", 
                                         key.c_str(), start, stop);
    }
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return result;
    }
    
    for (size_t i = 0; i < reply->elements; ++i) {
        if (reply->element[i]->str) {
            result.push_back(reply->element[i]->str);
        }
    }
    
    freeReplyObject(reply);
    return result;
}

// 发布订阅
bool DatabaseUtil::redisPublish(const std::string& channel, const std::string& message) {
    if (!redisCtx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "PUBLISH %s %s", 
                                                 channel.c_str(), message.c_str());
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return ok;
}

// 数据库管理
bool DatabaseUtil::backupDatabase(const std::string& backupPath) {
    try {
        std::string cmd = "pg_dump -h " + config.host + 
                         " -p " + std::to_string(config.port) +
                         " -U " + config.username +
                         " -d " + config.database +
                         " -f " + backupPath;
        return system(cmd.c_str()) == 0;
    } catch (const std::exception& e) {
        std::cerr << "Backup failed: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseUtil::restoreDatabase(const std::string& backupPath) {
    try {
        std::string cmd = "psql -h " + config.host + 
                         " -p " + std::to_string(config.port) +
                         " -U " + config.username +
                         " -d " + config.database +
                         " -f " + backupPath;
        return system(cmd.c_str()) == 0;
    } catch (const std::exception& e) {
        std::cerr << "Restore failed: " << e.what() << std::endl;
        return false;
    }
}

// 连接池统计 (连接字符串模式无真正连接池，返回简化值)
int DatabaseUtil::getActiveConnections() const {
    return isConnected() ? 1 : 0;
}

int DatabaseUtil::getIdleConnections() const {
    return 0;
}

int DatabaseUtil::getTotalConnections() const {
    return isConnected() ? 1 : 0;
}

// Redis统计
std::string DatabaseUtil::redisInfo(const std::string& section) {
    if (!redisCtx_) return "";
    
    redisReply* reply = nullptr;
    if (section.empty()) {
        reply = (redisReply*)redisCommand(redisCtx_, "INFO");
    } else {
        reply = (redisReply*)redisCommand(redisCtx_, "INFO %s", section.c_str());
    }
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string result = reply->str ? reply->str : "";
    freeReplyObject(reply);
    return result;
}

int64_t DatabaseUtil::redisDBSize() {
    if (!redisCtx_) return 0;
    
    redisReply* reply = (redisReply*)redisCommand(redisCtx_, "DBSIZE");
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return 0;
    }
    
    int64_t size = reply->integer;
    freeReplyObject(reply);
    return size;
}

} // namespace Utils
} // namespace Yachiyo
