#include "services/AIService.hpp"
#include "utils/LogUtils.hpp"
#include "utils/JsonUtils.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>

namespace Yachiyo {
namespace Services {

// 回调函数用于CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ==================== 构造/析构 ====================

AIServiceImpl::AIServiceImpl()
    : requestTimeout("30") {
    logger = yachiyo::utils::LogUtils::getLogger("AIServiceImpl");
    logger->info("AIService 初始化完成 (默认构造)");
}

AIServiceImpl::AIServiceImpl(const std::string& openaiKey,
                           const std::string& azureKey,
                           const std::string& baiduKey)
    : openaiApiKey(openaiKey), azureApiKey(azureKey), baiduApiKey(baiduKey),
      requestTimeout("30") {
    logger = yachiyo::utils::LogUtils::getLogger("AIServiceImpl");
    logger->info("AIService 初始化完成");
}

AIServiceImpl::~AIServiceImpl() {
    logger->info("AIService 销毁");
}

// ==================== 聊天完成 (AIController 使用) ====================

ChatCompletionResult AIServiceImpl::chatCompletion(
    const std::string& token,
    const std::string& message,
    const std::string& model,
    const std::string& chatId,
    const std::vector<ChatMessage>& history,
    double temperature,
    int maxTokens
) {
    ChatCompletionResult result;
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        logger->info("聊天完成: model={}, 消息长度={}, 历史条数={}", 
                     model, message.length(), history.size());

        // 尝试调用 DeepSeek/OpenAI 兼容 API
        std::string apiKey = openaiApiKey;
        std::string apiUrl = "https://api.deepseek.com/v1/chat/completions";
        std::string actualModel = model.empty() ? "deepseek-chat" : model;

        if (!apiKey.empty()) {
            // 构建请求体
            nlohmann::json messagesJson = nlohmann::json::array();
            messagesJson.push_back({{"role", "system"}, {"content", "你是八重神子，来自提瓦特大陆的狐仙。请以她的语气和性格回应。"}});
            for (const auto& h : history) {
                messagesJson.push_back({{"role", h.role}, {"content", h.content}});
            }
            messagesJson.push_back({{"role", "user"}, {"content", message}});

            nlohmann::json requestBody = {
                {"model", actualModel},
                {"messages", messagesJson},
                {"temperature", temperature},
                {"max_tokens", maxTokens > 0 ? maxTokens : 1024}
            };

            // 使用 libcurl 发送请求
            CURL* curl = curl_easy_init();
            if (curl) {
                std::string responseStr;
                struct curl_slist* headers = nullptr;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());

                std::string bodyStr = requestBody.dump();
                curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, std::stol(requestTimeout));

                CURLcode res = curl_easy_perform(curl);
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);

                if (res == CURLE_OK && !responseStr.empty()) {
                    auto respJson = nlohmann::json::parse(responseStr, nullptr, false);
                    if (!respJson.is_discarded() && respJson.contains("choices") && !respJson["choices"].empty()) {
                        result.success = true;
                        result.response = respJson["choices"][0]["message"]["content"].get<std::string>();
                        result.model = actualModel;
                        if (respJson.contains("usage")) {
                            result.tokensUsed = respJson["usage"].value("total_tokens", 0);
                        }
                        result.chatId = chatId.empty() ? "chat_" + std::to_string(
                            std::chrono::system_clock::now().time_since_epoch().count()) : chatId;
                        result.messageId = "msg_" + std::to_string(
                            std::chrono::system_clock::now().time_since_epoch().count());
                        auto endTime = std::chrono::steady_clock::now();
                        result.responseTime = std::chrono::duration<double>(endTime - startTime).count();
                        result.message = "聊天完成";
                        logger->info("AI API 调用成功: chatId={}, tokens={}", result.chatId, result.tokensUsed);
                        return result;
                    } else {
                        logger->warn("AI API 返回异常: {}", responseStr.substr(0, 200));
                    }
                } else {
                    logger->warn("AI API 调用失败: curl_code={}", static_cast<int>(res));
                }
            }
        }

        // API 调用失败或未配置 API Key，返回 fallback 响应
        logger->warn("AI API 未配置或调用失败，返回 fallback 响应");
        result.success = true;
        result.response = "抱歉，AI 服务暂时不可用，请稍后再试。";
        result.chatId = chatId.empty() ? "chat_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count()) : chatId;
        result.messageId = "msg_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
        result.model = actualModel;
        result.tokensUsed = 0;
        
        auto endTime = std::chrono::steady_clock::now();
        result.responseTime = std::chrono::duration<double>(endTime - startTime).count();
        result.message = "聊天完成 (fallback)";
        
    } catch (const std::exception& e) {
        logger->error("聊天完成失败: {}", e.what());
        result.success = false;
        result.message = std::string("聊天失败: ") + e.what();
    }
    
    return result;
}

