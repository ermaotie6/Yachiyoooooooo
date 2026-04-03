#include "services/Live2DAnimationService.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <chrono>

namespace yachiyo::services {

// ==================== 构造/析构 ====================

Live2DAnimationService::Live2DAnimationService() 
    : current_model_(Model::DEFAULT) {
}

Live2DAnimationService::~Live2DAnimationService() {
}

// ==================== 初始化 ====================

bool Live2DAnimationService::initialize() {
    LOG_INFO("初始化 Live2D 动画服务");
    
    // 加载模型配置
    if (!loadModelConfig(Model::DEFAULT)) {
        LOG_ERROR("加载默认模型失败");
        return false;
    }
    
    // 初始化映射表
    initializeEmotionMappings();
    initializeActionMappings();
    
    LOG_INFO("Live2D 动画服务初始化完成");
    return true;
}

// ==================== 生成动画序列 ====================

Utils::Result<dto::Live2DSequenceRequest> Live2DAnimationService::generateAnimationSequence(
    const std::vector<std::string>& emotions,
    const std::vector<std::string>& actions,
    Model model
) {
    LOG_DEBUG("生成动画序列: emotions={}, actions={}", emotions.size(), actions.size());
    
    dto::Live2DSequenceRequest sequence;
    sequence.requestId = "anim_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    
    // 1. 添加表情命令
    if (!emotions.empty()) {
        for (const auto& emotion : emotions) {
            sequence.commands.push_back(mapEmotionToExpression(emotion));
        }
    } else {
        // 默认表情
        sequence.commands.push_back(mapEmotionToExpression("default"));
    }
    
    // 2. 添加动作命令
    if (!actions.empty()) {
        for (const auto& action : actions) {
            sequence.commands.push_back(mapActionToMotion(action));
        }
    }
    
    return Utils::Result<dto::Live2DSequenceRequest>::success(sequence);
}

// ==================== 映射方法 ====================

dto::ExpressionCommand Live2DAnimationService::mapEmotionToExpression(
    const std::string& emotion
) {
    dto::ExpressionCommand cmd;
    
    if (emotion == "开心" || emotion == "happy") {
        cmd.expressionName = "f_smile";
        cmd.durationMs = 2000;
    } else if (emotion == "伤心" || emotion == "sad") {
        cmd.expressionName = "f_sad";
        cmd.durationMs = 2000;
    } else if (emotion == "生气" || emotion == "angry") {
        cmd.expressionName = "f_angry";
        cmd.durationMs = 2000;
    } else if (emotion == "惊讶" || emotion == "surprised") {
        cmd.expressionName = "f_surprised";
        cmd.durationMs = 2000;
    } else {
        cmd.expressionName = "f_default";
        cmd.durationMs = 1000;
    }
    
    return cmd;
}

dto::MotionCommand Live2DAnimationService::mapActionToMotion(
    const std::string& action
) {
    dto::MotionCommand cmd;
    cmd.priority = 1;
    cmd.loop = false;
    
    if (action == "wave" || action == "挥手") {
        cmd.group = "Tap Body";
        cmd.index = 0;
    } else if (action == "nod" || action == "点头") {
        cmd.group = "Nod";
        cmd.index = 0;
    } else if (action == "shake" || action == "摇头") {
        cmd.group = "Shake";
        cmd.index = 0;
    } else if (action == "think" || action == "思考") {
        cmd.group = "Think";
        cmd.index = 0;
    } else {
        cmd.group = "Idle";
        cmd.index = 0;
    }
    
    return cmd;
}

dto::ParameterCommand Live2DAnimationService::setParameter(
    const std::string& paramName,
    float value
) {
    dto::ParameterCommand cmd;
    cmd.paramName = paramName;
    cmd.value = std::max(0.0f, std::min(1.0f, value));
    cmd.transitionMs = 100;
    
    return cmd;
}

// ==================== 嘴部和眼睛同步 ====================

std::vector<dto::ParameterCommand> Live2DAnimationService::generateMouthSyncCommands(
    const std::string& audioPath,
    int durationMs
) {
    std::vector<dto::ParameterCommand> commands;
    
    // TODO: 实现音频分析
    // 当前返回 mock 命令
    
    int stepMs = 100;
    for (int i = 0; i < durationMs; i += stepMs) {
        float progress = static_cast<float>(i) / durationMs;
        float mouthValue = 0.5f + 0.3f * sin(progress * 3.14159f);
        
        commands.push_back(setParameter("ParamMouthOpenY", mouthValue));
    }
    
    return commands;
}

std::vector<dto::ParameterCommand> Live2DAnimationService::generateEyeTrackingCommands(
    float targetX,
    float targetY
) {
    std::vector<dto::ParameterCommand> commands;
    
    // 限制在 [-1, 1] 范围
    float clampedX = std::max(-1.0f, std::min(1.0f, (targetX - 0.5f) * 2.0f));
    float clampedY = std::max(-1.0f, std::min(1.0f, (targetY - 0.5f) * 2.0f));
    
    commands.push_back(setParameter("ParamEyeLOpen", 1.0f));
    commands.push_back(setParameter("ParamEyeROpen", 1.0f));
    commands.push_back(setParameter("ParamEyeLX", clampedX));
    commands.push_back(setParameter("ParamEyeRX", clampedX));
    commands.push_back(setParameter("ParamEyeLY", clampedY));
    commands.push_back(setParameter("ParamEyeRY", clampedY));
    
    return commands;
}

// ==================== 模型管理 ====================

void Live2DAnimationService::switchModel(Model model) {
    std::lock_guard<std::mutex> lock(model_mutex_);
    current_model_ = model;
    LOG_INFO("切换 Live2D 模型");
}

// ==================== 私有方法 ====================

bool Live2DAnimationService::loadModelConfig(Model model) {
    LOG_DEBUG("加载模型配置");
    
    // TODO: 从 resources/live2d/config.json 加载
    // 当前使用 hardcoded 配置
    
    ModelConfig config;
    config.path = "resources/live2d/models/yachiyou_default";
    config.name = "Yachiyo (Default)";
    
    models_[model] = config;
    return true;
}

void Live2DAnimationService::initializeEmotionMappings() {
    // 表情映射配置
    emotion_mappings_["开心"] = {"f_smile", {{"smile", 0.8f}}};
    emotion_mappings_["伤心"] = {"f_sad", {{"sad", 0.8f}}};
    emotion_mappings_["生气"] = {"f_angry", {{"anger", 0.9f}}};
    emotion_mappings_["惊讶"] = {"f_surprised", {{"surprise", 0.8f}}};
    
    LOG_DEBUG("情感映射表已初始化");
}

void Live2DAnimationService::initializeActionMappings() {
    // 动作映射配置
    action_mappings_["挥手"] = {"Tap Body", 0, false};
    action_mappings_["点头"] = {"Nod", 0, false};
    action_mappings_["摇头"] = {"Shake", 0, false};
    action_mappings_["思考"] = {"Think", 0, false};
    
    LOG_DEBUG("动作映射表已初始化");
}

std::vector<float> Live2DAnimationService::analyzeAudioFeatures(
    const std::string& audioPath
) {
    // TODO: 实现音频分析 (FFT, MFCC 等)
    std::vector<float> features(128, 0.0f);  // Mock 返回
    return features;
}

} // namespace yachiyo::services
