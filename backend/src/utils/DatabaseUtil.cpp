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
    : redisContext(nullptr) {
}

DatabaseUtil::~DatabaseUtil() {
    disconnect();
}

bool DatabaseUtil::connect(const DatabaseConfig& config) {
    std::lock_guard<std::mutex> lock(mutex);
    
    this->config = config;
    
    try {
        // 创建连接池
        connectionPool = std::make_shared<pqxx::connection_pool>(config.poolSize);
        
        // 初始化连接池中的连接
        for (int i = 0; i < config.poolSize; ++i) {
            auto conn = connectionPool->acquire();
            if (!testConnection(*conn)) {
                std::cerr << "Failed to initialize connection in pool" << std::endl;
                return false;
            }
            setupConnection(*conn);
        }
        
        std::cout << "Database connected successfully with " << config.poolSize << " connections" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseUtil::connectRedis(const RedisConfig& config) {
    std::lock_guard<std::mutex> lock(mutex);
    
    redisConfig_ = config;
    
    // 断开现有连接
    if (redisContext) {
        redisFree(redisContext);
        redisContext = nullptr;
    }
    
    // 设置连接超时
    struct timeval timeout = {config.timeout, 0};
    
    // 连接到Redis
    redisContext = redisConnectWithTimeout(config.host.c_str(), config.port, timeout);
    if (!redisContext || redisContext->err) {
        if (redisContext) {
            std::cerr << "Redis connection error: " << redisContext->errstr << std::endl;
            redisFree(redisContext);
            redisContext = nullptr;
        } else {
            std::cerr << "Failed to allocate Redis context" << std::endl;
        }
        return false;
    }
    
    // 如果有密码，进行认证
    if (!config.password.empty()) {
        redisReply* reply = (redisReply*)redisCommand(redisContext, "AUTH %s", config.password.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "Redis authentication failed" << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(redisContext);
            redisContext = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }
    
    // 选择数据库
    if (config.db != 0) {
        redisReply* reply = (redisReply*)redisCommand(redisContext, "SELECT %d", config.db);
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "Failed to select Redis database" << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(redisContext);
            redisContext = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }
    
    std::cout << "Redis connected successfully to " << config.host << ":" << config.port 
              << " (db=" << config.db << ")" << std::endl;
    return true;
}

void DatabaseUtil::disconnect() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 断开数据库连接
    if (connectionPool) {
        connectionPool.reset();
    }
    
    // 断开Redis连接
    if (redisContext) {
        redisFree(redisContext);
        redisContext = nullptr;
    }
    
    std::cout << "All database connections closed" << std::endl;
}

bool DatabaseUtil::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex);
    return connectionPool != nullptr;
}

bool DatabaseUtil::isRedisConnected() const {
    std::lock_guard<std::mutex> lock(mutex);
    return redisContext != nullptr;
}

pqxx::connection DatabaseUtil::getConnection() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!connectionPool) {
        throw std::runtime_error("Database not connected");
    }
    
    try {
        return connectionPool->acquire();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to acquire connection: " + std::string(e.what()));
    }
}

bool DatabaseUtil::executeQuery(const std::string& query) {
    try {
        auto conn = getConnection();
        pqxx::work txn(*conn);
        txn.exec(query);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Query execution failed: " << e.what() << std::endl;
        return false;
    }
}

pqxx::result DatabaseUtil::executeQueryWithResult(const std::string& query) {
    try {
        auto conn = getConnection();
        pqxx::work txn(*conn);
        auto result = txn.exec(query);
        txn.commit();
        return result;
    } catch (const std::exception& e) {
        throw std::runtime_error("Query execution failed: " + std::string(e.what()));
    }
}