// ==================== DTO 版 chat ====================

Utils::Result<dto::ChatRequest> AIServiceImpl::chat(const dto::ChatRequest& request) {
    try {
        logger->info("AI聊天(DTO): 消息长度={}", request.getMessage().length());

        // 通过 chatCompletion 统一调用 AI API
        ChatCompletionResult aiResult = chatCompletion(
            "", request.getMessage(), "deepseek-chat",
            request.getConversationId(), {}, 0.7, 1024
        );

        dto::ChatRequest response;
        response.setMessage(aiResult.success ? aiResult.response : "AI 服务暂时不可用，请稍后再试。");
        response.setConversationId(request.getConversationId());
        
        logger->info("聊天响应完成");
        return Utils::Result<dto::ChatRequest>::success(response);

    } catch (const std::exception& e) {
        logger->error("聊天失败: {}", e.what());
        return Utils::Result<dto::ChatRequest>::error(std::string("聊天失败: ") + e.what());
    }
}

// ==================== TTS (AIController 版) ====================

TTSResult AIServiceImpl::textToSpeech(
    const std::string& token,
    const std::string& text,
    const std::string& voice,
    double speed
) {
    TTSResult result;
    try {
        logger->info("TTS: 文本长度={}, voice={}, speed={}", text.length(), voice, speed);
        
        // TODO: 调用真实的 TTS API (Azure/Baidu)
        result.success = true;
        result.message = "语音合成完成";
        result.audioData = "MOCK_AUDIO_DATA";  // 实际应为音频二进制数据
        result.duration = text.length() * 0.1;  // 估算时长
        
    } catch (const std::exception& e) {
        logger->error("TTS 失败: {}", e.what());
        result.success = false;
        result.message = std::string("TTS 失败: ") + e.what();
    }
    return result;
}

// ==================== STT (AIController 版) ====================

STTResult AIServiceImpl::speechToText(
    const std::string& token,
    const std::string& audioData,
    const std::string& language
) {
    STTResult result;
    try {
        logger->info("STT: 音频大小={}, 语言={}", audioData.length(), language);
        
        // TODO: 调用真实的 STT API (Azure/Baidu)
        result.success = true;
        result.message = "语音识别完成";
        result.text = "这是来自音频的识别文本";
        result.language = language;
        result.confidence = 0.95;
        result.duration = 3.5;
        
    } catch (const std::exception& e) {
        logger->error("STT 失败: {}", e.what());
        result.success = false;
        result.message = std::string("STT 失败: ") + e.what();
    }
    return result;
}

// ==================== TTS (简单版本) ====================

Utils::Result<std::string> AIServiceImpl::textToSpeech(
    const std::string& text,
    const std::string& language) {
    try {
        logger->info("文本转语音: 文本长度={}, 语言={}", text.length(), language);

        std::string mockUrl = "https://example.com/audio/tts_" + 
                             std::to_string(std::hash<std::string>{}(text) % 10000) + ".mp3";
        
        logger->info("TTS 完成: URL={}", mockUrl);
        return Utils::Result<std::string>::success(mockUrl);

    } catch (const std::exception& e) {
        logger->error("TTS 失败: {}", e.what());
        return Utils::Result<std::string>::error(std::string("TTS 失败: ") + e.what());
    }
}

// ==================== STT (简单版本) ====================

