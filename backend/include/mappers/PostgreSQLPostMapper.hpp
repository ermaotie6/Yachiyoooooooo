#pragma once

#include "PostMapper.hpp"
#include <pqxx/pqxx>
#include <memory>
#include <stdexcept>

namespace Yachiyo {
namespace Mappers {

/**
 * @brief PostgreSQL PostMapper实现
 */
class PostgreSQLPostMapper : public PostMapper {
private:
    std::shared_ptr<pqxx::connection> conn;

    /**
     * @brief 将数据库结果行转换为Post模型
     */
    Models::Post rowToPost(const pqxx::row& row) {
        Models::Post post;
        post.id = row["id"].as<int64_t>();
        post.title = row["title"].as<std::string>();
        post.content = row["content"].as<std::string>();
        post.category = row["category"].as<std::string>();
        post.authorId = row["author_id"].as<int64_t>();
        post.authorName = row["author_name"].as<std::string>();
        post.likeCount = row["like_count"].as<int>();
        post.commentCount = row["comment_count"].as<int>();
        post.viewCount = row["view_count"].as<int>();
        post.status = row["status"].as<std::string>();
        
        // 处理时间戳
        std::string createdAtStr = row["created_at"].as<std::string>();
        std::string updatedAtStr = row["updated_at"].as<std::string>();
        // 简化处理，实际需要解析ISO 8601格式
        
        return post;
    }

public:
    explicit PostgreSQLPostMapper(std::shared_ptr<pqxx::connection> conn)
        : conn(conn) {
        if (!conn || !conn->is_open()) {
            throw std::runtime_error("数据库连接无效");
        }
    }

    Models::Post create(const dto::PostDTO& post) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = R"(
                INSERT INTO posts (title, content, category, author_id, author_name, 
                                   like_count, comment_count, view_count, status, created_at, updated_at)
                VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, NOW(), NOW())
                RETURNING id, title, content, category, author_id, author_name, 
                         like_count, comment_count, view_count, status, created_at, updated_at
            )";

            auto result = txn.exec_params(sql,
                post.title,
                post.content,
                post.category,
                post.authorId,
                post.authorName,
                0,  // like_count
                0,  // comment_count
                0   // view_count
            );

            if (result.empty()) {
                throw std::runtime_error("创建帖子失败");
            }

            txn.commit();

            Models::Post savedPost;
            savedPost.id = result[0]["id"].as<int64_t>();
            savedPost.title = result[0]["title"].as<std::string>();
            savedPost.content = result[0]["content"].as<std::string>();
            savedPost.category = result[0]["category"].as<std::string>();
            savedPost.authorId = result[0]["author_id"].as<int64_t>();
            savedPost.authorName = result[0]["author_name"].as<std::string>();
            
