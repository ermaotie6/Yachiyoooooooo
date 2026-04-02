#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include "dto/ChatRequest.hpp"
#include "utils/Result.hpp"

namespace Yachiyo {
namespace Services {

// AI 提供商类型
enum class AIProvider {
    OPENAI,         // OpenAI (GPT-3.5, GPT-4 等)
    CLAUDE,         // Anthropic Claude
    GEMINI,         // Google Gemini
    DEEPSEEK,       // DeepSeek 深思
    QIANWEN,        // Alibaba 千问 (阿里云国内版，需要单独API Key)
    QWEN_INTL,      // Alibaba Qwen (国际版 DashScope，需要单独API Key)
    LOCAL           // 本地部署模型 (Ollama 等)
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

// AI 聊天消息
struct AIChatMessage {
    std::string role;  // "system", "user", "assistant"
    std::string content;
    std::string name;  // 可选，参与者的名称
    
    AIChatMessage() = default;
    AIChatMessage(const std::string& role, const std::string& content, const std::string& name = "")
        : role(role), content(content), name(name) {}
};

// AI 聊天请求
struct AIChatRequest {
    std::vector<AIChatMessage> messages;
    AIModelConfig config;
    std::string userId;
    std::string sessionId;
    
    // 添加系统消息
    void addSystemMessage(const std::string& content) {
        messages.emplace_back("system", content);
    }
    
    // 添加用户消息
    void addUserMessage(const std::string& content, const std::string& name = "") {
        messages.emplace_back("user", content, name);
    }
    
    // 添加助手消息
    void addAssistantMessage(const std::string& content, const std::string& name = "") {
        messages.emplace_back("assistant", content, name);
    }
};

// AI 聊天响应
struct AIChatResponse {
    std::string content;
    std::string modelUsed;
    int tokensUsed;
    bool finished;
    std::string finishReason;
    std::string errorMessage;
    
    AIChatResponse() 
        : content(""), 
          modelUsed(""),
          tokensUsed(0),
          finished(true),
          finishReason("stop"),
          errorMessage("") {}
    
    bool hasError() const {
        return !errorMessage.empty();
    }
};

// AI 流式响应回调
using AIStreamCallback = std::function<void(const std::string& chunk, bool finished)>;

/**
 * @brief AI 服务接口
 */
class AIService {
public:
    virtual ~AIService() = default;

    /**
     * @brief 初始化 AI 服务
     * @param config AI 模型配置
     * @return 初始化结果
     */
    virtual Utils::Result<void> initialize(const AIModelConfig& config) = 0;

    /**
     * @brief 聊天（同步）
     * @param request 聊天请求
     * @return 聊天响应
     */
    virtual Utils::Result<AIChatResponse> chat(const AIChatRequest& request) = 0;

    /**
     * @brief 聊天（流式）
     * @param request 聊天请求
     * @param callback 流式回调
     * @return 操作结果
     */
    virtual Utils::Result<void> chatStream(const AIChatRequest& request, AIStreamCallback callback) = 0;

    /**
     * @brief 文本转语音
     * @param text 要转换的文本
     * @param voice 语音配置
     * @return 音频数据
     */
    virtual Utils::Result<std::vector<uint8_t>> textToSpeech(const std::string& text, const std::string& voice = "alloy") = 0;

    /**
     * @brief 语音转文本
     * @param audioData 音频数据
     * @param language 语言代码
     * @return 转换后的文本
     */
    virtual Utils::Result<std::string> speechToText(const std::vector<uint8_t>& audioData, const std::string& language = "zh-CN") = 0;

    /**
     * @brief 生成图像
     * @param prompt 提示词
     * @param size 图像尺寸
     * @param count 生成数量
     * @return 图像 URL 或数据
     */
    virtual Utils::Result<std::vector<std::string>> generateImage(
        const std::string& prompt, 
        const std::string& size = "1024x1024", 
        int count = 1) = 0;

    /**
     * @brief 获取支持的模型列表
     * @return 模型列表
     */
    virtual std::vector<std::string> getSupportedModels() const = 0;

    /**
     * @brief 获取当前配置
     * @return AI 模型配置
     */
    virtual AIModelConfig getCurrentConfig() const = 0;

    /**
     * @brief 设置 API 密钥
     * @param apiKey API 密钥
     */
    virtual void setApiKey(const std::string& apiKey) = 0;

    /**
     * @brief 验证 API 密钥
     * @return 验证结果
     */
    virtual Utils::Result<void> validateApiKey() = 0;
};

/**
 * @brief AI 服务实现类
 */
class AIServiceImpl : public AIService {
public:
    AIServiceImpl();
    ~AIServiceImpl() override;

    Utils::Result<void> initialize(const AIModelConfig& config) override;
    Utils::Result<AIChatResponse> chat(const AIChatRequest& request) override;
    Utils::Result<void> chatStream(const AIChatRequest& request, AIStreamCallback callback) override;
    Utils::Result<std::vector<uint8_t>> textToSpeech(const std::string& text, const std::string& voice) override;
    Utils::Result<std::string> speechToText(const std::vector<uint8_t>& audioData, const std::string& language) override;
    Utils::Result<std::vector<std::string>> generateImage(
        const std::string& prompt, 
        const std::string& size, 
        int count) override;
    
    std::vector<std::string> getSupportedModels() const override;
    AIModelConfig getCurrentConfig() const override;
    void setApiKey(const std::string& apiKey) override;
    Utils::Result<void> validateApiKey() override;

private:
    AIModelConfig currentConfig;
    std::string apiKey;
    bool initialized;
    
    // 模拟 AI 响应（实际项目中应该调用真实的 AI API）
    AIChatResponse simulateAIResponse(const AIChatRequest& request);
    
    // 模拟流式响应
    void simulateStreamResponse(const AIChatRequest& request, AIStreamCallback callback);
};

} // namespace Services
} // namespace Yachiyo