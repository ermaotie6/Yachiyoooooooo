#pragma once

#include <string>
#include <memory>
#include <pqxx/pqxx>
#include "mappers/PostgreSQLPostMapper.hpp"
#include "mappers/PostgreSQLCommentMapper.hpp"

namespace Yachiyo {
namespace Database {

/**
 * @brief 数据库连接和初始化管理器
 */
class DatabaseManager {
private:
    std::string connectionString;
    std::shared_ptr<pqxx::connection> mainConnection;
    
public:
    DatabaseManager() = default;
    
    /**
     * @brief 连接到数据库
     */
    bool connect(const std::string& host, int port, const std::string& database,
                 const std::string& username, const std::string& password) {
        try {
            std::string connStr = "host=" + host + 
                                 " port=" + std::to_string(port) + 
                                 " dbname=" + database + 
                                 " user=" + username + 
                                 " password=" + password;
            
            mainConnection = std::make_shared<pqxx::connection>(connStr);
            
            if (!mainConnection->is_open()) {
                return false;
            }
            
            connectionString = connStr;
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    /**
     * @brief 获取主连接
     */
    std::shared_ptr<pqxx::connection> getConnection() const {
        return mainConnection;
    }
    
    /**
     * @brief 获取新连接
     */
    std::shared_ptr<pqxx::connection> createConnection() {
        try {
            return std::make_shared<pqxx::connection>(connectionString);
        } catch (const std::exception& e) {
            return nullptr;
        }
    }
    
    /**
     * @brief 创建 PostMapper
     */
    std::shared_ptr<Mappers::PostMapper> createPostMapper() {
        if (!mainConnection) {
            return nullptr;
        }
        return std::make_shared<Mappers::PostgreSQLPostMapper>(mainConnection);
    }
    
    /**
     * @brief 创建 CommentMapper
     */
    std::shared_ptr<Mappers::CommentMapper> createCommentMapper() {
        if (!mainConnection) {
            return nullptr;
        }
        return std::make_shared<Mappers::PostgreSQLCommentMapper>(mainConnection);
    }
    
    /**
     * @brief 断开连接
     */
    void disconnect() {
        if (mainConnection && mainConnection->is_open()) {
            mainConnection->disconnect();
        }
    }
    
    /**
     * @brief 测试连接
     */
    bool testConnection() {
        try {
            if (!mainConnection || !mainConnection->is_open()) {
                return false;
            }
            
            pqxx::work txn(*mainConnection);
            auto result = txn.exec("SELECT 1");
            txn.commit();
            
            return !result.empty();
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    /**
     * @brief 执行 SQL 脚本初始化数据库
     */
    bool initializeDatabase(const std::string& sqlScript) {
        try {
            pqxx::work txn(*mainConnection);
            txn.exec(sqlScript);
            txn.commit();
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    /**
     * @brief 检查表是否存在
     */
    bool tableExists(const std::string& tableName) {
        try {
            pqxx::work txn(*mainConnection);
            std::string sql = "SELECT EXISTS(SELECT 1 FROM information_schema.tables WHERE table_name = $1)";
            auto result = txn.exec_params(sql, tableName);
            return result[0][0].as<bool>();
        } catch (const std::exception& e) {
            return false;
        }
    }
};

} // namespace Database
} // namespace Yachiyo
