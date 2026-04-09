#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <hiredis/hiredis.h>

namespace Yachiyo {
namespace Utils {

// Redis配置
struct RedisConfig {
    std::string host = "localhost";
    int port = 6379;
    std::string password;
    int database = 0;
    int timeoutSeconds = 5;
    int maxConnections = 10;
    bool enableSSL = false;
    std::string sslCertPath;
    std::string sslKeyPath;
    std::string sslCaPath;
};

/**
 * @brief Redis连接类
 */
class RedisConnection {
public:
    RedisConnection(const RedisConfig& config);
    ~RedisConnection();

    /**
     * @brief 连接Redis
     * @return 是否连接成功
     */
    bool connect();

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 是否已连接
     * @return 连接状态
     */
    bool isConnected() const;

    /**
     * @brief 执行Redis命令
     * @param command 命令
     * @param args 参数列表
     * @return 执行结果
     */
    std::string executeCommand(const std::string& command, 
                              const std::vector<std::string>& args = {});

    /**
     * @brief 设置键值
     * @param key 键
     * @param value 值
     * @param ttlSeconds 过期时间（秒，0表示不过期）
     * @return 是否成功
     */
    bool set(const std::string& key, const std::string& value, int ttlSeconds = 0);

    /**
     * @brief 获取键值
     * @param key 键
     * @return 值（如果键不存在返回空字符串）
     */
    std::string get(const std::string& key);

    /**
     * @brief 删除键
     * @param key 键
     * @return 是否成功
     */
    bool del(const std::string& key);

    /**
     * @brief 检查键是否存在
     * @param key 键
     * @return 是否存在
     */
    bool exists(const std::string& key);

    /**
     * @brief 设置过期时间
     * @param key 键
     * @param ttlSeconds 过期时间（秒）
     * @return 是否成功
     */
    bool expire(const std::string& key, int ttlSeconds);

    /**
     * @brief 获取剩余过期时间
     * @param key 键
     * @return 剩余时间（秒，-1表示永不过期，-2表示键不存在）
     */
    int ttl(const std::string& key);

    /**
     * @brief 哈希表设置字段
     * @param key 哈希表键
     * @param field 字段
     * @param value 值
     * @return 是否成功
     */
    bool hset(const std::string& key, const std::string& field, const std::string& value);

    /**
     * @brief 哈希表获取字段
     * @param key 哈希表键
     * @param field 字段
     * @return 值
     */
    std::string hget(const std::string& key, const std::string& field);

    /**
     * @brief 哈希表获取所有字段
     * @param key 哈希表键
     * @return 字段-值映射
     */
    std::map<std::string, std::string> hgetall(const std::string& key);

    /**
     * @brief 列表左推
     * @param key 列表键
     * @param value 值
     * @return 列表长度
     */
    int64_t lpush(const std::string& key, const std::string& value);

    /**
     * @brief 列表右推
     * @param key 列表键
     * @param value 值
     * @return 列表长度
     */
    int64_t rpush(const std::string& key, const std::string& value);

    /**
     * @brief 列表左弹出
     * @param key 列表键
     * @return 弹出的值
     */
    std::string lpop(const std::string& key);

    /**
     * @brief 列表右弹出
     * @param key 列表键
     * @return 弹出的值
     */
    std::string rpop(const std::string& key);

    /**
     * @brief 获取列表范围
     * @param key 列表键
     * @param start 起始索引
     * @param end 结束索引
     * @return 值列表
     */
    std::vector<std::string> lrange(const std::string& key, int start, int end);

    /**
     * @brief 发布消息
     * @param channel 频道
     * @param message 消息
     * @return 接收消息的客户端数量
     */
    int64_t publish(const std::string& channel, const std::string& message);

    /**
     * @brief 订阅频道
     * @param channel 频道
     * @param callback 消息回调函数
     */
    void subscribe(const std::string& channel, 
                  std::function<void(const std::string&, const std::string&)> callback);

    /**
     * @brief 获取错误信息
     * @return 错误信息
     */
    std::string getError() const;

private:
    // 执行Redis命令并获取回复
    redisReply* execute(const std::string& command, const std::vector<std::string>& args);

    // 释放回复
    void freeReply(redisReply* reply);

    // Redis连接上下文
    redisContext* context;
    
    // 配置
    RedisConfig config;
    
    // 连接状态
    bool connected;
};

/**
 * @brief Redis连接池
 */
class RedisConnectionPool {
public:
    RedisConnectionPool(size_t maxConnections, const RedisConfig& config);
    ~RedisConnectionPool();

    /**
     * @brief 获取Redis连接
     * @return Redis连接
     */
    std::shared_ptr<RedisConnection> getConnection();

    /**
     * @brief 释放Redis连接
     * @param connection Redis连接
     */
    void releaseConnection(std::shared_ptr<RedisConnection> connection);

    /**
     * @brief 获取当前活跃连接数
     * @return 活跃连接数
     */
    size_t getActiveConnections() const;

    /**
     * @brief 获取空闲连接数
     * @return 空闲连接数
     */
    size_t getIdleConnections() const;

    /**
     * @brief 获取最大连接数
     * @return 最大连接数
     */
    size_t getMaxConnections() const;

private:
    // 创建新连接
    std::shared_ptr<RedisConnection> createConnection();

    // 连接池参数
    size_t maxConnections;
    RedisConfig config;
    
    // 连接池状态
    std::vector<std::shared_ptr<RedisConnection>> idleConnections;
    std::vector<std::shared_ptr<RedisConnection>> activeConnections;
    
    // 线程安全
    mutable std::mutex mutex;
    std::condition_variable condition;
};

/**
 * @brief Redis工具类
 */
class RedisUtil {
public:
    /**
     * @brief 初始化Redis连接池
     * @param config Redis配置
     * @return 是否成功
     */
    static bool initializePool(const RedisConfig& config = RedisConfig());

    /**
     * @brief 获取Redis连接
     * @return Redis连接
     */
    static std::shared_ptr<RedisConnection> getConnection();

    /**
     * @brief 释放Redis连接
     * @param connection Redis连接
     */
    static void releaseConnection(std::shared_ptr<RedisConnection> connection);

    /**
     * @brief 关闭连接池
     */
    static void shutdown();

    /**
     * @brief 设置缓存
     * @param key 键
     * @param value 值
     * @param ttlSeconds 过期时间
     * @return 是否成功
     */
    static bool setCache(const std::string& key, const std::string& value, int ttlSeconds = 3600);

    /**
     * @brief 获取缓存
     * @param key 键
     * @return 值
     */
    static std::string getCache(const std::string& key);

    /**
     * @brief 删除缓存
     * @param key 键
     * @return 是否成功
     */
    static bool deleteCache(const std::string& key);

    /**
     * @brief 检查缓存是否存在
     * @param key 键
     * @return 是否存在
     */
    static bool cacheExists(const std::string& key);

    /**
     * @brief 发布事件
     * @param channel 频道
     * @param message 消息
     * @return 接收消息的客户端数量
     */
    static int64_t publishEvent(const std::string& channel, const std::string& message);

    /**
     * @brief 订阅事件
     * @param channel 频道
     * @param callback 消息回调函数
     */
    static void subscribeEvent(const std::string& channel, 
                              std::function<void(const std::string&, const std::string&)> callback);

private:
    // 连接池单例
    static std::unique_ptr<RedisConnectionPool> pool;
    
    // 禁止实例化
    RedisUtil() = delete;
    ~RedisUtil() = delete;
};

} // namespace Utils
} // namespace Yachiyo