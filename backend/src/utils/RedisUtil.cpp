#include "utils/RedisUtil.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>

namespace Yachiyo {
namespace Utils {

// 全局连接池实例
std::unique_ptr<RedisConnectionPool> RedisUtil::pool = nullptr;

// ==================== RedisConnection ====================

RedisConnection::RedisConnection(const RedisConfig& config)
    : context(nullptr), config(config), connected(false) {
}

RedisConnection::~RedisConnection() {
    disconnect();
}

bool RedisConnection::connect() {
    if (connected) {
        return true;
    }

    // 设置连接超时
    struct timeval timeout = {config.timeoutSeconds, 0};
    
    // 连接到Redis
    context = redisConnectWithTimeout(config.host.c_str(), config.port, timeout);
    if (!context || context->err) {
        if (context) {
            std::cerr << "[ERROR] Redis连接失败: " << context->errstr << std::endl;
            redisFree(context);
            context = nullptr;
        } else {
            std::cerr << "[ERROR] Redis连接上下文分配失败" << std::endl;
        }
        return false;
    }

    // 如果有密码，进行认证
    if (!config.password.empty()) {
        redisReply* reply = (redisReply*)redisCommand(context, "AUTH %s", config.password.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "[ERROR] Redis认证失败" << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(context);
            context = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }

    // 选择数据库
    if (config.database != 0) {
        redisReply* reply = (redisReply*)redisCommand(context, "SELECT %d", config.database);
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "[ERROR] Redis数据库选择失败" << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(context);
            context = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }

    connected = true;
    std::cout << "[INFO] Redis连接成功: " << config.host << ":" << config.port << std::endl;
    return true;
}

void RedisConnection::disconnect() {
    if (context) {
        redisFree(context);
        context = nullptr;
    }
    connected = false;
}

bool RedisConnection::isConnected() const {
    return connected && context != nullptr;
}

std::string RedisConnection::executeCommand(const std::string& command, 
                                           const std::vector<std::string>& args) {
    if (!isConnected()) {
        return "";
    }

    redisReply* reply = execute(command, args);
    if (!reply) {
        return "";
    }

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        result = std::to_string(reply->integer);
    } else if (reply->type == REDIS_REPLY_STATUS) {
        result = std::string(reply->str, reply->len);
    }

    freeReplyObject(reply);
    return result;
}

bool RedisConnection::set(const std::string& key, const std::string& value, int ttlSeconds) {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = nullptr;
    
    if (ttlSeconds > 0) {
        reply = (redisReply*)redisCommand(context, "SETEX %s %d %s", 
                                         key.c_str(), ttlSeconds, value.c_str());
    } else {
        reply = (redisReply*)redisCommand(context, "SET %s %s", 
                                         key.c_str(), value.c_str());
    }

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }

    freeReplyObject(reply);
    return true;
}

std::string RedisConnection::get(const std::string& key) {
    if (!isConnected()) {
        return "";
    }

    redisReply* reply = (redisReply*)redisCommand(context, "GET %s", key.c_str());
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }

    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return "";
    }

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }

    freeReplyObject(reply);
    return result;
}

bool RedisConnection::del(const std::string& key) {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "DEL %s", key.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }

    freeReplyObject(reply);
    return true;
}

bool RedisConnection::exists(const std::string& key) {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "EXISTS %s", key.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }

    bool result = reply->integer > 0;
    freeReplyObject(reply);
    return result;
}

bool RedisConnection::expire(const std::string& key, int ttlSeconds) {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "EXPIRE %s %d", 
                                                  key.c_str(), ttlSeconds);

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }

    freeReplyObject(reply);
    return true;
}

int RedisConnection::ttl(const std::string& key) {
    if (!isConnected()) {
        return -2;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "TTL %s", key.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return -2;
    }

    int result = reply->integer;
    freeReplyObject(reply);
    return result;
}

bool RedisConnection::hset(const std::string& key, const std::string& field, 
                          const std::string& value) {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "HSET %s %s %s", 
                                                  key.c_str(), field.c_str(), value.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }

    freeReplyObject(reply);
    return true;
}

std::string RedisConnection::hget(const std::string& key, const std::string& field) {
    if (!isConnected()) {
        return "";
    }

    redisReply* reply = (redisReply*)redisCommand(context, "HGET %s %s", 
                                                  key.c_str(), field.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }

    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return "";
    }

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }

    freeReplyObject(reply);
    return result;
}

