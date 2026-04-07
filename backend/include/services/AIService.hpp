#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <chrono>
#include <spdlog/spdlog.h>
#include "dto/ChatRequest.hpp"
#include "utils/Result.hpp"

namespace Yachiyo {
namespace Services {

// DTO 命名空间别名
namespace dto = ::Yachiyo::DTO;

// ==================== 数据结构 ====================

// 聊天消息 (用于历史记录传递)
struct ChatMessage {
    std::string role;    // "system", "user", "assistant"
    std::string content;
    std::string id;
    int tokens = 0;
    std::string createdAt;
    
    ChatMessage() = default;
    ChatMessage(const std::string& role, const std::string& content)
        : role(role), content(content) {}
};

// 聊天完成结果
struct ChatCompletionResult {
    bool success = false;
    std::string message;
    std::string response;       // AI 回复内容
    std::string chatId;
    std::string messageId;
    std::string model;
    int tokensUsed = 0;
    double responseTime = 0.0;  // 响应时间(秒)
};

// TTS 结果
struct TTSResult {
    bool success = false;
    std::string message;
    std::string audioData;      // 音频数据 (可能是二进制或Base64)
    double duration = 0.0;      // 音频时长(秒)
};

// STT 结果
struct STTResult {
    bool success = false;
    std::string message;
    std::string text;           // 识别出的文本
    std::string language;       // 检测到的语言
    double confidence = 0.0;    // 置信度
    double duration = 0.0;      // 音频时长(秒)
};

// 图像生成结果
struct ImageGenerationResult {
    bool success = false;
    std::string message;
    std::vector<std::string> imageUrls;
    std::vector<std::string> revisedPrompts;
    int64_t created = 0;
};

// 图像分析结果
struct ImageAnalysisResult {
    bool success = false;
    std::string message;
    std::string analysis;
    std::vector<std::string> tags;
};

// AI 模型信息
struct AIModelInfo {
    std::string id;
    std::string name;
    std::string provider;
    std::string type;           // "chat", "image", "audio"
    int maxTokens = 0;
    bool supportsVision = false;
    bool supportsAudio = false;
    bool isAvailable = true;
};

// 模型列表结果
struct ModelsResult {
    bool success = false;
    std::string message;
    std::vector<AIModelInfo> models;
};

// 聊天历史记录
struct ChatHistoryEntry {
    std::string id;
    std::string title;
    std::string model;
    int messageCount = 0;
    std::string createdAt;
    std::string updatedAt;
    std::vector<ChatMessage> messages;
};

// 聊天历史查询结果
struct ChatHistoryResult {
    bool success = false;
    std::string message;
    int total = 0;
    std::vector<ChatHistoryEntry> chats;
};

// 删除聊天记录结果
struct DeleteChatResult {
    bool success = false;
    std::string message;
    int deletedCount = 0;
};

// AI 提供商类型
enum class AIProvider {
    OPENAI,
    CLAUDE,
    GEMINI,
    DEEPSEEK,
    QIANWEN,
    QWEN_INTL,
    LOCAL
};

// AI 模型配置
struct AIModelConfig {
    std::string modelName;
    AIProvider provider;
    double temperature;
    int maxTokens;
    double topP;
    int topK;
    bool stream;
    
    AIModelConfig() 
        : modelName("gpt-3.5-turbo"), 
          provider(AIProvider::OPENAI),
          temperature(0.7),
          maxTokens(2048),
          topP(0.9),
          topK(50),
          stream(false) {}
};

// ==================== 服务类 ====================

/**
 * @brief AI 服务实现类
 * 
 * 提供 AI 聊天、语音合成/识别、图像生成/分析等功能
 */
class AIServiceImpl {
public:
    AIServiceImpl();
    AIServiceImpl(const std::string& openaiKey,
                  const std::string& azureKey = "",
                  const std::string& baiduKey = "");
    ~AIServiceImpl();

    // ---- 聊天 ----
    ChatCompletionResult chatCompletion(
        const std::string& token,
        const std::string& message,
        const std::string& model,
        const std::string& chatId,
        const std::vector<ChatMessage>& history,
        double temperature = 0.7,
        int maxTokens = 1000
    );
    
    // ---- 低级别 chat (DTO 版) ----
    Utils::Result<dto::ChatRequest> chat(const dto::ChatRequest& request);

    // ---- 语音 ----
    TTSResult textToSpeech(
        const std::string& token,
        const std::string& text,
        const std::string& voice,
        double speed = 1.0
    );
    
    STTResult speechToText(
        const std::string& token,
        const std::string& audioData,
        const std::string& language
    );
    
    // 简单版本 (返回 Result<string>)
    Utils::Result<std::string> textToSpeech(const std::string& text, const std::string& language);
    Utils::Result<std::string> speechToText(const std::string& audioPath, const std::string& language);

    // ---- 图像 ----
    ImageGenerationResult generateImage(
        const std::string& token,
        const std::string& prompt,
        const std::string& size,
        int n,
        const std::string& style
    );
    
    ImageAnalysisResult analyzeImage(
        const std::string& token,
        const std::string& imageUrl,
        const std::string& imageBase64,
        const std::string& prompt
    );
    
    // 简单版本 (返回 Result<string>)
    Utils::Result<std::string> generateImage(const std::string& prompt, const std::string& style);
    Utils::Result<std::string> analyzeImage(const std::string& imagePath, const std::string& analysisType);

    // ---- 模型管理 ----
    ModelsResult getAvailableModels(const std::string& token);
    
    // ---- 聊天历史 ----
    ChatHistoryResult getChatHistory(
        const std::string& token,
        const std::string& chatId,
        int limit = 50,
        int offset = 0
    );
    
    DeleteChatResult deleteChatHistory(
        const std::string& token,
        const std::string& chatId
    );

private:
    std::string openaiApiKey;
    std::string azureApiKey;
    std::string baiduApiKey;
    std::string requestTimeout;
    std::shared_ptr<spdlog::logger> logger;
    
    // 内部 API 调用
    Utils::Result<std::string> callOpenAIAPI(const std::string& endpoint, const std::string& payload);
    Utils::Result<std::string> callAzureAPI(const std::string& endpoint, const std::string& payload);
    Utils::Result<std::string> callBaiduAPI(const std::string& endpoint, const std::string& payload);
};

} // namespace Services
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::services {
    using Yachiyo::Services::AIServiceImpl;
    using Yachiyo::Services::ChatMessage;
    using Yachiyo::Services::ChatCompletionResult;
}