#include "services/GPTSoVITSService.hpp"
#include "utils/Logger.hpp"
#include "utils/JsonUtils.hpp"
#include <curl/curl.h>
#include <chrono>

namespace yachiyo::services {

// ==================== 辅助函数 ====================

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ==================== 构造/析构 ====================

GPTSoVITSService::GPTSoVITSService() 
    : endpoint_("") {
}

GPTSoVITSService::~GPTSoVITSService() {
    clearCache();
}

// ==================== 初始化 ====================

bool GPTSoVITSService::initialize(const std::string& endpoint) {
    LOG_INFO("初始化 GPT-SoVITS 服务: endpoint={}", endpoint);
    
    endpoint_ = endpoint;
    
    // 配置预设声音
    voice_configs_[VoicePreset::DEFAULT] = {
        "resources/voice_models/yachiyou_default.pth",
        "resources/voice_samples/yachiyou_default.wav",
        50.0, 300.0,   // 音调范围
        0.5, 2.0       // 速度范围
    };
    
    voice_configs_[VoicePreset::PLAYFUL] = {
        "resources/voice_models/yachiyou_playful.pth",
        "resources/voice_samples/yachiyou_playful.wav",
        80.0, 350.0,
        0.6, 1.8
    };
    
    voice_configs_[VoicePreset::COOL] = {
        "resources/voice_models/yachiyou_cool.pth",
        "resources/voice_samples/yachiyou_cool.wav",
        40.0, 200.0,
        0.7, 1.5
    };
    
    LOG_INFO("GPT-SoVITS 服务初始化完成");
    return true;
}

// ==================== 合成语音 ====================

Utils::Result<dto::TTSResponse> GPTSoVITSService::synthesize(const dto::TTSRequest& request) {
    auto startTime = std::chrono::steady_clock::now();
    
    // 生成缓存键
    std::string cacheKey = generateCacheKey(request);
    
    // 检查缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(cacheKey);
        if (it != cache_.end()) {
            LOG_DEBUG("使用缓存的 TTS 响应");
            return Utils::Result<dto::TTSResponse>::success(it->second.response);
        }
    }
    
    dto::TTSResponse response;
    response.requestId = request.requestId;
    response.emotionApplied = request.emotionType;
    response.emotionIntensity = request.emotionIntensity;
    
    if (!endpoint_.empty()) {
        // 真实 API 调用
        CURL* curl = curl_easy_init();
        if (!curl) {
            return Utils::Result<dto::TTSResponse>::error("CURL_INIT_ERROR", "CURL 初始化失败");
        }
        
        nlohmann::json reqJson;
        reqJson["text"] = request.text;
        reqJson["emotion"] = request.emotionType;
        reqJson["voice_preset"] = request.voicePreset;
        reqJson["pitch_shift"] = request.pitchShift;
        reqJson["speed_factor"] = request.speedFactor;
        reqJson["energy_level"] = request.energyLevel;
        std::string postData = reqJson.dump();
        
        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, (endpoint_ + "/synthesize").c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            LOG_ERROR("GPT-SoVITS 请求失败: {}", curl_easy_strerror(res));
            return Utils::Result<dto::TTSResponse>::error("REQUEST_ERROR", curl_easy_strerror(res));
        }
        
        try {
            auto respJson = nlohmann::json::parse(readBuffer);
            response.success = respJson.value("success", true);
            response.audioUrl = respJson.value("audio_url", "");
            response.durationMs = respJson.value("duration_ms", 0);
        } catch (const std::exception& e) {
            LOG_ERROR("GPT-SoVITS 响应解析失败: {}", e.what());
            return Utils::Result<dto::TTSResponse>::error("PARSE_ERROR", e.what());
        }
    } else {
        // Mock 回退（未配置 endpoint）
        LOG_WARN("GPT-SoVITS endpoint 未配置，使用 mock 响应");
        response.success = true;
        response.audioUrl = "http://localhost:8000/audio/generated_" + request.requestId + ".wav";
        response.durationMs = request.text.length() * 50;
    }
    
    response.processingTimeMs = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime
        ).count();
    response.cacheHit = false;
    
    // 保存到缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_[cacheKey] = {response, std::chrono::system_clock::now().time_since_epoch().count()};
    }
    
    return Utils::Result<dto::TTSResponse>::success(response);
}

// ==================== 带情感的合成 ====================

Utils::Result<dto::TTSResponse> GPTSoVITSService::synthesizeWithEmotion(
    const std::string& text,
    const std::string& emotionType,
    VoicePreset voicePreset
) {
    dto::TTSRequest request;
    request.requestId = "tts_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    request.text = text;
    request.emotionType = emotionType;
    request.voicePreset = "yachiyou_" + std::to_string(static_cast<int>(voicePreset));
    
    // 映射情感到参数
    mapEmotionToParameters(
        emotionType,
        request.pitchShift,
        request.speedFactor,
        request.energyLevel
    );
    
    return synthesize(request);
}

// ==================== 批量合成 ====================

std::vector<Utils::Result<dto::TTSResponse>> GPTSoVITSService::batchSynthesize(
    const std::vector<dto::TTSRequest>& requests
) {
    std::vector<Utils::Result<dto::TTSResponse>> results;
    
    for (const auto& request : requests) {
        results.push_back(synthesize(request));
    }
    
    return results;
}

// ==================== 声音操作 ====================

bool GPTSoVITSService::registerReferenceAudio(const std::string& audioPath) {
    LOG_INFO("注册参考音频: {}", audioPath);
    // TODO: 验证音频文件
    return true;
}

bool GPTSoVITSService::cloneVoice(
    const std::string& referenceAudioPath,
    const std::string& outputModelPath
) {
    LOG_INFO("克隆声音: {} -> {}", referenceAudioPath, outputModelPath);
    // TODO: 调用 GPT-SoVITS 克隆 API
    return true;
}

// ==================== 缓存管理 ====================

void GPTSoVITSService::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
}

bool GPTSoVITSService::isHealthy() const {
    // TODO: 执行健康检查
    return true;
}

// ==================== 私有方法 ====================

std::string GPTSoVITSService::generateCacheKey(const dto::TTSRequest& request) const {
    return "tts_" + std::to_string(
        std::hash<std::string>{}(
            request.text + "_" + request.emotionType + "_" + request.voicePreset
        )
    );
}

void GPTSoVITSService::mapEmotionToParameters(
    const std::string& emotionType,
    float& pitchShift,
    float& speedFactor,
    float& energyLevel
) const {
    // 映射情感到 TTS 参数
    if (emotionType == "happy") {
        pitchShift = 1.2f;
        speedFactor = 1.1f;
        energyLevel = 0.9f;
    } else if (emotionType == "sad") {
        pitchShift = 0.8f;
        speedFactor = 0.8f;
        energyLevel = 0.4f;
    } else if (emotionType == "angry") {
        pitchShift = 1.1f;
        speedFactor = 1.0f;
        energyLevel = 0.95f;
    } else if (emotionType == "excited") {
        pitchShift = 1.3f;
        speedFactor = 1.2f;
        energyLevel = 1.0f;
    } else {
        // 默认 (calm)
        pitchShift = 1.0f;
        speedFactor = 1.0f;
        energyLevel = 0.5f;
    }
}

} // namespace yachiyo::services
