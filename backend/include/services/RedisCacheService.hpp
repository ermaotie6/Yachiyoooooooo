#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>
#include <optional>
#include "utils/DatabaseUtil.hpp"

namespace Yachiyo {
namespace Services {

/**
 * @brief Redis缓存服务类
 * 提供高级缓存功能，包括缓存穿透、缓存击穿、缓存雪崩防护
 */
class RedisCacheService {
public:
    RedisCacheService(std::shared_ptr<Utils::DatabaseUtil> dbUtil);
    ~RedisCacheService();
    
    // 禁止拷贝
    RedisCacheService(const RedisCacheService&) = delete;
    RedisCacheService& operator=(const RedisCacheService&) = delete;
    
    /**
     * @brief 设置缓存
     * @param key 缓存键
     * @param value 缓存值
     * @param ttlSeconds 过期时间（秒），0表示永不过期
     * @return 是否成功
     */
    bool set(const std::string& key, const std::string& value, int ttlSeconds = 0);
    
    /**
     * @brief 获取缓存
     * @param key 缓存键
     * @return 缓存值，如果不存在返回空字符串
     */
    std::string get(const std::string& key);
    
    /**
     * @brief 删除缓存
     * @param key 缓存键
     * @return 是否成功
     */
    bool del(const std::string& key);
    
    /**
     * @brief 检查缓存是否存在
     * @param key 缓存键
     * @return 是否存在
     */
    bool exists(const std::string& key);
    
    /**
     * @brief 设置缓存并设置过期时间
     * @param key 缓存键
     * @param value 缓存值
     * @param ttlSeconds 过期时间（秒）
     * @return 是否成功
     */
    bool setex(const std::string& key, const std::string& value, int ttlSeconds);
    
    /**
     * @brief 设置缓存（如果不存在）
     * @param key 缓存键
     * @param value 缓存值
     * @param ttlSeconds 过期时间（秒）
     * @return 是否设置成功（如果已存在返回false）
     */
    bool setnx(const std::string& key, const std::string& value, int ttlSeconds = 0);
    
    /**
     * @brief 获取并删除缓存
     * @param key 缓存键
     * @return 缓存值
     */
    std::string getdel(const std::string& key);
    
    /**
     * @brief 获取缓存并设置过期时间
     * @param key 缓存键
     * @param ttlSeconds 新的过期时间（秒）
     * @return 缓存值
     */
    std::string getex(const std::string& key, int ttlSeconds);
    
    /**
     * @brief 获取或设置缓存（缓存穿透防护）
     * @param key 缓存键
     * @param loader 数据加载函数（当缓存不存在时调用）
     * @param ttlSeconds 过期时间（秒）
     * @return 缓存值
     */
    std::string getOrSet(const std::string& key, 
                        std::function<std::string()> loader, 
                        int ttlSeconds = 3600);
    
    /**
     * @brief 获取或设置缓存（带互斥锁，防止缓存击穿）
     * @param key 缓存键
     * @param loader 数据加载函数
     * @param ttlSeconds 过期时间（秒）
     * @param mutexTimeout 互斥锁超时时间（毫秒）
     * @return 缓存值
     */
    std::string getOrSetWithMutex(const std::string& key,
                                 std::function<std::string()> loader,
                                 int ttlSeconds = 3600,
                                 int mutexTimeout = 5000);
    
    /**
     * @brief 批量获取缓存
     * @param keys 缓存键列表
     * @return 缓存值映射
     */
    std::unordered_map<std::string, std::string> mget(const std::vector<std::string>& keys);
    
    /**
     * @brief 批量设置缓存
     * @param keyValues 键值对映射
     * @param ttlSeconds 过期时间（秒）
     * @return 是否成功
     */
    bool mset(const std::unordered_map<std::string, std::string>& keyValues, int ttlSeconds = 0);
    
    /**
     * @brief 递增计数器
     * @param key 计数器键
     * @param increment 增量
     * @return 递增后的值
     */
    int64_t incr(const std::string& key, int64_t increment = 1);
    
    /**
     * @brief 递减计数器
     * @param key 计数器键
     * @param decrement 减量
     * @return 递减后的值
     */
    int64_t decr(const std::string& key, int64_t decrement = 1);
    
    /**
     * @brief 设置哈希表字段
     * @param key 哈希表键
     * @param field 字段名
     * @param value 字段值
     * @return 是否成功
     */
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    
    /**
     * @brief 获取哈希表字段
     * @param key 哈希表键
     * @param field 字段名
     * @return 字段值
     */
    std::string hget(const std::string& key, const std::string& field);
    