std::map<std::string, std::string> RedisConnection::hgetall(const std::string& key) {
    std::map<std::string, std::string> result;

    if (!isConnected()) {
        return result;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "HGETALL %s", key.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return result;
    }

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (i + 1 < reply->elements) {
                std::string field(reply->element[i]->str, reply->element[i]->len);
                std::string value(reply->element[i + 1]->str, reply->element[i + 1]->len);
                result[field] = value;
            }
        }
    }

    freeReplyObject(reply);
    return result;
}

int64_t RedisConnection::lpush(const std::string& key, const std::string& value) {
    if (!isConnected()) {
        return 0;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "LPUSH %s %s", 
                                                  key.c_str(), value.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return 0;
    }

    int64_t result = reply->integer;
    freeReplyObject(reply);
    return result;
}

int64_t RedisConnection::rpush(const std::string& key, const std::string& value) {
    if (!isConnected()) {
        return 0;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "RPUSH %s %s", 
                                                  key.c_str(), value.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return 0;
    }

    int64_t result = reply->integer;
    freeReplyObject(reply);
    return result;
}

std::string RedisConnection::lpop(const std::string& key) {
    if (!isConnected()) {
        return "";
    }

    redisReply* reply = (redisReply*)redisCommand(context, "LPOP %s", key.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }

    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return "";
    }

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }

    freeReplyObject(reply);
    return result;
}

std::string RedisConnection::rpop(const std::string& key) {
    if (!isConnected()) {
        return "";
    }

    redisReply* reply = (redisReply*)redisCommand(context, "RPOP %s", key.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return "";
    }

    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return "";
    }

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }

    freeReplyObject(reply);
    return result;
}

std::vector<std::string> RedisConnection::lrange(const std::string& key, int start, int end) {
    std::vector<std::string> result;

    if (!isConnected()) {
        return result;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "LRANGE %s %d %d", 
                                                  key.c_str(), start, end);

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return result;
    }

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i++) {
            if (reply->element[i]->type == REDIS_REPLY_STRING) {
                result.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
            }
        }
    }

    freeReplyObject(reply);
    return result;
}

int64_t RedisConnection::publish(const std::string& channel, const std::string& message) {
    if (!isConnected()) {
        return 0;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "PUBLISH %s %s", 
                                                  channel.c_str(), message.c_str());

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return 0;
    }

    int64_t result = reply->integer;
    freeReplyObject(reply);
    return result;
}

void RedisConnection::subscribe(const std::string& channel, 
                               std::function<void(const std::string&, const std::string&)> callback) {
    if (!isConnected()) {
        return;
    }

    redisReply* reply = (redisReply*)redisCommand(context, "SUBSCRIBE %s", channel.c_str());
    if (reply) {
        freeReplyObject(reply);
    }

    // 在循环中接收消息
    while (redisGetReply(context, (void**)&reply) == REDIS_OK) {
        if (reply && reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3) {
            if (std::string(reply->element[0]->str, reply->element[0]->len) == "message") {
                std::string ch(reply->element[1]->str, reply->element[1]->len);
                std::string msg(reply->element[2]->str, reply->element[2]->len);
                callback(ch, msg);
            }
        }
        if (reply) {
            freeReplyObject(reply);
        }
    }
}

std::string RedisConnection::getError() const {
    if (!context) {
        return "未连接";
    }
    return context->errstr;
}

redisReply* RedisConnection::execute(const std::string& command, 
                                    const std::vector<std::string>& args) {
    if (!isConnected()) {
        return nullptr;
    }

    std::string fullCommand = command;
    for (const auto& arg : args) {
        fullCommand += " " + arg;
    }

    return (redisReply*)redisCommand(context, fullCommand.c_str());
}

void RedisConnection::freeReply(redisReply* reply) {
    if (reply) {
        freeReplyObject(reply);
    }
}

// ==================== RedisConnectionPool ====================

RedisConnectionPool::RedisConnectionPool(size_t maxConnections, const RedisConfig& config)
    : maxConnections(maxConnections), config(config) {
}

RedisConnectionPool::~RedisConnectionPool() {
    std::lock_guard<std::mutex> lock(mutex);
    idleConnections.clear();
    activeConnections.clear();
}

