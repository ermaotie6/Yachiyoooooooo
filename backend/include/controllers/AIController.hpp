#pragma once

#include "BaseController.hpp"
#include "../services/AIService.hpp"
#include "../services/ChatService.hpp"
#include "../utils/LogUtils.hpp"
#include <crow.h>
#include <memory>
#include <spdlog/spdlog.h>

namespace yachiyo::controllers {

class AIController : public BaseController {
public:
    AIController();
    
    void registerRoutes(crow::SimpleApp& app);
    
private:
    std::shared_ptr<Yachiyo::Services::AIServiceImpl> aiService;
    std::shared_ptr<spdlog::logger> logger;
    
    // AI 聊天
    crow::response chat(const crow::request& req);
    crow::response chatStream(const crow::request& req);
    
    // 语音
    crow::response textToSpeech(const crow::request& req);
    crow::response speechToText(const crow::request& req);
    
    // 图像
    crow::response generateImage(const crow::request& req);
    crow::response analyzeImage(const crow::request& req);
    
    // 模型管理
    crow::response getModels(const crow::request& req);
    
    // 聊天历史
    crow::response getChatHistory(const crow::request& req);
    crow::response deleteChatHistory(const crow::request& req, const std::string& chatId);
};

} // namespace yachiyo::controllers