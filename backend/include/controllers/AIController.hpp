#pragma once

#include "controllers/BaseController.hpp"
#include "services/ChatService.hpp"
#include "dto/AIChatRequest.hpp"
#include "dto/AIChatResponse.hpp"
#include <crow.h>
#include <memory>

namespace Yachiyo {
namespace controllers {

class AIController : public BaseController {
public:
    explicit AIController(std::shared_ptr<services::ChatService> chatService = nullptr);
    
    void registerRoutes(crow::SimpleApp& app) override;
    
private:
    std::shared_ptr<services::ChatService> chatService;
    
    // AI聊天端点
    crow::response chat(const crow::request& req);
    
    // 获取聊天历史
    crow::response getChatHistory(const crow::request& req);
    
    // 删除聊天会话
    crow::response deleteChatSession(const crow::request& req);
    
    // 流式聊天（SSE）
    crow::response chatStream(const crow::request& req);
};

} // namespace controllers
} // namespace Yachiyo