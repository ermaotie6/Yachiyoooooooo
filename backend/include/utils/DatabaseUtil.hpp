#pragma once

/**
 * @file DatabaseUtil.hpp
 * 
 * @note 架构待改进项:
 * - P1-1: 项目中存在两套数据库访问机制 (DatabaseUtil 参数化查询 vs DatabaseService DAO)。
 *         建议统一为一套，或明确分层: DatabaseUtil 供底层工具使用，DatabaseService 供业务服务使用。
 * - P1-3: 项目中存在两套 Redis API (RedisUtil 静态方法 vs DatabaseUtil 实例方法)。
 *         建议统一为 RedisUtil 一套，DatabaseUtil 中的 Redis 方法仅作为兼容层委托给 RedisUtil。
 * - P2-3: DatabasePool 名义上是连接池，但底层实现依赖 pqxx::connection_pool，
 *         实际池大小取决于配置。名称无误，但需确保 config.poolSize > 1。
 * - P1-7: 密码哈希使用 SHA-256+随机盐，安全性低于 bcrypt/argon2。
 *         建议引入 libsodium 或 bcrypt 库替换 HashUtil 的密码哈希实现。
 */

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <unordered_set>
#include <hiredis/hiredis.h>
#include <pqxx/pqxx>
#include "RedisUtil.hpp"

namespace Yachiyo {
namespace Utils {

/**
 * @brief 数据库配置结构体
 */
struct DatabaseConfig {
    std::string type;           // "mysql" 或 "postgresql"
    std::string host;
    int port;
    std::string database;
    std::string username;
    std::string password;
    int poolSize;
    int connectionTimeout;
    int maxRetries;
    int retryDelay; // 毫秒
    
    DatabaseConfig() 
        : type("postgresql"), host("localhost"), port(5432), 
          database("yachiyo"), username("postgres"), password(""),
          poolSize(10), connectionTimeout(30), maxRetries(3), retryDelay(1000) {}
};

// RedisConfig 定义在 RedisUtil.hpp 中，此处直接复用

/**
 * @brief 数据库工具类 - 支持MySQL和PostgreSQL
 */
class DatabaseUtil {
public:
    DatabaseUtil();
    ~DatabaseUtil();
    
    // 禁止拷贝
    DatabaseUtil(const DatabaseUtil&) = delete;
    DatabaseUtil& operator=(const DatabaseUtil&) = delete;
    
    /**
     * @brief 连接到数据库
     * @param config 数据库配置
     * @return 是否成功
     */
    bool connect(const DatabaseConfig& config);
    
    /**
     * @brief 连接到Redis
     * @param config Redis配置
     * @return 是否成功
     */
    bool connectRedis(const RedisConfig& config);
    
    /**
     * @brief 断开所有连接
     */
    void disconnect();
    
    /**
     * @brief 检查数据库连接状态
     * @return 是否已连接
     */
    bool isConnected() const;
    
    /**
     * @brief 检查Redis连接状态
     * @return 是否已连接
     */
    bool isRedisConnected() const;
    
    /**
     * @brief 获取数据库连接
     * @return 数据库连接
     * @throws std::runtime_error 如果未连接
     */
    pqxx::connection getConnection();
    
    /**
     * @brief 执行SQL查询（无结果）
     * @param query SQL查询语句
     * @return 是否成功
     */
    bool executeQuery(const std::string& query);
    
    /**
     * @brief 执行SQL查询并返回结果
     * @param query SQL查询语句
     * @return 查询结果
     * @throws std::runtime_error 如果查询失败
     */
    pqxx::result executeQueryWithResult(const std::string& query);
    
    /**
     * @brief 执行事务
     * @param transactionFunc 事务函数
     * @return 是否成功
     */
    bool executeTransaction(std::function<bool(pqxx::work&)> transactionFunc);

    // ==================== 参数化查询接口 ====================
    // 这些方法被 AuthServiceImpl / MessageServiceImpl 等服务层调用

