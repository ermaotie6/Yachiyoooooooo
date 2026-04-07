#pragma once

#include "BaseController.hpp"
#include "../services/MessageService.hpp"
#include "../services/AuthService.hpp"
#include "../services/DatabaseService.hpp"
#include "../services/WebSocketService.hpp"
#include "../utils/JwtUtil.hpp"
#include "../utils/Logger.hpp"
#include <memory>
#include <nlohmann/json.hpp>

namespace yachiyo::controllers {

using json = nlohmann::json;

/**
 * @brief 消息控制器 (v2.0)
 * 
 * API端点:
 * - POST /api/messages/send - 发送消息 (需要认证)
 * - GET /api/messages/list - 获取消息列表 (需要认证)
 * - GET /api/messages/pending - 获取待审查消息 (管理员)
 * - POST /api/messages/review - 审查消息 (管理员)
 * - POST /api/messages/delete - 删除消息 (消息所有者或管理员)
 * - GET /api/messages/stats - 获取统计信息 (管理员)
 * - GET /api/messages/high-risk - 获取高风险消息 (管理员)
 * - GET /api/messages/context - 获取对话上下文 (需要认证)
 * - POST /api/messages/feedback - 提交内容反馈 (需要认证)
 */
class MessageController : public BaseController {
private:
    std::shared_ptr<services::IMessageService> messageService;
    std::shared_ptr<services::IAuthService> authService;
    std::shared_ptr<Yachiyo::Services::DatabaseService> databaseService;
    std::shared_ptr<Yachiyo::Services::WebSocketService> webSocketService;
    std::shared_ptr<Yachiyo::Utils::JwtUtil> jwtUtil;
    std::shared_ptr<Yachiyo::Utils::Logger> logger;
    
public:
    MessageController(
        std::shared_ptr<services::IMessageService> msg,
        std::shared_ptr<services::IAuthService> auth,
        std::shared_ptr<Yachiyo::Services::DatabaseService> db,
        std::shared_ptr<Yachiyo::Services::WebSocketService> ws,
        std::shared_ptr<Yachiyo::Utils::JwtUtil> jwt,
        std::shared_ptr<Yachiyo::Utils::Logger> log
    ) : messageService(msg), authService(auth), databaseService(db),
        webSocketService(ws), jwtUtil(jwt), logger(log) {}
    
    // ==================== 路由注册 ====================
    
    /**
     * @brief 注册所有消息路由到 Crow 应用
     */
    void registerRoutes(crow::SimpleApp& app) override;
    
    // ==================== 消息端点 ====================
    
    /**
     * @brief 发送消息
     * 
     * 请求头: Authorization: Bearer <access_token>
     * 请求体:
     * {
     *   "message": "Hello world!",
     *   "conversation_id": 123  // 可选，用于特定对话
     * }
     * 
     * 响应:
     * {
     *   "code": 200,
     *   "data": {
     *     "message_id": 456,
     *     "status": "pending",
     *     "created_at": 1234567890
     *   }
     * }
     */
    void sendMessage(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取消息列表
     * 
     * 查询参数: ?limit=20&offset=0&conversation_id=123
     * 
     * 响应:
     * {
     *   "code": 200,
     *   "data": {
     *     "messages": [...],
     *     "total": 100,
     *     "has_more": true
     *   }
     * }
     */
    void getMessages(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取待审查消息 (管理员)
     * 
     * 查询参数: ?limit=20&offset=0&severity=0.5
     */
    void getPendingMessages(const crow::request& req, crow::response& res);
    
    /**
     * @brief 审查消息 (管理员)
     * 
     * 请求体:
     * {
     *   "message_id": 123,
     *   "approved": true,
     *   "violation_type": "harassment",
     *   "severity_score": 0.85,
     *   "action_taken": "removed",
     *   "reason": "审查理由"
     * }
     */
    void reviewMessage(const crow::request& req, crow::response& res);
    
    /**
     * @brief 删除消息
     * 
     * 请求头: Authorization: Bearer <access_token>
     * 请求体:
     * {
     *   "message_id": 123,
     *   "reason": "原因"
     * }
     */
    void deleteMessage(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取统计信息 (管理员)
     * 
     * 响应:
     * {
     *   "code": 200,
     *   "data": {
     *     "total_messages": 1000,
     *     "pending_review": 50,
     *     "violations": 20,
     *     "avg_moderation_time": 30
     *   }
     * }
     */
    void getStatistics(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取高风险消息 (管理员)
     * 
     * 查询参数: ?severity_threshold=0.8&limit=50
     */
    void getHighRiskMessages(const crow::request& req, crow::response& res);
    
    /**
     * @brief 获取对话上下文
     * 
     * 查询参数: ?conversation_id=123&depth=10
     * 
     * 用于 LiveStream 获取对话历史和上下文
     */
    void getConversationContext(const crow::request& req, crow::response& res);
    
    /**
     * @brief 提交内容反馈
     * 
     * 请求头: Authorization: Bearer <access_token>
     * 请求体:
     * {
     *   "message_id": 123,
     *   "feedback_type": "inappropriate|helpful|harmful",
     *   "reason": "反馈原因"
     * }
     */
    void submitFeedback(const crow::request& req, crow::response& res);
    
    // ==================== 辅助方法 ====================
    
    /**
     * @brief 验证管理员权限
     */
    bool validateAdmin(const crow::request& req, crow::response& res);
    
    /**
     * @brief 验证用户权限
     */
    bool validateUser(const crow::request& req, crow::response& res, int64_t& user_id);
    
    /**
     * @brief 广播消息到所有连接的客户端
     */
    void broadcastMessage(const json& message);
    
    /**
     * @brief 处理消息审查结果并更新相关状态
     */
    void processReviewResult(int64_t message_id, const json& review_data);
};

} // namespace yachiyo::controllers
