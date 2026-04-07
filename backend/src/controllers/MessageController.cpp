#include "controllers/MessageController.hpp"
#include "utils/Logger.hpp"
#include "services/DatabaseService.hpp"
#include "services/WebSocketService.hpp"
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

namespace yachiyo::controllers {

using namespace Yachiyo::Services;
using yachiyo::models::UserRole;

// ==================== 辅助函数 ====================

// 从Authorization头提取用户ID，失败返回0
static int64_t extractUserId(const crow::request& req, const std::shared_ptr<Utils::JwtUtil>& jwtUtil) {
    std::string authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        return 0;
    }
    std::string token = authHeader.substr(7);
    // 简化处理，实际应使用authService提取
    return 0; // 占位符
}

// ==================== 发送消息 ====================
void MessageController::sendMessage(const crow::request& req, crow::response& res) {
    try {
        // 验证用户认证
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
        std::string token = authHeader.substr(7);
        int64_t userId = authService->getUserIdFromToken(token);
        
        if (userId <= 0) {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "令牌无效或已过期"}
            }).dump();
            return;
        }
        
        // 解析请求体
        auto body = json::parse(req.body);
        std::string message = body.value("message", "");
        
        if (message.empty()) {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "消息内容不能为空"}
            }).dump();
            return;
        }
        
        // 获取用户IP和User-Agent
        std::string userIp = req.remote_ip_address;
        std::string userAgent = req.get_header_value("User-Agent");
        
        // 调用消息服务 (执行6层审查)
        auto result = messageService->sendMessage(userId, message, userIp, userAgent);
        
        if (result.isSuccess()) {
            auto msg = result.value();
            res.code = 201;
            res.body = json({
                {"code", 201},
                {"msg", "消息已提交审查"},
                {"data", {
                    {"message_id", msg->getMessageId()},
                    {"user_id", msg->getUserId()},
                    {"review_status", static_cast<int>(msg->getReviewStatus())},
                    {"spam_score", msg->getSpamScore()},
                    {"created_at", msg->getCreatedAt()}
                }}
            }).dump();
            LOG_INFO("消息发送成功: " + std::to_string(msg->getMessageId()));
        } else {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("发送消息异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 获取消息列表 ====================
void MessageController::getMessages(const crow::request& req, crow::response& res) {
    try {
        // 验证用户认证
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
        std::string token = authHeader.substr(7);
        int64_t userId = authService->getUserIdFromToken(token);
        
        if (userId <= 0) {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "令牌无效或已过期"}
            }).dump();
            return;
        }
        
        // 解析查询参数
        int32_t limit = 20;
        int32_t offset = 0;
        
        auto limit_param = req.url_params.get("limit");
        if (limit_param) {
            try {
                limit = std::stoi(limit_param);
                if (limit > 100) limit = 100; // 最多返回100条
            } catch (...) {}
        }
        
        auto offset_param = req.url_params.get("offset");
        if (offset_param) {
            try {
                offset = std::stoi(offset_param);
            } catch (...) {}
        }
        
        // 获取消息列表
        auto result = messageService->getUserMessages(userId, limit, offset);
        
        if (result.isSuccess()) {
            const auto& messages = result.value();
            json data = json::array();
            
            for (const auto& msg : messages) {
                data.push_back({
                    {"message_id", msg->getMessageId()},
                    {"user_id", msg->getUserId()},
                    {"message", msg->getOriginalMessage()},
                    {"review_status", static_cast<int>(msg->getReviewStatus())},
                    {"created_at", msg->getCreatedAt()}
                });
            }
            
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "获取成功"},
                {"data", {
                    {"messages", data},
                    {"total", static_cast<int>(messages.size())}
                }}
            }).dump();
        } else {
            res.code = 500;
            res.body = json({
                {"code", 500},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("获取消息列表异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 获取待审查消息 (管理员) ====================
void MessageController::getPendingMessages(const crow::request& req, crow::response& res) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
        std::string token = authHeader.substr(7);
        auto role = authService->getRoleFromToken(token);
        
        if (role != UserRole::ADMIN) {
            res.code = 403;
            res.body = json({
                {"code", 403},
                {"msg", "权限不足，仅管理员可操作"}
            }).dump();
            return;
        }
        
        // 解析查询参数
        int32_t limit = 50;
        int32_t offset = 0;
        
        auto limit_param = req.url_params.get("limit");
        if (limit_param) {
            try {
                limit = std::stoi(limit_param);
            } catch (...) {}
        }
        
        auto offset_param = req.url_params.get("offset");
        if (offset_param) {
            try {
                offset = std::stoi(offset_param);
            } catch (...) {}
        }
        
        // 获取待审查消息
        auto result = messageService->getPendingMessages(limit, offset);
        
        if (result.isSuccess()) {
            const auto& messages = result.value();
            json data = json::array();
            
            for (const auto& msg : messages) {
                data.push_back({
                    {"message_id", msg->getMessageId()},
                    {"user_id", msg->getUserId()},
                    {"message", msg->getOriginalMessage()},
                    {"spam_score", msg->getSpamScore()},
                    {"is_spam", msg->getIsSpam()},
                    {"is_abusive", msg->getIsAbusive()},
                    {"created_at", msg->getCreatedAt()}
                });
            }
            
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "获取成功"},
                {"data", data}
            }).dump();
        } else {
            res.code = 500;
            res.body = json({
                {"code", 500},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("获取待审查消息异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 审查消息 (管理员) ====================
void MessageController::reviewMessage(const crow::request& req, crow::response& res) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
        std::string token = authHeader.substr(7);
        int64_t reviewerId = authService->getUserIdFromToken(token);
        auto role = authService->getRoleFromToken(token);
        
        if (role != UserRole::ADMIN) {
            res.code = 403;
            res.body = json({
                {"code", 403},
                {"msg", "权限不足，仅管理员可操作"}
            }).dump();
            return;
        }
        
        // 解析请求体
        auto body = json::parse(req.body);
        int64_t messageId = body.value("message_id", 0);
        bool approved = body.value("approved", false);
        std::string reason = body.value("reason", "");
        
        if (messageId <= 0) {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "缺少 message_id"}
            }).dump();
            return;
        }
        
        // 审查消息
        auto result = messageService->reviewMessage(messageId, reviewerId, approved, reason);
        
        if (result.isSuccess()) {
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "审查完成"},
                {"data", {
                    {"message_id", messageId},
                    {"approved", approved}
                }}
            }).dump();
            LOG_INFO("消息审查完成: " + std::to_string(messageId));
        } else {
            res.code = 500;
            res.body = json({
                {"code", 500},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("审查消息异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 获取统计信息 (管理员) ====================
void MessageController::getStatistics(const crow::request& req, crow::response& res) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
        std::string token = authHeader.substr(7);
        auto role = authService->getRoleFromToken(token);
        
        if (role != UserRole::ADMIN) {
            res.code = 403;
            res.body = json({
                {"code", 403},
                {"msg", "权限不足，仅管理员可操作"}
            }).dump();
            return;
        }
        
        // 获取统计信息
        auto result = messageService->getStatistics();
        
        if (result.isSuccess()) {
            const auto& stats = result.value();
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "获取成功"},
                {"data", stats}
            }).dump();
        } else {
            res.code = 500;
            res.body = json({
                {"code", 500},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("获取统计信息异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 获取高风险消息 (管理员) ====================
void MessageController::getHighRiskMessages(const crow::request& req, crow::response& res) {
    try {
        // 验证管理员权限
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = json({
                {"code", 401},
                {"msg", "缺少认证令牌"}
            }).dump();
            return;
        }
        
        std::string token = authHeader.substr(7);
        auto role = authService->getRoleFromToken(token);
        
        if (role != UserRole::ADMIN) {
            res.code = 403;
            res.body = json({
                {"code", 403},
                {"msg", "权限不足，仅管理员可操作"}
            }).dump();
            return;
        }
        
        // 解析查询参数
        int32_t limit = 50;
        int32_t offset = 0;
        
        auto limit_param = req.url_params.get("limit");
        if (limit_param) {
            try {
                limit = std::stoi(limit_param);
            } catch (...) {}
        }
        
        auto offset_param = req.url_params.get("offset");
        if (offset_param) {
            try {
                offset = std::stoi(offset_param);
            } catch (...) {}
        }
        
        // 获取高风险消息
        auto result = messageService->getHighRiskMessages(limit, offset);
        
        if (result.isSuccess()) {
            const auto& messages = result.value();
            json data = json::array();
            
            for (const auto& msg : messages) {
                data.push_back({
                    {"message_id", msg->getMessageId()},
                    {"user_id", msg->getUserId()},
                    {"message", msg->getOriginalMessage()},
                    {"spam_score", msg->getSpamScore()},
                    {"is_spam", msg->getIsSpam()},
                    {"is_abusive", msg->getIsAbusive()},
                    {"created_at", msg->getCreatedAt()}
                });
            }
            
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "获取成功"},
                {"data", data}
            }).dump();
        } else {
            res.code = 500;
            res.body = json({
                {"code", 500},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("获取高风险消息异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

} // namespace yachiyo::controllers