            return savedPost;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("创建帖子异常: ") + e.what());
        }
    }

    std::optional<Models::Post> findById(int64_t id) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = R"(
                SELECT id, title, content, category, author_id, author_name,
                       like_count, comment_count, view_count, status, created_at, updated_at
                FROM posts WHERE id = $1
            )";

            auto result = txn.exec_params(sql, id);

            if (result.empty()) {
                return std::nullopt;
            }

            return rowToPost(result[0]);

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("查询帖子异常: ") + e.what());
        }
    }

    std::vector<Models::Post> findAll(int page, int pageSize) override {
        try {
            pqxx::work txn(*conn);
            
            int offset = (page - 1) * pageSize;
            std::string sql = R"(
                SELECT id, title, content, category, author_id, author_name,
                       like_count, comment_count, view_count, status, created_at, updated_at
                FROM posts
                ORDER BY created_at DESC
                LIMIT $1 OFFSET $2
            )";

            auto result = txn.exec_params(sql, pageSize, offset);

            std::vector<Models::Post> posts;
            for (auto row : result) {
                posts.push_back(rowToPost(row));
            }

            return posts;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("查询帖子列表异常: ") + e.what());
        }
    }

    std::vector<Models::Post> findByCategory(const std::string& category, int page, int pageSize) override {
        try {
            pqxx::work txn(*conn);
            
            int offset = (page - 1) * pageSize;
            std::string sql = R"(
                SELECT id, title, content, category, author_id, author_name,
                       like_count, comment_count, view_count, status, created_at, updated_at
                FROM posts
                WHERE category = $1
                ORDER BY created_at DESC
                LIMIT $2 OFFSET $3
            )";

            auto result = txn.exec_params(sql, category, pageSize, offset);

            std::vector<Models::Post> posts;
            for (auto row : result) {
                posts.push_back(rowToPost(row));
            }

            return posts;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("按类别查询帖子异常: ") + e.what());
        }
    }

    std::vector<Models::Post> findByAuthor(int64_t authorId, int page, int pageSize) override {
        try {
            pqxx::work txn(*conn);
            
            int offset = (page - 1) * pageSize;
            std::string sql = R"(
                SELECT id, title, content, category, author_id, author_name,
                       like_count, comment_count, view_count, status, created_at, updated_at
                FROM posts
                WHERE author_id = $1
                ORDER BY created_at DESC
                LIMIT $2 OFFSET $3
            )";

            auto result = txn.exec_params(sql, authorId, pageSize, offset);

            std::vector<Models::Post> posts;
            for (auto row : result) {
                posts.push_back(rowToPost(row));
            }

            return posts;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("按作者查询帖子异常: ") + e.what());
        }
    }

    bool update(int64_t id, const Models::Post& post) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = R"(
                UPDATE posts 
                SET title = $1, content = $2, category = $3, 
                    like_count = $4, comment_count = $5, view_count = $6,
                    status = $7, updated_at = NOW()
                WHERE id = $8
            )";

            txn.exec_params(sql,
                post.title,
                post.content,
                post.category,
                post.likeCount,
                post.commentCount,
                post.viewCount,
                post.status,
                id
            );

            txn.commit();
            return true;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("更新帖子异常: ") + e.what());
        }
    }

    bool deleteById(int64_t id) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = "DELETE FROM posts WHERE id = $1";
            txn.exec_params(sql, id);
            txn.commit();

            return true;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("删除帖子异常: ") + e.what());
        }
    }

    bool incrementViewCount(int64_t id) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = "UPDATE posts SET view_count = view_count + 1 WHERE id = $1";
            txn.exec_params(sql, id);
            txn.commit();

            return true;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("增加浏览数异常: ") + e.what());
        }
    }

    std::vector<Models::Post> getHotPosts(int limit) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = R"(
                SELECT id, title, content, category, author_id, author_name,
                       like_count, comment_count, view_count, status, created_at, updated_at
                FROM posts
                ORDER BY (like_count + view_count * 0.1) DESC
                LIMIT $1
            )";

            auto result = txn.exec_params(sql, limit);

            std::vector<Models::Post> posts;
            for (auto row : result) {
                posts.push_back(rowToPost(row));
            }

            return posts;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("获取热门帖子异常: ") + e.what());
        }
    }

    std::vector<Models::Post> search(const std::string& keyword, int page, int pageSize) override {
        try {
            pqxx::work txn(*conn);
            
            int offset = (page - 1) * pageSize;
            std::string sql = R"(
                SELECT id, title, content, category, author_id, author_name,
                       like_count, comment_count, view_count, status, created_at, updated_at
                FROM posts
                WHERE title ILIKE $1 OR content ILIKE $1
                ORDER BY created_at DESC
                LIMIT $2 OFFSET $3
            )";

            std::string searchTerm = "%" + keyword + "%";
            auto result = txn.exec_params(sql, searchTerm, pageSize, offset);

            std::vector<Models::Post> posts;
            for (auto row : result) {
                posts.push_back(rowToPost(row));
            }

            return posts;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("搜索帖子异常: ") + e.what());
        }
    }
};

} // namespace Mappers
} // namespace Yachiyo
