#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * 语音合成请求 DTO
 */
struct TTSRequest {
    std::string requestId;
    std::string text;
    std::string language = "ja";
    std::string voicePreset = "yachiyou_default";
    int speakerId = 0;
    
    // 感情参数
    std::string emotionType = "calm";
    float emotionIntensity = 0.5;
    float pitchShift = 1.0;
    float speedFactor = 1.0;
    float energyLevel = 0.5;
    
    // 输出配置
    std::string format = "wav";
    int sampleRate = 22050;
    int bitDepth = 16;
    
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        j["text"] = text;
        j["language"] = language;
        j["voice"]["preset"] = voicePreset;
        j["voice"]["speaker_id"] = speakerId;
        j["emotion"]["emotion_type"] = emotionType;
        j["emotion"]["intensity"] = emotionIntensity;
        j["emotion"]["pitch_shift"] = pitchShift;
        j["emotion"]["speed_factor"] = speedFactor;
        j["emotion"]["energy_level"] = energyLevel;
        j["output"]["format"] = format;
        j["output"]["sample_rate"] = sampleRate;
        j["output"]["bit_depth"] = bitDepth;
        return j;
    }
    
    static TTSRequest fromJson(const json& j) {
        TTSRequest req;
        if (j.contains("request_id")) req.requestId = j["request_id"];
        if (j.contains("text")) req.text = j["text"];
        if (j.contains("language")) req.language = j["language"];
        if (j.contains("voice")) {
            if (j["voice"].contains("preset")) req.voicePreset = j["voice"]["preset"];
            if (j["voice"].contains("speaker_id")) req.speakerId = j["voice"]["speaker_id"];
        }
        if (j.contains("emotion")) {
            if (j["emotion"].contains("emotion_type")) req.emotionType = j["emotion"]["emotion_type"];
            if (j["emotion"].contains("intensity")) req.emotionIntensity = j["emotion"]["intensity"];
            if (j["emotion"].contains("pitch_shift")) req.pitchShift = j["emotion"]["pitch_shift"];
            if (j["emotion"].contains("speed_factor")) req.speedFactor = j["emotion"]["speed_factor"];
            if (j["emotion"].contains("energy_level")) req.energyLevel = j["emotion"]["energy_level"];
        }
        if (j.contains("output")) {
            if (j["output"].contains("format")) req.format = j["output"]["format"];
            if (j["output"].contains("sample_rate")) req.sampleRate = j["output"]["sample_rate"];
            if (j["output"].contains("bit_depth")) req.bitDepth = j["output"]["bit_depth"];
        }
        return req;
    }
};

/**
 * 语音合成响应 DTO
 */
struct TTSResponse {
    std::string requestId;
    bool success = false;
    std::string audioUrl;
    std::string audioBase64;
    int durationMs = 0;
    std::string filePath;
    std::string emotionApplied;
    float emotionIntensity = 0.0;
    int processingTimeMs = 0;
    bool cacheHit = false;
    int fileSizeBytes = 0;
    std::string errorMessage;
    
    json toJson() const {
        json j;
        j["request_id"] = requestId;
        j["success"] = success;
        j["audio_url"] = audioUrl;
        j["audio_base64"] = audioBase64;
        j["duration_ms"] = durationMs;
        j["file_path"] = filePath;
        j["emotion_applied"] = emotionApplied;
        j["emotion_intensity"] = emotionIntensity;
        j["processing_time_ms"] = processingTimeMs;
        j["cache_hit"] = cacheHit;
        j["file_size_bytes"] = fileSizeBytes;
        if (!errorMessage.empty()) {
            j["error"] = errorMessage;
        }
        return j;
    }
    
    static TTSResponse fromJson(const json& j) {
        TTSResponse resp;
        if (j.contains("request_id")) resp.requestId = j["request_id"];
        if (j.contains("success")) resp.success = j["success"];
        if (j.contains("audio_url")) resp.audioUrl = j["audio_url"];
        if (j.contains("audio_base64")) resp.audioBase64 = j["audio_base64"];
        if (j.contains("duration_ms")) resp.durationMs = j["duration_ms"];
        if (j.contains("file_path")) resp.filePath = j["file_path"];
        if (j.contains("emotion_applied")) resp.emotionApplied = j["emotion_applied"];
        if (j.contains("emotion_intensity")) resp.emotionIntensity = j["emotion_intensity"];
        if (j.contains("processing_time_ms")) resp.processingTimeMs = j["processing_time_ms"];
        if (j.contains("cache_hit")) resp.cacheHit = j["cache_hit"];
        if (j.contains("file_size_bytes")) resp.fileSizeBytes = j["file_size_bytes"];
        if (j.contains("error")) resp.errorMessage = j["error"];
        return resp;
    }
};
