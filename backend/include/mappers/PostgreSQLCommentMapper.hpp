#pragma once

#include "CommentMapper.hpp"
#include <pqxx/pqxx>
#include <memory>
#include <stdexcept>

namespace Yachiyo {
namespace Mappers {

/**
 * @brief PostgreSQL CommentMapper实现
 */
class PostgreSQLCommentMapper : public CommentMapper {
private:
    std::shared_ptr<pqxx::connection> conn;

    /**
     * @brief 将数据库结果行转换为Message模型
     */
    Models::Message rowToComment(const pqxx::row& row) {
        Models::Message comment;
        comment.id = row["id"].as<int64_t>();
        comment.content = row["content"].as<std::string>();
        comment.userId = row["user_id"].as<int64_t>();
        comment.username = row["username"].as<std::string>();
        comment.likeCount = row["like_count"].as<int>();
        // 时间戳解析省略
        return comment;
    }

public:
    explicit PostgreSQLCommentMapper(std::shared_ptr<pqxx::connection> conn)
        : conn(conn) {
        if (!conn || !conn->is_open()) {
            throw std::runtime_error("数据库连接无效");
        }
    }

    Models::Message create(const Models::Message& comment) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = R"(
                INSERT INTO comments (post_id, content, user_id, username, like_count, created_at)
                VALUES ($1, $2, $3, $4, $5, NOW())
                RETURNING id, content, user_id, username, like_count, created_at
            )";

            auto result = txn.exec_params(sql,
                comment.postId,
                comment.content,
                comment.userId,
                comment.username,
                0
            );

            if (result.empty()) {
                throw std::runtime_error("创建评论失败");
            }

            txn.commit();

            Models::Message savedComment = rowToComment(result[0]);
            savedComment.id = result[0]["id"].as<int64_t>();
            
            return savedComment;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("创建评论异常: ") + e.what());
        }
    }

    std::optional<Models::Message> findById(int64_t id) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = R"(
                SELECT id, content, user_id, username, like_count, created_at
                FROM comments WHERE id = $1
            )";

            auto result = txn.exec_params(sql, id);

            if (result.empty()) {
                return std::nullopt;
            }

            return rowToComment(result[0]);

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("查询评论异常: ") + e.what());
        }
    }

    std::vector<Models::Message> findByPostId(int64_t postId, int page, int pageSize) override {
        try {
            pqxx::work txn(*conn);
            
            int offset = (page - 1) * pageSize;
            std::string sql = R"(
                SELECT id, content, user_id, username, like_count, created_at
                FROM comments
                WHERE post_id = $1
                ORDER BY created_at DESC
                LIMIT $2 OFFSET $3
            )";

            auto result = txn.exec_params(sql, postId, pageSize, offset);

            std::vector<Models::Message> comments;
            for (auto row : result) {
                comments.push_back(rowToComment(row));
            }

            return comments;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("查询帖子评论异常: ") + e.what());
        }
    }

    std::vector<Models::Message> findByAuthor(int64_t authorId, int page, int pageSize) override {
        try {
            pqxx::work txn(*conn);
            
            int offset = (page - 1) * pageSize;
            std::string sql = R"(
                SELECT id, content, user_id, username, like_count, created_at
                FROM comments
                WHERE user_id = $1
                ORDER BY created_at DESC
                LIMIT $2 OFFSET $3
            )";

            auto result = txn.exec_params(sql, authorId, pageSize, offset);

            std::vector<Models::Message> comments;
            for (auto row : result) {
                comments.push_back(rowToComment(row));
            }

            return comments;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("查询用户评论异常: ") + e.what());
        }
    }

    bool update(int64_t id, const Models::Message& comment) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = R"(
                UPDATE comments 
                SET content = $1, like_count = $2, updated_at = NOW()
                WHERE id = $3
            )";

            txn.exec_params(sql,
                comment.content,
                comment.likeCount,
                id
            );

            txn.commit();
            return true;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("更新评论异常: ") + e.what());
        }
    }

    bool deleteById(int64_t id) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = "DELETE FROM comments WHERE id = $1";
            txn.exec_params(sql, id);
            txn.commit();

            return true;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("删除评论异常: ") + e.what());
        }
    }

    int getCommentCount(int64_t postId) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = "SELECT COUNT(*) FROM comments WHERE post_id = $1";
            auto result = txn.exec_params(sql, postId);

            return result[0][0].as<int>();

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("获取评论数异常: ") + e.what());
        }
    }

    bool incrementLikeCount(int64_t id) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = "UPDATE comments SET like_count = like_count + 1 WHERE id = $1";
            txn.exec_params(sql, id);
            txn.commit();

            return true;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("增加评论点赞异常: ") + e.what());
        }
    }

    bool decrementLikeCount(int64_t id) override {
        try {
            pqxx::work txn(*conn);
            
            std::string sql = "UPDATE comments SET like_count = GREATEST(0, like_count - 1) WHERE id = $1";
            txn.exec_params(sql, id);
            txn.commit();

            return true;

        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("减少评论点赞异常: ") + e.what());
        }
    }
};

} // namespace Mappers
} // namespace Yachiyo