std::shared_ptr<RedisConnection> RedisConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex);

    // 等待空闲连接
    while (idleConnections.empty() && activeConnections.size() >= maxConnections) {
        condition.wait(lock);
    }

    std::shared_ptr<RedisConnection> connection;

    if (!idleConnections.empty()) {
        connection = idleConnections.back();
        idleConnections.pop_back();
    } else {
        connection = createConnection();
    }

    if (connection && connection->isConnected()) {
        activeConnections.push_back(connection);
        return connection;
    }

    return nullptr;
}

void RedisConnectionPool::releaseConnection(std::shared_ptr<RedisConnection> connection) {
    std::lock_guard<std::mutex> lock(mutex);

    // 从活跃连接中移除
    auto it = std::find(activeConnections.begin(), activeConnections.end(), connection);
    if (it != activeConnections.end()) {
        activeConnections.erase(it);
    }

    // 添加到空闲连接
    if (connection && connection->isConnected()) {
        idleConnections.push_back(connection);
    }

    condition.notify_one();
}

size_t RedisConnectionPool::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(mutex);
    return activeConnections.size();
}

size_t RedisConnectionPool::getIdleConnections() const {
    std::lock_guard<std::mutex> lock(mutex);
    return idleConnections.size();
}

size_t RedisConnectionPool::getMaxConnections() const {
    return maxConnections;
}

std::shared_ptr<RedisConnection> RedisConnectionPool::createConnection() {
    auto connection = std::make_shared<RedisConnection>(config);
    if (connection->connect()) {
        return connection;
    }
    return nullptr;
}

// ==================== RedisUtil ====================

bool RedisUtil::initializePool(const RedisConfig& config) {
    if (!pool) {
        pool = std::make_unique<RedisConnectionPool>(config.maxConnections, config);
        
        // 创建初始连接以验证配置
        auto conn = pool->getConnection();
        if (!conn) {
            std::cerr << "[ERROR] Redis连接池初始化失败" << std::endl;
            pool.reset();
            return false;
        }
        pool->releaseConnection(conn);
        
        std::cout << "[INFO] Redis连接池初始化成功" << std::endl;
        return true;
    }
    return true;
}

std::shared_ptr<RedisConnection> RedisUtil::getConnection() {
    if (!pool) {
        std::cerr << "[ERROR] Redis连接池未初始化" << std::endl;
        return nullptr;
    }
    return pool->getConnection();
}

void RedisUtil::releaseConnection(std::shared_ptr<RedisConnection> connection) {
    if (pool) {
        pool->releaseConnection(connection);
    }
}

void RedisUtil::shutdown() {
    pool.reset();
    std::cout << "[INFO] Redis连接池已关闭" << std::endl;
}

bool RedisUtil::setCache(const std::string& key, const std::string& value, int ttlSeconds) {
    auto conn = getConnection();
    if (!conn) {
        return false;
    }

    bool result = conn->set(key, value, ttlSeconds);
    releaseConnection(conn);
    return result;
}

std::string RedisUtil::getCache(const std::string& key) {
    auto conn = getConnection();
    if (!conn) {
        return "";
    }

    std::string result = conn->get(key);
    releaseConnection(conn);
    return result;
}

bool RedisUtil::deleteCache(const std::string& key) {
    auto conn = getConnection();
    if (!conn) {
        return false;
    }

    bool result = conn->del(key);
    releaseConnection(conn);
    return result;
}

bool RedisUtil::cacheExists(const std::string& key) {
    auto conn = getConnection();
    if (!conn) {
        return false;
    }

    bool result = conn->exists(key);
    releaseConnection(conn);
    return result;
}

int64_t RedisUtil::publishEvent(const std::string& channel, const std::string& message) {
    auto conn = getConnection();
    if (!conn) {
        return 0;
    }

    int64_t result = conn->publish(channel, message);
    releaseConnection(conn);
    return result;
}

void RedisUtil::subscribeEvent(const std::string& channel, 
                              std::function<void(const std::string&, const std::string&)> callback) {
    auto conn = getConnection();
    if (conn) {
        conn->subscribe(channel, callback);
        releaseConnection(conn);
    }
}

} // namespace Utils
} // namespace Yachiyo
