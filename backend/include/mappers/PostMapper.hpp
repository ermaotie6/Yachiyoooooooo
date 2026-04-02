#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include "models/Post.hpp"
#include "dto/PostDTO.hpp"

namespace Yachiyo {
namespace Mappers {

/**
 * @brief 帖子数据访问接口
 */
class PostMapper {
public:
    virtual ~PostMapper() = default;

    /**
     * @brief 创建帖子
     * @param post 帖子DTO
     * @return 创建后的帖子（包含ID）
     */
    virtual Models::Post create(const dto::PostDTO& post) = 0;

    /**
     * @brief 根据ID查找帖子
     * @param id 帖子ID
     * @return 帖子对象（如果存在）
     */
    virtual std::optional<Models::Post> findById(int64_t id) = 0;

    /**
     * @brief 获取所有帖子（支持分页）
     * @param page 页码（从1开始）
     * @param pageSize 每页数量
     * @return 帖子列表
     */
    virtual std::vector<Models::Post> findAll(int page, int pageSize) = 0;

    /**
     * @brief 按类别获取帖子
     * @param category 类别
     * @param page 页码
     * @param pageSize 每页数量
     * @return 帖子列表
     */
    virtual std::vector<Models::Post> findByCategory(const std::string& category, int page, int pageSize) = 0;

    /**
     * @brief 按作者获取帖子
     * @param authorId 作者ID
     * @param page 页码
     * @param pageSize 每页数量
     * @return 帖子列表
     */
    virtual std::vector<Models::Post> findByAuthor(int64_t authorId, int page, int pageSize) = 0;

    /**
     * @brief 更新帖子
     * @param id 帖子ID
     * @param post 更新的帖子数据
     * @return 是否成功
     */
    virtual bool update(int64_t id, const Models::Post& post) = 0;

    /**
     * @brief 删除帖子
     * @param id 帖子ID
     * @return 是否成功
     */
    virtual bool deleteById(int64_t id) = 0;

    /**
     * @brief 增加浏览数
     * @param id 帖子ID
     * @return 是否成功
     */
    virtual bool incrementViewCount(int64_t id) = 0;

    /**
     * @brief 获取热门帖子
     * @param limit 限制数量
     * @return 热门帖子列表
     */
    virtual std::vector<Models::Post> getHotPosts(int limit) = 0;

    /**
     * @brief 搜索帖子
     * @param keyword 搜索关键字
     * @param page 页码
     * @param pageSize 每页数量
     * @return 帖子列表
     */
    virtual std::vector<Models::Post> search(const std::string& keyword, int page, int pageSize) = 0;
};

} // namespace Mappers
} // namespace Yachiyo
