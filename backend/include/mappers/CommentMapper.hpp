#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include "models/Message.hpp"
#include "dto/ChatRequest.hpp"

namespace Yachiyo {
namespace Mappers {

/**
 * @brief 评论/消息数据访问接口
 */
class CommentMapper {
public:
    virtual ~CommentMapper() = default;

    /**
     * @brief 创建评论
     * @param comment 评论数据
     * @return 创建后的评论（包含ID）
     */
    virtual Models::Message create(const Models::Message& comment) = 0;

    /**
     * @brief 根据ID查找评论
     * @param id 评论ID
     * @return 评论对象（如果存在）
     */
    virtual std::optional<Models::Message> findById(int64_t id) = 0;

    /**
     * @brief 按帖子获取评论
     * @param postId 帖子ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 评论列表
     */
    virtual std::vector<Models::Message> findByPostId(int64_t postId, int page, int pageSize) = 0;

    /**
     * @brief 按作者获取评论
     * @param authorId 作者ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 评论列表
     */
    virtual std::vector<Models::Message> findByAuthor(int64_t authorId, int page, int pageSize) = 0;

    /**
     * @brief 更新评论
     * @param id 评论ID
     * @param comment 更新的评论数据
     * @return 是否成功
     */
    virtual bool update(int64_t id, const Models::Message& comment) = 0;

    /**
     * @brief 删除评论
     * @param id 评论ID
     * @return 是否成功
     */
    virtual bool deleteById(int64_t id) = 0;

    /**
     * @brief 获取评论数量
     * @param postId 帖子ID
     * @return 评论数量
     */
    virtual int getCommentCount(int64_t postId) = 0;

    /**
     * @brief 增加点赞数
     * @param id 评论ID
     * @return 是否成功
     */
    virtual bool incrementLikeCount(int64_t id) = 0;

    /**
     * @brief 减少点赞数
     * @param id 评论ID
     * @return 是否成功
     */
    virtual bool decrementLikeCount(int64_t id) = 0;
};

} // namespace Mappers
} // namespace Yachiyo