    /**
     * @brief 执行参数化SELECT查询
     * @param sql SQL语句，参数占位符用 $1 $2 ...
     * @param params 参数列表
     * @return 结果行（每行为 map<字段名, 值>），空结果返回空向量
     */
    std::vector<std::map<std::string, std::string>> query(
        const std::string& sql,
        const std::vector<std::string>& params = {}
    );

    /**
     * @brief 执行参数化INSERT并返回结果 (通常配合 RETURNING)
     * @param sql SQL语句
     * @param params 参数列表
     * @return 返回的结果行
     */
    std::vector<std::map<std::string, std::string>> insert(
        const std::string& sql,
        const std::vector<std::string>& params = {}
    );

    /**
     * @brief 执行参数化UPDATE/DELETE (无返回值)
     * @param sql SQL语句
     * @param params 参数列表
     * @return 受影响的行数，失败返回 -1
     */
    int execute(
        const std::string& sql,
        const std::vector<std::string>& params = {}
    );
    
    /**
     * @brief 执行预编译语句
     * @param name 语句名称
     * @param params 参数列表
     * @return 是否成功
     */
    bool executePreparedStatement(const std::string& name, 
                                 const std::vector<std::string>& params);
    
    // Redis操作
    bool redisSet(const std::string& key, const std::string& value, int expireSeconds = 0);
    std::string redisGet(const std::string& key);
    bool redisDelete(const std::string& key);
    bool redisExists(const std::string& key);
    bool redisExpire(const std::string& key, int seconds);
    
    // 哈希表操作
    bool redisHSet(const std::string& key, const std::string& field, const std::string& value);
    std::string redisHGet(const std::string& key, const std::string& field);
    bool redisHDel(const std::string& key, const std::string& field);
    std::vector<std::string> redisHGetAll(const std::string& key);
    
    // 列表操作
    bool redisLPush(const std::string& key, const std::string& value);
    bool redisRPush(const std::string& key, const std::string& value);
    std::string redisLPop(const std::string& key);
    std::string redisRPop(const std::string& key);
    std::vector<std::string> redisLRange(const std::string& key, int start, int stop);
    
    // 集合操作
    bool redisSAdd(const std::string& key, const std::string& member);
    bool redisSRem(const std::string& key, const std::string& member);
    bool redisSIsMember(const std::string& key, const std::string& member);
    std::vector<std::string> redisSMembers(const std::string& key);
    
    // 有序集合操作
    bool redisZAdd(const std::string& key, double score, const std::string& member);
    bool redisZRem(const std::string& key, const std::string& member);
    std::vector<std::string> redisZRange(const std::string& key, int start, int stop, bool withScores = false);
    
    // 发布订阅
    bool redisPublish(const std::string& channel, const std::string& message);
    
    // 数据库管理
    bool backupDatabase(const std::string& backupPath);
    bool restoreDatabase(const std::string& backupPath);
    
    // 连接池统计
    int getActiveConnections() const;
    int getIdleConnections() const;
    int getTotalConnections() const;
    
    // Redis统计
    std::string redisInfo(const std::string& section = "");
    int64_t redisDBSize();

private:
    // 构建连接字符串
    std::string buildConnectionString() const;
    
    // 测试连接
    bool testConnection(pqxx::connection& conn);
    
    // 设置连接参数
    void setupConnection(pqxx::connection& conn);
    
    // 检查语句是否已准备
    bool isStatementPrepared(const std::string& name);
    
    // 执行Redis命令并处理回复
    template<typename... Args>
    redisReply* executeRedisCommand(const char* format, Args... args);
    
    // 释放Redis回复
    void freeRedisReply(redisReply* reply);
    
    // 成员变量
    DatabaseConfig config;
    RedisConfig redisConfig_;
    std::string connectionString_;
    mutable std::mutex mutex;
    ::redisContext* redisCtx_;
    
    // 已准备的语句集合
    std::unordered_set<std::string> preparedStatements;
};

} // namespace Utils
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::utils {
    using DatabaseUtil = Yachiyo::Utils::DatabaseUtil;
}