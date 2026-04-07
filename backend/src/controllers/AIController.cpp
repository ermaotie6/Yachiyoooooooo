#include "controllers/AIController.hpp"
#include "services/AIService.hpp"
#include "utils/JsonUtils.hpp"
#include "utils/LogUtils.hpp"
#include <crow.h>
#include <sstream>

namespace yachiyo::controllers {

using yachiyo::utils::LogUtils;
namespace services = Yachiyo::Services;
namespace utils = yachiyo::utils;

AIController::AIController() {
    logger = LogUtils::getLogger("AIController");
    aiService = std::make_shared<services::AIServiceImpl>();
}

void AIController::registerRoutes(crow::SimpleApp& app) {
    // 文本聊天
    CROW_ROUTE(app, "/api/v2/ai/chat")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            return this->chat(req);
        });

    // 流式聊天
    CROW_ROUTE(app, "/api/v2/ai/chat/stream")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            return this->chatStream(req);
        });

    // 语音合成
    CROW_ROUTE(app, "/api/v2/ai/tts")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            return this->textToSpeech(req);
        });

    // 语音识别
    CROW_ROUTE(app, "/api/v2/ai/stt")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            return this->speechToText(req);
        });

    // 图像生成
    CROW_ROUTE(app, "/api/v2/ai/image/generate")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            return this->generateImage(req);
        });

    // 图像分析
    CROW_ROUTE(app, "/api/v2/ai/image/analyze")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            return this->analyzeImage(req);
        });

    // 获取可用模型
    CROW_ROUTE(app, "/api/v2/ai/models")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            return this->getModels(req);
        });

    // 获取聊天历史
    CROW_ROUTE(app, "/api/v2/ai/history")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            return this->getChatHistory(req);
        });

    // 删除聊天记录
    CROW_ROUTE(app, "/api/v2/ai/history/<string>")
        .methods("DELETE"_method)
        ([this](const crow::request& req, const std::string& chatId) {
            return this->deleteChatHistory(req, chatId);
        });

    logger->info("AI控制器路由已注册");
}