    /**
     * @brief 获取哈希表所有字段
     * @param key 哈希表键
     * @return 字段映射
     */
    std::unordered_map<std::string, std::string> hgetall(const std::string& key);
    
    /**
     * @brief 删除哈希表字段
     * @param key 哈希表键
     * @param field 字段名
     * @return 是否成功
     */
    bool hdel(const std::string& key, const std::string& field);
    
    /**
     * @brief 检查哈希表字段是否存在
     * @param key 哈希表键
     * @param field 字段名
     * @return 是否存在
     */
    bool hexists(const std::string& key, const std::string& field);
    
    /**
     * @brief 设置列表元素
     * @param key 列表键
     * @param value 元素值
     * @param left 是否从左侧插入（true=LPUSH，false=RPUSH）
     * @return 是否成功
     */
    bool listPush(const std::string& key, const std::string& value, bool left = true);
    
    /**
     * @brief 弹出列表元素
     * @param key 列表键
     * @param left 是否从左侧弹出（true=LPOP，false=RPOP）
     * @return 元素值
     */
    std::string listPop(const std::string& key, bool left = true);
    
    /**
     * @brief 获取列表范围
     * @param key 列表键
     * @param start 起始索引
     * @param stop 结束索引
     * @return 元素列表
     */
    std::vector<std::string> listRange(const std::string& key, int start = 0, int stop = -1);
    
    /**
     * @brief 添加集合元素
     * @param key 集合键
     * @param member 元素
     * @return 是否成功
     */
    bool sadd(const std::string& key, const std::string& member);
    
    /**
     * @brief 移除集合元素
     * @param key 集合键
     * @param member 元素
     * @return 是否成功
     */
    bool srem(const std::string& key, const std::string& member);
    
    /**
     * @brief 检查集合元素是否存在
     * @param key 集合键
     * @param member 元素
     * @return 是否存在
     */
    bool sismember(const std::string& key, const std::string& member);
    
    /**
     * @brief 获取集合所有元素
     * @param key 集合键
     * @return 元素列表
     */
    std::vector<std::string> smembers(const std::string& key);
    
    /**
     * @brief 添加有序集合元素
     * @param key 有序集合键
     * @param score 分数
     * @param member 元素
     * @return 是否成功
     */
    bool zadd(const std::string& key, double score, const std::string& member);
    
    /**
     * @brief 获取有序集合范围
     * @param key 有序集合键
     * @param start 起始排名
     * @param stop 结束排名
     * @param withScores 是否返回分数
     * @return 元素列表
     */
    std::vector<std::string> zrange(const std::string& key, int start = 0, int stop = -1, bool withScores = false);
    
    /**
     * @brief 发布消息
     * @param channel 频道
     * @param message 消息
     * @return 是否成功
     */
    bool publish(const std::string& channel, const std::string& message);
    
    /**
     * @brief 获取缓存统计信息
     * @return 统计信息
     */
    std::string getStats();
    
    /**
     * @brief 清空所有缓存
     * @return 是否成功
     */
    bool flushAll();
    
    /**
     * @brief 清空当前数据库
     * @return 是否成功
     */
    bool flushDB();
    
    /**
     * @brief 获取缓存键数量
     * @return 键数量
     */
    int64_t dbsize();
    
    /**
     * @brief 获取Redis信息
     * @param section 信息部分
     * @return 信息字符串
     */
    std::string info(const std::string& section = "");

private:
    // 生成互斥锁键
    std::string generateMutexKey(const std::string& key);
    
    // 获取互斥锁
    bool acquireMutex(const std::string& mutexKey, int timeoutMs);
    
    // 释放互斥锁
    bool releaseMutex(const std::string& mutexKey);
    
    // 检查是否为空值（用于缓存穿透防护）
    bool isNullValue(const std::string& value);
    
    // 生成空值标记
    std::string generateNullValue();
    
    // 成员变量
    std::shared_ptr<Utils::DatabaseUtil> dbUtil_;
    mutable std::mutex mutex_;
    
    // 统计信息
    struct Stats {
        int64_t hits = 0;
        int64_t misses = 0;
        int64_t sets = 0;
        int64_t deletes = 0;
        int64_t mutexAcquires = 0;
        int64_t mutexTimeouts = 0;
    } stats_;
};

} // namespace Services
} // namespace Yachiyo