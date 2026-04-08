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

// ==================== 路由注册 ====================
void MessageController::registerRoutes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/v1/messages/send")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->sendMessage(req, res);
            return res;
        });

    CROW_ROUTE(app, "/api/v1/messages/list")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->getMessages(req, res);
            return res;
        });

    CROW_ROUTE(app, "/api/v1/messages/pending")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->getPendingMessages(req, res);
            return res;
        });

    CROW_ROUTE(app, "/api/v1/messages/review")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->reviewMessage(req, res);
            return res;
        });

    CROW_ROUTE(app, "/api/v1/messages/delete")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->deleteMessage(req, res);
            return res;
        });

    CROW_ROUTE(app, "/api/v1/messages/stats")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->getStatistics(req, res);
            return res;
        });

    CROW_ROUTE(app, "/api/v1/messages/high-risk")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->getHighRiskMessages(req, res);
            return res;
        });

    CROW_ROUTE(app, "/api/v1/messages/context")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->getConversationContext(req, res);
            return res;
        });

    CROW_ROUTE(app, "/api/v1/messages/feedback")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            crow::response res;
            this->submitFeedback(req, res);
            return res;
        });

    logger->info("消息控制器路由已注册");
}

// ==================== 辅助函数 ====================

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

// ==================== 删除消息 ====================
void MessageController::deleteMessage(const crow::request& req, crow::response& res) {
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
        auto role = authService->getRoleFromToken(token);
        
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
        int64_t messageId = body.value("message_id", 0);
        std::string reason = body.value("reason", "");
        
        if (messageId <= 0) {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "缺少 message_id"}
            }).dump();
            return;
        }
        
        // 调用消息服务删除（消息服务内部会检查权限：消息所有者或管理员）
        auto result = messageService->deleteMessage(messageId, userId, role == UserRole::ADMIN, reason);
        
        if (result.isSuccess()) {
            res.code = 200;
            res.body = json({
                {"code", 200},
                {"msg", "消息已删除"},
                {"data", {
                    {"message_id", messageId}
                }}
            }).dump();
            LOG_INFO("消息删除: " + std::to_string(messageId) + " by user " + std::to_string(userId));
        } else {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", result.getErrorMsg()}
            }).dump();
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("删除消息异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 获取对话上下文 ====================
void MessageController::getConversationContext(const crow::request& req, crow::response& res) {
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
        int32_t depth = 10;
        auto depth_param = req.url_params.get("depth");
        if (depth_param) {
            try {
                depth = std::stoi(depth_param);
                if (depth > 50) depth = 50;
            } catch (...) {}
        }
        
        // 获取用户最近的对话上下文
        auto result = messageService->getUserMessages(userId, depth, 0);
        
        if (result.isSuccess()) {
            const auto& messages = result.value();
            json context = json::array();
            
            for (const auto& msg : messages) {
                context.push_back({
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
                    {"context", context},
                    {"depth", depth},
                    {"count", static_cast<int>(messages.size())}
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
        LOG_ERROR("获取对话上下文异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

// ==================== 提交内容反馈 ====================
void MessageController::submitFeedback(const crow::request& req, crow::response& res) {
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
        int64_t messageId = body.value("message_id", 0);
        std::string feedbackType = body.value("feedback_type", "");
        std::string reason = body.value("reason", "");
        
        if (messageId <= 0) {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "缺少 message_id"}
            }).dump();
            return;
        }
        
        if (feedbackType.empty()) {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "缺少 feedback_type (inappropriate|helpful|harmful)"}
            }).dump();
            return;
        }
        
        // 验证 feedback_type 的合法性
        if (feedbackType != "inappropriate" && feedbackType != "helpful" && feedbackType != "harmful") {
            res.code = 400;
            res.body = json({
                {"code", 400},
                {"msg", "无效的 feedback_type，可选值: inappropriate, helpful, harmful"}
            }).dump();
            return;
        }
        
        // 将反馈持久化到 review_logs 表
        try {
            auto& dbService = Yachiyo::Services::DatabaseService::getInstance();
            if (dbService.isInitialized()) {
                auto dbConn = Yachiyo::Services::DatabasePool::getInstance().getConnection();
                pqxx::work txn(*dbConn);
                txn.exec_params(
                    R"(INSERT INTO review_logs (message_id, review_type, reviewed_by, action_taken, reason, created_at)
                       VALUES ($1, 2, $2, 1, $3, NOW()))",
                    messageId, userId,
                    feedbackType + (reason.empty() ? "" : ": " + reason)
                );
                txn.commit();
            }
        } catch (const std::exception& dbErr) {
            LOG_ERROR("反馈存储失败: " + std::string(dbErr.what()));
        }
        
        LOG_INFO("用户反馈: user={}, message={}, type={}, reason={}", 
                 userId, messageId, feedbackType, reason);
        
        res.code = 200;
        res.body = json({
            {"code", 200},
            {"msg", "反馈已提交"},
            {"data", {
                {"message_id", messageId},
                {"feedback_type", feedbackType}
            }}
        }).dump();
        
    } catch (const std::exception& e) {
        LOG_ERROR("提交反馈异常: " + std::string(e.what()));
        res.code = 500;
        res.body = json({
            {"code", 500},
            {"msg", "服务器错误"}
        }).dump();
    }
}

} // namespace yachiyo::controllers
