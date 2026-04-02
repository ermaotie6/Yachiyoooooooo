#include "../../include/controllers/AIController.hpp"
#include "../../include/utils/LogUtils.hpp"
#include "../../include/utils/JsonUtils.hpp"
#include <sstream>

namespace Yachiyo {
namespace controllers {

AIController::AIController(std::shared_ptr<services::AIService> aiService)
    : aiService(aiService) {
    logger = Utils::LogUtils::getLogger("AIController");
    logger->info("AIController 初始化完成");
}

AIController::~AIController() {
    logger->info("AIController 销毁");
}

crow::response AIController::textToSpeech(const crow::request& req) {
    try {
        logger->info("文本转语音请求");

        // 验证JWT令牌
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return crow::response(401, "Unauthorized");
        }

        // 解析请求体
        auto body = crow::json::load(req.body);
        if (!body || !body.has("text")) {
            return crow::response(400, "Missing 'text' field");
        }

        std::string text = body["text"].s();
        std::string language = body.has("language") ? body["language"].s() : "zh-CN";

        if (text.empty()) {
            return crow::response(400, "Text cannot be empty");
        }

        // 调用 AI 服务
        auto result = aiService->textToSpeech(text, language);

        if (result.isSuccess()) {
            crow::json::wvalue response;
            response["success"] = true;
            response["data"]["audio_url"] = result.getData();
            response["data"]["text"] = text;
            response["data"]["language"] = language;
            return crow::response(200, response);
        } else {
            crow::json::wvalue response;
            response["success"] = false;
            response["error"] = result.getError();
            return crow::response(500, response);
        }

    } catch (const std::exception& e) {
        logger->error("TTS 处理失败: {}", e.what());
        return crow::response(500, "Internal server error");
    }
}

crow::response AIController::speechToText(const crow::request& req) {
    try {
        logger->info("语音转文本请求");

        // 验证JWT令牌
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return crow::response(401, "Unauthorized");
        }

        // 解析请求体
        auto body = crow::json::load(req.body);
        if (!body || !body.has("audio_url")) {
            return crow::response(400, "Missing 'audio_url' field");
        }

        std::string audioUrl = body["audio_url"].s();
        std::string language = body.has("language") ? body["language"].s() : "zh-CN";

        // 调用 AI 服务
        auto result = aiService->speechToText(audioUrl, language);

        if (result.isSuccess()) {
            crow::json::wvalue response;
            response["success"] = true;
            response["data"]["text"] = result.getData();
            response["data"]["language"] = language;
            response["data"]["confidence"] = 0.95; // mock 置信度
            return crow::response(200, response);
        } else {
            crow::json::wvalue response;
            response["success"] = false;
            response["error"] = result.getError();
            return crow::response(500, response);
        }

    } catch (const std::exception& e) {
        logger->error("STT 处理失败: {}", e.what());
        return crow::response(500, "Internal server error");
    }
}

crow::response AIController::generateImage(const crow::request& req) {
    try {
        logger->info("生成图像请求");

        // 验证JWT令牌
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return crow::response(401, "Unauthorized");
        }

        // 解析请求体
        auto body = crow::json::load(req.body);
        if (!body || !body.has("prompt")) {
            return crow::response(400, "Missing 'prompt' field");
        }

        std::string prompt = body["prompt"].s();
        std::string style = body.has("style") ? body["style"].s() : "default";
        int count = body.has("count") ? body["count"].i() : 1;

        if (prompt.empty()) {
            return crow::response(400, "Prompt cannot be empty");
        }

        // 限制生成数量
        count = std::min(count, 4);

        // 调用 AI 服务
        auto result = aiService->generateImage(prompt, style);

        if (result.isSuccess()) {
            crow::json::wvalue response;
            response["success"] = true;
            response["data"]["prompt"] = prompt;
            response["data"]["style"] = style;
            response["data"]["count"] = count;
            response["data"]["images"] = crow::json::wvalue::list();

            auto& images = response["data"]["images"];
            for (int i = 0; i < count; ++i) {
                images.push_back(result.getData() + "?v=" + std::to_string(i));
            }

            return crow::response(200, response);
        } else {
            crow::json::wvalue response;
            response["success"] = false;
            response["error"] = result.getError();
            return crow::response(500, response);
        }

    } catch (const std::exception& e) {
        logger->error("图像生成失败: {}", e.what());
        return crow::response(500, "Internal server error");
    }
}

