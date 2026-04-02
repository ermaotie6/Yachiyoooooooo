#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include "models/User.hpp"

namespace Yachiyo {
namespace Mappers {

// 数据库连接接口
class DatabaseConnection {
public:
    virtual ~DatabaseConnection() = default;
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool execute(const std::string& sql) = 0;
    virtual std::vector<std::vector<std::string>> query(const std::string& sql) = 0;
};

/**
 * @brief 用户数据访问接口
 */
class UserMapper {
public:
    virtual ~UserMapper() = default;

    /**
     * @brief 根据ID查找用户
     * @param id 用户ID
     * @return 用户对象（如果存在）
     */
    virtual std::optional<Models::User> findById(int64_t id) = 0;

    /**
     * @brief 根据邮箱查找用户
     * @param email 邮箱地址
     * @return 用户对象（如果存在）
     */
    virtual std::optional<Models::User> findByEmail(const std::string& email) = 0;

    /**
     * @brief 根据用户名查找用户
     * @param username 用户名
     * @return 用户对象（如果存在）
     */
    virtual std::optional<Models::User> findByUsername(const std::string& username) = 0;

    /**
     * @brief 插入新用户
     * @param user 用户对象
     * @return 插入的用户ID（失败返回0）
     */
    virtual int64_t insert(const Models::User& user) = 0;

    /**
     * @brief 更新用户信息
     * @param user 用户对象
     * @return 是否更新成功
     */
    virtual bool update(const Models::User& user) = 0;

    /**
     * @brief 删除用户
     * @param id 用户ID
     * @return 是否删除成功
     */
    virtual bool deleteById(int64_t id) = 0;

    /**
     * @brief 获取所有用户
     * @param limit 限制数量
     * @param offset 偏移量
     * @return 用户列表
     */
    virtual std::vector<Models::User> findAll(int limit = 100, int offset = 0) = 0;

    /**
     * @brief 统计用户数量
     * @return 用户总数
     */
    virtual int64_t count() = 0;

    /**
     * @brief 检查邮箱是否已存在
     * @param email 邮箱地址
     * @return 是否存在
     */
    virtual bool existsByEmail(const std::string& email) = 0;

    /**
     * @brief 检查用户名是否已存在
     * @param username 用户名
     * @return 是否存在
     */
    virtual bool existsByUsername(const std::string& username) = 0;
};

/**
 * @brief MySQL用户数据访问实现
 */
class MySQLUserMapper : public UserMapper {
public:
    MySQLUserMapper(std::shared_ptr<DatabaseConnection> connection);
    ~MySQLUserMapper();

    // 实现接口方法
    std::optional<Models::User> findById(int64_t id) override;
    std::optional<Models::User> findByEmail(const std::string& email) override;
    std::optional<Models::User> findByUsername(const std::string& username) override;
    int64_t insert(const Models::User& user) override;
    bool update(const Models::User& user) override;
    bool deleteById(int64_t id) override;
    std::vector<Models::User> findAll(int limit = 100, int offset = 0) override;
    int64_t count() override;
    bool existsByEmail(const std::string& email) override;
    bool existsByUsername(const std::string& username) override;

private:
    /**
     * @brief 从数据库行创建用户对象
     * @param row 数据库行数据
     * @return 用户对象
     */
    Models::User createUserFromRow(const std::vector<std::string>& row);

    /**
     * @brief 执行查询并返回单个结果
     * @param sql SQL语句
     * @return 查询结果
     */
    std::optional<std::vector<std::string>> querySingle(const std::string& sql);

    /**
     * @brief 执行更新操作
     * @param sql SQL语句
     * @return 是否成功
     */
    bool executeUpdate(const std::string& sql);

    // 数据库连接
    std::shared_ptr<DatabaseConnection> connection;
    
    // 表名
    static constexpr const char* TABLE_NAME = "users";
};

} // namespace Mappers
} // namespace Yachiyo