Utils::Result<std::string> AIServiceImpl::speechToText(
    const std::string& audioPath,
    const std::string& language) {
    try {
        logger->info("语音转文本: 音频={}, 语言={}", audioPath, language);

        std::string mockResult = "这是来自音频文件的识别文本";
        
        logger->info("STT 完成: 文本长度={}", mockResult.length());
        return Utils::Result<std::string>::success(mockResult);

    } catch (const std::exception& e) {
        logger->error("STT 失败: {}", e.what());
        return Utils::Result<std::string>::error(std::string("STT 失败: ") + e.what());
    }
}

// ==================== 图像生成 (AIController 版) ====================

ImageGenerationResult AIServiceImpl::generateImage(
    const std::string& token,
    const std::string& prompt,
    const std::string& size,
    int n,
    const std::string& style
) {
    ImageGenerationResult result;
    try {
        logger->info("生成图像: prompt={}, size={}, n={}, style={}", prompt, size, n, style);
        
        // TODO: 调用真实的图像生成 API (DALL-E/Stable Diffusion)
        result.success = true;
        result.message = "图像生成完成";
        result.created = std::chrono::system_clock::now().time_since_epoch().count();
        
        for (int i = 0; i < n; ++i) {
            result.imageUrls.push_back("https://example.com/images/generated_" +
                std::to_string(std::hash<std::string>{}(prompt) % 10000 + i) + ".png");
            result.revisedPrompts.push_back(prompt);
        }
        
    } catch (const std::exception& e) {
        logger->error("图像生成失败: {}", e.what());
        result.success = false;
        result.message = std::string("图像生成失败: ") + e.what();
    }
    return result;
}

// ==================== 图像分析 (AIController 版) ====================

ImageAnalysisResult AIServiceImpl::analyzeImage(
    const std::string& token,
    const std::string& imageUrl,
    const std::string& imageBase64,
    const std::string& prompt
) {
    ImageAnalysisResult result;
    try {
        logger->info("分析图像: url={}, prompt={}", imageUrl, prompt);
        
        // TODO: 调用真实的图像分析 API (Vision API)
        result.success = true;
        result.message = "图像分析完成";
        result.analysis = "这是一张包含人物和物品的图片。";
        result.tags = {"indoor", "modern", "technology", "person"};
        
    } catch (const std::exception& e) {
        logger->error("图像分析失败: {}", e.what());
        result.success = false;
        result.message = std::string("图像分析失败: ") + e.what();
    }
    return result;
}

// ==================== 图像 (简单版本) ====================

Utils::Result<std::string> AIServiceImpl::generateImage(
    const std::string& prompt,
    const std::string& style) {
    try {
        logger->info("生成图像: prompt={}, style={}", prompt, style);

        std::string mockUrl = "https://example.com/images/generated_" +
                             std::to_string(std::hash<std::string>{}(prompt) % 10000) + ".png";
        
        logger->info("图像生成完成: URL={}", mockUrl);
        return Utils::Result<std::string>::success(mockUrl);

    } catch (const std::exception& e) {
        logger->error("图像生成失败: {}", e.what());
        return Utils::Result<std::string>::error(std::string("图像生成失败: ") + e.what());
    }
}

Utils::Result<std::string> AIServiceImpl::analyzeImage(
    const std::string& imagePath,
    const std::string& analysisType) {
    try {
        logger->info("分析图像: image={}, type={}", imagePath, analysisType);

        std::string mockResult = R"({
            "type": "object_detection",
            "confidence": 0.95,
            "objects": [
                {"name": "person", "confidence": 0.92},
                {"name": "phone", "confidence": 0.88}
            ],
            "tags": ["indoor", "modern", "technology"]
        })";
        
        logger->info("图像分析完成");
        return Utils::Result<std::string>::success(mockResult);

    } catch (const std::exception& e) {
        logger->error("图像分析失败: {}", e.what());
        return Utils::Result<std::string>::error(std::string("图像分析失败: ") + e.what());
    }
}

// ==================== 模型管理 ====================

