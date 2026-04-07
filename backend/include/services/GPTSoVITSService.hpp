#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include "utils/Result.hpp"
#include "utils/Compat.hpp"
#include "dto/TTSServiceDTO.hpp"

namespace yachiyo::services {

/**
 * GPT-SoVITS 语音合成服务
 * 
 * 职责:
 * - 接收文本和情感参数
 * - 调用 GPT-SoVITS 生成语音
 * - 支持多种声音预设
 * - 管理音频缓存
 */
class GPTSoVITSService {
public:
    enum class VoicePreset {
        DEFAULT,   // yachiyou_default (温暖)
        PLAYFUL,   // yachiyou_playful (调皮)
        COOL       // yachiyou_cool (冷淡)
    };
    
    enum class InferenceMode {
        GPU,       // GPU 推理 (低延迟, timeout ~15s)
        CPU        // CPU 推理 (高延迟, timeout ~60s)
    };
    
    GPTSoVITSService();
    ~GPTSoVITSService();
    
    /**
     * 初始化服务 (连接到 GPT-SoVITS 端点)
     * @param endpoint GPT-SoVITS API 端点 (例: http://localhost:5000)
     * @param mode 推理模式 (GPU 或 CPU, 影响超时策略)
     */
    bool initialize(const std::string& endpoint = "", InferenceMode mode = InferenceMode::CPU);
    
    /**
     * 合成语音
     * @param request 合成请求 (包含文本、情感、声音等参数)
     * @return 合成结果 (包含音频 URL 和 Base64 编码)
     */
    Utils::Result<dto::TTSResponse> synthesize(const dto::TTSRequest& request);
    
    /**
     * 从 OpenClaw 响应直接合成语音
     * @param text 要合成的文本
     * @param emotionType 情感类型
     * @param voicePreset 声音预设
     * @return 合成结果
     */
    Utils::Result<dto::TTSResponse> synthesizeWithEmotion(
        const std::string& text,
        const std::string& emotionType,
        VoicePreset voicePreset = VoicePreset::DEFAULT
    );
    
    /**
     * 批量合成
     * @param requests 请求列表
     * @return 结果列表
     */
    std::vector<Utils::Result<dto::TTSResponse>> batchSynthesize(
        const std::vector<dto::TTSRequest>& requests
    );
    
    /**
     * 注册参考音频 (用于声音克隆)
     * @param audioPath 参考音频文件路径
     * @return 成功/失败
     */
    bool registerReferenceAudio(const std::string& audioPath);
    
    /**
     * 克隆新声音
     * @param referenceAudioPath 参考音频路径
     * @param outputModelPath 输出模型路径
     * @return 成功/失败
     */
    bool cloneVoice(
        const std::string& referenceAudioPath,
        const std::string& outputModelPath
    );
    
    /**
     * 清空缓存
     */
    void clearCache();
    
    /**
     * 获取服务状态
     */
    bool isHealthy() const;

private:
    struct CacheEntry {
        dto::TTSResponse response;
        int64_t timestamp;
    };
    
    struct VoiceConfig {
        std::string modelPath;
        std::string referenceAudio;
        float pitchMin, pitchMax;
        float speedMin, speedMax;
    };
    
    std::string endpoint_;
    InferenceMode inference_mode_;
    long timeout_seconds_;
    std::map<std::string, CacheEntry> cache_;
    std::mutex cache_mutex_;
    std::map<VoicePreset, VoiceConfig> voice_configs_;
    
    // 情感 → 参考音频映射
    std::map<std::string, std::string> emotion_ref_audio_map_;
    
    /**
     * 获取情感对应的参考音频路径
     */
    std::string getRefAudioForEmotion(const std::string& emotionType) const;
    
    /**
     * 生成缓存键
     */
    std::string generateCacheKey(const dto::TTSRequest& request) const;
    
    /**
     * 将情感类型映射到参数
     */
    void mapEmotionToParameters(
        const std::string& emotionType,
        float& pitchShift,
        float& speedFactor,
        float& energyLevel
    ) const;
};

} // namespace yachiyo::services