crow::response AIController::chat(const crow::request& req) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        auto json = crow::json::load(req.body);
        if (!json) {
            return utils::JsonUtils::createErrorResponse(400, "无效的JSON数据");
        }

        std::string message = json["message"].s();
        std::string model = json.has("model") ? json["model"].s() : "gpt-3.5-turbo";
        std::string chatId = json.has("chat_id") ? json["chat_id"].s() : "";
        double temperature = json.has("temperature") ? json["temperature"].d() : 0.7;
        int maxTokens = json.has("max_tokens") ? static_cast<int>(json["max_tokens"].i()) : 1000;
        
        if (message.empty()) {
            return utils::JsonUtils::createErrorResponse(400, "消息内容不能为空");
        }

        // 构建消息历史
        std::vector<services::ChatMessage> history;
        if (json.has("history") && json["history"].t() == crow::json::type::List) {
            auto historyArray = json["history"];
            for (size_t i = 0; i < historyArray.size(); ++i) {
                auto msg = historyArray[i];
                services::ChatMessage chatMsg;
                chatMsg.role = msg["role"].s();
                chatMsg.content = msg["content"].s();
                history.push_back(chatMsg);
            }
        }

        auto result = aiService->chatCompletion(token, message, model, chatId, 
                                               history, temperature, maxTokens);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "聊天完成";
        response["data"]["response"] = result.response;
        response["data"]["chat_id"] = result.chatId;
        response["data"]["message_id"] = result.messageId;
        response["data"]["model"] = result.model;
        response["data"]["tokens_used"] = result.tokensUsed;
        response["data"]["response_time"] = result.responseTime;
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("聊天请求失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response AIController::chatStream(const crow::request& req) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        auto json = crow::json::load(req.body);
        if (!json) {
            return utils::JsonUtils::createErrorResponse(400, "无效的JSON数据");
        }

        std::string message = json["message"].s();
        std::string model = json.has("model") ? json["model"].s() : "gpt-3.5-turbo";
        std::string chatId = json.has("chat_id") ? json["chat_id"].s() : "";
        
        if (message.empty()) {
            return utils::JsonUtils::createErrorResponse(400, "消息内容不能为空");
        }

        // 创建流式响应
        auto response = crow::response();
        response.set_header("Content-Type", "text/event-stream");
        response.set_header("Cache-Control", "no-cache");
        response.set_header("Connection", "keep-alive");
        
        // 设置响应体写入器
        response.write("data: {\"event\":\"start\"}\n\n");
        
        // 这里应该实现真正的流式响应
        // 由于Crow框架的限制，这里简化处理
        auto result = aiService->chatCompletion(token, message, model, chatId, 
                                               {}, 0.7, 1000);
        
        if (result.success) {
            // 模拟流式输出
            std::stringstream ss(result.response);
            std::string line;
            while (std::getline(ss, line, ' ')) {
                if (!line.empty()) {
                    crow::json::wvalue chunk;
                    chunk["event"] = "chunk";
                    chunk["data"] = line + " ";
                    response.write("data: " + crow::json::dump(chunk) + "\n\n");
                }
            }
            
            crow::json::wvalue endChunk;
            endChunk["event"] = "end";
            endChunk["data"]["chat_id"] = result.chatId;
            endChunk["data"]["message_id"] = result.messageId;
            endChunk["data"]["tokens_used"] = result.tokensUsed;
            response.write("data: " + crow::json::dump(endChunk) + "\n\n");
        } else {
            crow::json::wvalue errorChunk;
            errorChunk["event"] = "error";
            errorChunk["data"] = result.message;
            response.write("data: " + crow::json::dump(errorChunk) + "\n\n");
        }
        
        response.write("data: {\"event\":\"done\"}\n\n");
        return response;

    } catch (const std::exception& e) {
        logger->error("流式聊天请求失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response AIController::textToSpeech(const crow::request& req) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        auto json = crow::json::load(req.body);
        if (!json) {
            return utils::JsonUtils::createErrorResponse(400, "无效的JSON数据");
        }

        std::string text = json["text"].s();
        std::string voice = json.has("voice") ? json["voice"].s() : "alloy";
        double speed = json.has("speed") ? json["speed"].d() : 1.0;
        
        if (text.empty()) {
            return utils::JsonUtils::createErrorResponse(400, "文本内容不能为空");
        }

        auto result = aiService->textToSpeech(token, text, voice, speed);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        // 返回音频数据
        auto response = crow::response(result.audioData);
        response.set_header("Content-Type", "audio/mpeg");
        response.set_header("Content-Length", std::to_string(result.audioData.size()));
        response.set_header("X-Audio-Duration", std::to_string(result.duration));
        return response;

    } catch (const std::exception& e) {
        logger->error("语音合成失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response AIController::speechToText(const crow::request& req) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 检查内容类型
        std::string contentType = req.get_header_value("Content-Type");
        if (contentType.find("audio/") == std::string::npos && 
            contentType.find("multipart/form-data") == std::string::npos) {
            return utils::JsonUtils::createErrorResponse(400, "无效的内容类型，期望音频文件");
        }

        // 获取音频数据
        std::string audioData = req.body;
        if (audioData.empty()) {
            return utils::JsonUtils::createErrorResponse(400, "音频数据不能为空");
        }

        std::string language = req.get_header_value("X-Language");
        if (language.empty()) {
            language = "zh-CN";
        }

        auto result = aiService->speechToText(token, audioData, language);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "语音识别完成";
        response["data"]["text"] = result.text;
        response["data"]["language"] = result.language;
        response["data"]["confidence"] = result.confidence;
        response["data"]["duration"] = result.duration;
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("语音识别失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response AIController::generateImage(const crow::request& req) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        auto json = crow::json::load(req.body);
        if (!json) {
            return utils::JsonUtils::createErrorResponse(400, "无效的JSON数据");
        }

        std::string prompt = json["prompt"].s();
        std::string size = json.has("size") ? json["size"].s() : "1024x1024";
        int n = json.has("n") ? static_cast<int>(json["n"].i()) : 1;
        std::string style = json.has("style") ? json["style"].s() : "vivid";
        
        if (prompt.empty()) {
            return utils::JsonUtils::createErrorResponse(400, "提示词不能为空");
        }

        auto result = aiService->generateImage(token, prompt, size, n, style);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "图像生成完成";
        
        // 添加图像URL或Base64数据
        crow::json::wvalue images = crow::json::wvalue::list();
        for (size_t i = 0; i < result.imageUrls.size(); ++i) {
            crow::json::wvalue img;
            img["url"] = result.imageUrls[i];
            img["revised_prompt"] = result.revisedPrompts[i];
            images[i] = img;
        }
        response["data"]["images"] = images;
        response["data"]["created"] = result.created;
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("图像生成失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response AIController::analyzeImage(const crow::request& req) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        auto json = crow::json::load(req.body);
        if (!json) {
            return utils::JsonUtils::createErrorResponse(400, "无效的JSON数据");
        }

        std::string imageUrl = json["image_url"].s();
        std::string imageBase64 = json.has("image_base64") ? json["image_base64"].s() : "";
        std::string prompt = json.has("prompt") ? json["prompt"].s() : "描述这张图片";
        
        if (imageUrl.empty() && imageBase64.empty()) {
            return utils::JsonUtils::createErrorResponse(400, "必须提供图像URL或Base64数据");
        }

        auto result = aiService->analyzeImage(token, imageUrl, imageBase64, prompt);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "图像分析完成";
        response["data"]["analysis"] = result.analysis;
        response["data"]["tags"] = crow::json::wvalue::list();
        
        for (size_t i = 0; i < result.tags.size(); ++i) {
            response["data"]["tags"][i] = result.tags[i];
        }
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("图像分析失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response AIController::getModels(const crow::request& req) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        auto result = aiService->getAvailableModels(token);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "获取模型列表成功";
        response["data"]["models"] = crow::json::wvalue::list();
        
        for (size_t i = 0; i < result.models.size(); ++i) {
            crow::json::wvalue model;
            model["id"] = result.models[i].id;
            model["name"] = result.models[i].name;
            model["provider"] = result.models[i].provider;
            model["type"] = result.models[i].type;
            model["max_tokens"] = result.models[i].maxTokens;
            model["supports_vision"] = result.models[i].supportsVision;
            model["supports_audio"] = result.models[i].supportsAudio;
            model["is_available"] = result.models[i].isAvailable;
            response["data"]["models"][i] = model;
        }
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("获取模型列表失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response AIController::getChatHistory(const crow::request& req) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        // 获取查询参数
        std::string chatId = req.url_params.get("chat_id");
        int limit = 50;
        if (req.url_params.get("limit")) {
            limit = std::stoi(req.url_params.get("limit"));
        }
        int offset = 0;
        if (req.url_params.get("offset")) {
            offset = std::stoi(req.url_params.get("offset"));
        }

        auto result = aiService->getChatHistory(token, chatId, limit, offset);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "获取聊天历史成功";
        response["data"]["total"] = result.total;
        response["data"]["limit"] = limit;
        response["data"]["offset"] = offset;
        response["data"]["chats"] = crow::json::wvalue::list();
        
        for (size_t i = 0; i < result.chats.size(); ++i) {
            crow::json::wvalue chat;
            chat["id"] = result.chats[i].id;
            chat["title"] = result.chats[i].title;
            chat["model"] = result.chats[i].model;
            chat["message_count"] = result.chats[i].messageCount;
            chat["created_at"] = result.chats[i].createdAt;
            chat["updated_at"] = result.chats[i].updatedAt;
            
            // 添加消息列表
            chat["messages"] = crow::json::wvalue::list();
            for (size_t j = 0; j < result.chats[i].messages.size(); ++j) {
                crow::json::wvalue msg;
                msg["id"] = result.chats[i].messages[j].id;
                msg["role"] = result.chats[i].messages[j].role;
                msg["content"] = result.chats[i].messages[j].content;
                msg["tokens"] = result.chats[i].messages[j].tokens;
                msg["created_at"] = result.chats[i].messages[j].createdAt;
                chat["messages"][j] = msg;
            }
            
            response["data"]["chats"][i] = chat;
        }
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("获取聊天历史失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

crow::response AIController::deleteChatHistory(const crow::request& req, const std::string& chatId) {
    try {
        // 验证认证令牌
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return utils::JsonUtils::createErrorResponse(401, "未提供有效的认证令牌");
        }
        std::string token = authHeader.substr(7);

        if (chatId.empty()) {
            return utils::JsonUtils::createErrorResponse(400, "聊天ID不能为空");
        }

        auto result = aiService->deleteChatHistory(token, chatId);

        if (!result.success) {
            return utils::JsonUtils::createErrorResponse(400, result.message);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = "删除聊天记录成功";
        response["data"]["deleted_count"] = result.deletedCount;
        
        auto resp = crow::response(response);
        resp.set_header("Content-Type", "application/json");
        return resp;

    } catch (const std::exception& e) {
        logger->error("删除聊天记录失败: {}", e.what());
        return utils::JsonUtils::createErrorResponse(500, "服务器内部错误");
    }
}

} // namespace yachiyo::controllers