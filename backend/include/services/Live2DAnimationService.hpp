#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "utils/Result.hpp"
#include "dto/Live2DDTO.hpp"

using json = nlohmann::json;

namespace yachiyo::services {

/**
 * Live2D 虚拱动画服务
 * 
 * 职责:
 * - 从 OpenClaw 情感和动作生成动画命令
 * - 管理多个虚拱模型
 * - 处理表情和肢体动作映射
 * - 生成动画序列
 */
class Live2DAnimationService {
public:
    enum class Model {
        DEFAULT,    // yachiyou_default
        PLAYFUL,    // yachiyou_playful
        COOL        // yachiyou_cool
    };
    
    Live2DAnimationService();
    ~Live2DAnimationService();
    
    /**
     * 初始化服务 (加载模型配置)
     */
    bool initialize();
    
    /**
     * 从情感和动作生成动画命令序列
     * @param emotions 情感标签列表
     * @param actions 动作命令列表
     * @param model 使用的虚拱模型
     * @return 动画命令序列
     */
    Utils::Result<dto::Live2DSequenceRequest> generateAnimationSequence(
        const std::vector<std::string>& emotions,
        const std::vector<std::string>& actions,
        Model model = Model::DEFAULT
    );
    
    /**
     * 将情感映射到表情
     * @param emotion 情感标签
     * @return 表情命令
     */
    dto::ExpressionCommand mapEmotionToExpression(const std::string& emotion);
    
    /**
     * 将动作映射到 Live2D 动作命令
     * @param action 动作标签
     * @return 动作命令
     */
    dto::MotionCommand mapActionToMotion(const std::string& action);
    
    /**
     * 设置参数 (用于高级动画控制)
     * @param paramName 参数名 (例: ParamMouthOpenY)
     * @param value 参数值 [0.0-1.0]
     * @return 参数命令
     */
    dto::ParameterCommand setParameter(
        const std::string& paramName,
        float value
    );
    
    /**
     * 生成嘴部同步命令 (基于音频)
     * @param audioPath 音频文件路径
     * @param duration 音频时长 (ms)
     * @return 嘴部动作命令序列
     */
    std::vector<dto::ParameterCommand> generateMouthSyncCommands(
        const std::string& audioPath,
        int durationMs
    );
    
    /**
     * 生成眼睛跟踪命令
     * @param targetX 目标 X 坐标 [0.0-1.0]
     * @param targetY 目标 Y 坐标 [0.0-1.0]
     * @return 眼睛参数命令列表
     */
    std::vector<dto::ParameterCommand> generateEyeTrackingCommands(
        float targetX,
        float targetY
    );
    
    /**
     * 切换虚拱模型
     * @param model 新模型
     */
    void switchModel(Model model);
    
    /**
     * 获取当前模型
     */
    Model getCurrentModel() const { return current_model_; }

private:
    struct ModelConfig {
        std::string path;
        std::string name;
        json config;
    };
    
    struct EmotionMapping {
        std::string expressionName;
        std::vector<std::pair<std::string, float>> parameters;
    };
    
    struct ActionMapping {
        std::string motionGroup;
        int motionIndex;
        bool loop;
    };
    
    std::map<Model, ModelConfig> models_;
    std::map<std::string, EmotionMapping> emotion_mappings_;
    std::map<std::string, ActionMapping> action_mappings_;
    Model current_model_;
    std::mutex model_mutex_;
    
    /**
     * 加载模型配置
     */
    bool loadModelConfig(Model model);
    
    /**
     * 初始化情感映射表
     */
    void initializeEmotionMappings();
    
    /**
     * 初始化动作映射表
     */
    void initializeActionMappings();
    
    /**
     * 解析音频文件获取特征
     */
    std::vector<float> analyzeAudioFeatures(const std::string& audioPath);
};

} // namespace yachiyo::services