bool DatabaseUtil::executeTransaction(std::function<bool(pqxx::work&)> transactionFunc) {
    try {
        auto conn = getConnection();
        pqxx::work txn(*conn);
        
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

bool DatabaseUtil::executePreparedStatement(const std::string& name, 
                                           const std::vector<std::string>& params) {
    try {
        auto conn = getConnection();
        pqxx::work txn(*conn);
        
        // 准备语句（如果尚未准备）
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
        
        // 执行预编译语句
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

// Redis操作实现
bool DatabaseUtil::redisSet(const std::string& key, const std::string& value, int expireSeconds) {
    if (!redisContext) return false;
    
    redisReply* reply = nullptr;
    if (expireSeconds > 0) {
        reply = (redisReply*)redisCommand(redisContext, "SET %s %s EX %d", 
                                         key.c_str(), value.c_str(), expireSeconds);
    } else {
        reply = (redisReply*)redisCommand(redisContext, "SET %s %s", 
                                         key.c_str(), value.c_str());
    }
    
    bool success = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return success;
}

std::string DatabaseUtil::redisGet(const std::string& key) {
    if (!redisContext) return "";
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "GET %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string result = reply->str ? reply->str : "";
    freeReplyObject(reply);
    return result;
}

bool DatabaseUtil::redisDelete(const std::string& key) {
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "DEL %s", key.c_str());
    bool success = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return success;
}

bool DatabaseUtil::redisExists(const std::string& key) {
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "EXISTS %s", key.c_str());
    bool exists = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return exists;
}

bool DatabaseUtil::redisExpire(const std::string& key, int seconds) {
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "EXPIRE %s %d", key.c_str(), seconds);
    bool success = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return success;
}

// 哈希表操作
bool DatabaseUtil::redisHSet(const std::string& key, const std::string& field, const std::string& value) {
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "HSET %s %s %s", 
                                                 key.c_str(), field.c_str(), value.c_str());
    bool success = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return success;
}

std::string DatabaseUtil::redisHGet(const std::string& key, const std::string& field) {
    if (!redisContext) return "";
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "HGET %s %s", 
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
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "HDEL %s %s", 
                                                 key.c_str(), field.c_str());
    bool success = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return success;
}

std::vector<std::string> DatabaseUtil::redisHGetAll(const std::string& key) {
    std::vector<std::string> result;
    if (!redisContext) return result;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "HGETALL %s", key.c_str());
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
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "LPUSH %s %s", 
                                                 key.c_str(), value.c_str());
    bool success = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return success;
}

bool DatabaseUtil::redisRPush(const std::string& key, const std::string& value) {
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "RPUSH %s %s", 
                                                 key.c_str(), value.c_str());
    bool success = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return success;
}

std::string DatabaseUtil::redisLPop(const std::string& key) {
    if (!redisContext) return "";
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "LPOP %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string result = reply->str ? reply->str : "";
    freeReplyObject(reply);
    return result;
}

std::string DatabaseUtil::redisRPop(const std::string& key) {
    if (!redisContext) return "";
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "RPOP %s", key.c_str());
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
    if (!redisContext) return result;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "LRANGE %s %d %d", 
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
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "SADD %s %s", 
                                                 key.c_str(), member.c_str());
    bool success = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return success;
}

bool DatabaseUtil::redisSRem(const std::string& key, const std::string& member) {
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "SREM %s %s", 
                                                 key.c_str(), member.c_str());
    bool success = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return success;
}

bool DatabaseUtil::redisSIsMember(const std::string& key, const std::string& member) {
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "SISMEMBER %s %s", 
                                                 key.c_str(), member.c_str());
    bool isMember = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    return isMember;
}

std::vector<std::string> DatabaseUtil::redisSMembers(const std::string& key) {
    std::vector<std::string> result;
    if (!redisContext) return result;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "SMEMBERS %s", key.c_str());
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
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "ZADD %s %f %s", 
                                                 key.c_str(), score, member.c_str());
    bool success = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply) freeReplyObject(reply);
    return success;
}

bool DatabaseUtil::redisZRem(const std::string& key, const std::string& member) {
    if (!redisContext) return false;
    
    redisReply* reply = (redisReply*)redisCommand(redisContext, "ZREM %s %s", 
                                                 key.c_str(), member.c_str());
    bool success = reply && reply->type != REDIS_REPLY_ERROR && reply->integer > 0;
    if (reply) freeReplyObject(reply);
