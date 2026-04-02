#pragma once

#include "controllers/BaseController.hpp"
#include "services/PostService.hpp"
#include "dto/PostDTO.hpp"
#include <crow.h>
#include <memory>

namespace Yachiyo {
namespace controllers {

class PostController : public BaseController {
public:
    explicit PostController(std::shared_ptr<services::PostService> postService = nullptr);
    
    void registerRoutes(crow::SimpleApp& app) override;
    
private:
    std::shared_ptr<services::PostService> postService;
    
    // 创建帖子
    crow::response createPost(const crow::request& req);
    
    // 获取帖子列表
    crow::response getPostList(const crow::request& req);
    
    // 获取帖子详情
    crow::response getPostDetail(const crow::request& req);
    
    // 更新帖子
    crow::response updatePost(const crow::request& req);
    
    // 删除帖子
    crow::response deletePost(const crow::request& req);
    
    // 点赞帖子
    crow::response likePost(const crow::request& req);
    
    // 取消点赞
    crow::response unlikePost(const crow::request& req);
    
    // 收藏帖子
    crow::response favoritePost(const crow::request& req);
    
    // 取消收藏
    crow::response unfavoritePost(const crow::request& req);
    
    // 获取帖子评论
    crow::response getPostComments(const crow::request& req);
    
    // 添加评论
    crow::response addComment(const crow::request& req);
    
    // 删除评论
    crow::response deleteComment(const crow::request& req);
};

} // namespace controllers
} // namespace Yachiyo