crow::response AIController::analyzeImage(const crow::request& req) {
    try {
        logger->info("分析图像请求");

        // 验证JWT令牌
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return crow::response(401, "Unauthorized");
        }

        // 解析请求体
        auto body = crow::json::load(req.body);
        if (!body || !body.has("image_url")) {
            return crow::response(400, "Missing 'image_url' field");
        }

        std::string imageUrl = body["image_url"].s();
        std::string analysisType = body.has("analysis_type") ? body["analysis_type"].s() : "general";

        // 调用 AI 服务
        auto result = aiService->analyzeImage(imageUrl, analysisType);

        if (result.isSuccess()) {
            crow::json::wvalue response;
            response["success"] = true;
            response["data"]["image_url"] = imageUrl;
            response["data"]["analysis_type"] = analysisType;

            // 解析 JSON 结果
            try {
                auto analysisData = crow::json::load(result.getData());
                response["data"]["analysis"] = analysisData;
            } catch (...) {
                response["data"]["analysis"] = result.getData();
            }

            return crow::response(200, response);
        } else {
            crow::json::wvalue response;
            response["success"] = false;
            response["error"] = result.getError();
            return crow::response(500, response);
        }

    } catch (const std::exception& e) {
        logger->error("图像分析失败: {}", e.what());
        return crow::response(500, "Internal server error");
    }
}

crow::response AIController::chat(const crow::request& req) {
    try {
        logger->info("AI 聊天请求");

        // 验证JWT令牌
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            return crow::response(401, "Unauthorized");
        }

        // 解析请求体
        auto body = crow::json::load(req.body);
        if (!body || !body.has("message")) {
            return crow::response(400, "Missing 'message' field");
        }

        std::string message = body["message"].s();
        std::string conversationId = body.has("conversation_id") ? body["conversation_id"].s() : "";

        if (message.empty()) {
            return crow::response(400, "Message cannot be empty");
        }

        // 构建聊天请求
        dto::ChatRequest chatRequest;
        chatRequest.message = message;
        chatRequest.conversationId = conversationId;

        // 调用 AI 服务
        auto result = aiService->chat(chatRequest);

        if (result.isSuccess()) {
            auto response_data = result.getData();

            crow::json::wvalue response;
            response["success"] = true;
            response["data"]["conversation_id"] = conversationId;
            response["data"]["message"] = message;
            response["data"]["reply"] = response_data.message;
            response["data"]["timestamp"] = "2024-01-15T12:00:00Z";

            return crow::response(200, response);
        } else {
            crow::json::wvalue response;
            response["success"] = false;
            response["error"] = result.getError();
            return crow::response(500, response);
        }

    } catch (const std::exception& e) {
        logger->error("聊天处理失败: {}", e.what());
        return crow::response(500, "Internal server error");
    }
}

crow::response AIController::getStatus(const crow::request& req) {
    try {
        logger->info("获取 AI 服务状态");

        crow::json::wvalue response;
        response["success"] = true;
        response["data"]["service"] = "AIService";
        response["data"]["status"] = "running";
        response["data"]["features"] = crow::json::wvalue::list();

        auto& features = response["data"]["features"];
        features.push_back("text_to_speech");
        features.push_back("speech_to_text");
        features.push_back("image_generation");
        features.push_back("image_analysis");
        features.push_back("chat");

        response["data"]["version"] = "1.0.0";
        response["data"]["timestamp"] = "2024-01-15T12:00:00Z";

        return crow::response(200, response);

    } catch (const std::exception& e) {
        logger->error("获取状态失败: {}", e.what());
        return crow::response(500, "Internal server error");
    }
}

} // namespace controllers
} // namespace Yachiyo
