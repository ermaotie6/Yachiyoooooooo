#pragma once

#include "BaseController.hpp"
#include "../services/MessageService.hpp"
#include "../services/AuthService.hpp"
#include "../utils/JwtUtil.hpp"
#include <memory>

namespace yachiyo::controllers {

using Services::IMessageService, Services::IAuthService;
using Utils::JwtUtil, Utils::Result;

/**
 * @brief 消息控制器 (v2.0)
 * 
 * API端点:
 * - POST /api/messages/send - 发送消息 (需要认证)
 * - GET /api/messages/list - 获取消息列表 (需要认证)
 * - GET /api/messages/pending - 获取待审查消息 (管理员)
 * - POST /api/messages/review - 审查消息 (管理员)
 * - GET /api/messages/stats - 获取统计信息 (管理员)
 */
class MessageController : public BaseController {
private:
    std::shared_ptr<IMessageService> messageService;
    std::shared_ptr<IAuthService> authService;
    std::shared_ptr<JwtUtil> jwtUtil;
    
public:
    MessageController(
        std::shared_ptr<IMessageService> msg,
        std::shared_ptr<IAuthService> auth,
        std::shared_ptr<JwtUtil> jwt
    ) : messageService(msg), authService(auth), jwtUtil(jwt) {}
    
    // ==================== 消息端点 ====================
    
    /**
     * @brief 发送消息
     * 
     * 请求头: Authorization: Bearer <access_token>
     * 请求体:
     * {
     *   "message": "Hello world!"
     * }
     */
    void sendMessage(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取消息列表
     * 
     * 查询参数: ?limit=20&offset=0
     */
    void getMessages(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取待审查消息 (管理员)
     */
    void getPendingMessages(const crow::request& req, crow::response& res);
    
    /**
     * @brief 审查消息 (管理员)
     * 
     * 请求体:
     * {
     *   "message_id": 123,
     *   "approved": true,
     *   "reason": "审查理由"
     * }
     */
    void reviewMessage(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取统计信息 (管理员)
     */
    void getStatistics(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取高风险消息 (管理员)
     */
    void getHighRiskMessages(const crow::request& req, crow::response& res);
};

} // namespace yachiyo::controllers