ModelsResult AIServiceImpl::getAvailableModels(const std::string& token) {
    ModelsResult result;
    try {
        result.success = true;
        result.message = "获取模型列表成功";
        
        // 返回可用的模型列表
        result.models = {
            {"gpt-3.5-turbo", "GPT-3.5 Turbo", "OpenAI", "chat", 4096, false, false, true},
            {"gpt-4", "GPT-4", "OpenAI", "chat", 8192, true, false, !openaiApiKey.empty()},
            {"gpt-4-vision", "GPT-4 Vision", "OpenAI", "chat", 8192, true, false, !openaiApiKey.empty()},
            {"deepseek-chat", "DeepSeek Chat", "DeepSeek", "chat", 4096, false, false, true},
            {"dall-e-3", "DALL-E 3", "OpenAI", "image", 0, false, false, !openaiApiKey.empty()},
        };
        
    } catch (const std::exception& e) {
        logger->error("获取模型列表失败: {}", e.what());
        result.success = false;
        result.message = std::string("获取模型列表失败: ") + e.what();
    }
    return result;
}

// ==================== 聊天历史 ====================

ChatHistoryResult AIServiceImpl::getChatHistory(
    const std::string& token,
    const std::string& chatId,
    int limit,
    int offset
) {
    ChatHistoryResult result;
    try {
        logger->info("获取聊天历史: chatId={}, limit={}, offset={}", chatId, limit, offset);
        
        // TODO: 从数据库获取聊天历史
        result.success = true;
        result.message = "获取聊天历史成功";
        result.total = 0;
        
    } catch (const std::exception& e) {
        logger->error("获取聊天历史失败: {}", e.what());
        result.success = false;
        result.message = std::string("获取聊天历史失败: ") + e.what();
    }
    return result;
}

DeleteChatResult AIServiceImpl::deleteChatHistory(
    const std::string& token,
    const std::string& chatId
) {
    DeleteChatResult result;
    try {
        logger->info("删除聊天记录: chatId={}", chatId);
        
        // TODO: 从数据库删除聊天记录
        result.success = true;
        result.message = "删除聊天记录成功";
        result.deletedCount = 0;
        
    } catch (const std::exception& e) {
        logger->error("删除聊天记录失败: {}", e.what());
        result.success = false;
        result.message = std::string("删除聊天记录失败: ") + e.what();
    }
    return result;
}

// ==================== 内部 API 调用 ====================

Utils::Result<std::string> AIServiceImpl::callOpenAIAPI(
    const std::string& endpoint,
    const std::string& payload) {
    try {
        if (openaiApiKey.empty()) {
            return Utils::Result<std::string>::error("OpenAI API 密钥未配置");
        }

        logger->debug("调用 OpenAI API: endpoint={}", endpoint);

        // TODO: 使用 libcurl 实现 HTTP 请求
        return Utils::Result<std::string>::success("{}");

    } catch (const std::exception& e) {
        logger->error("OpenAI API 调用失败: {}", e.what());
        return Utils::Result<std::string>::error("API 调用失败");
    }
}

Utils::Result<std::string> AIServiceImpl::callAzureAPI(
    const std::string& endpoint,
    const std::string& payload) {
    try {
        if (azureApiKey.empty()) {
            return Utils::Result<std::string>::error("Azure API 密钥未配置");
        }

        logger->debug("调用 Azure API: endpoint={}", endpoint);
        return Utils::Result<std::string>::success("{}");

    } catch (const std::exception& e) {
        logger->error("Azure API 调用失败: {}", e.what());
        return Utils::Result<std::string>::error("API 调用失败");
    }
}

Utils::Result<std::string> AIServiceImpl::callBaiduAPI(
    const std::string& endpoint,
    const std::string& payload) {
    try {
        if (baiduApiKey.empty()) {
            return Utils::Result<std::string>::error("百度 API 密钥未配置");
        }

        logger->debug("调用百度 API: endpoint={}", endpoint);
        return Utils::Result<std::string>::success("{}");

    } catch (const std::exception& e) {
        logger->error("百度 API 调用失败: {}", e.what());
        return Utils::Result<std::string>::error("API 调用失败");
    }
}

} // namespace Services
} // namespace Yachiyo
