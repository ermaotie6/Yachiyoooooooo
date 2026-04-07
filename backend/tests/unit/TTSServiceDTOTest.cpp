#include <gtest/gtest.h>
#include "dto/TTSServiceDTO.hpp"

using Yachiyo::DTO::TTSRequest;
using Yachiyo::DTO::TTSResponse;

// ==================== TTSRequest 测试 ====================

TEST(TTSServiceDTOTest, Request_DefaultValues) {
    TTSRequest req;
    EXPECT_EQ(req.language, "ja");
    EXPECT_EQ(req.voicePreset, "yachiyou_default");
    EXPECT_EQ(req.emotionType, "calm");
    EXPECT_FLOAT_EQ(req.emotionIntensity, 0.5f);
    EXPECT_FLOAT_EQ(req.pitchShift, 1.0f);
    EXPECT_FLOAT_EQ(req.speedFactor, 1.0f);
    EXPECT_EQ(req.format, "wav");
    EXPECT_EQ(req.sampleRate, 22050);
}

TEST(TTSServiceDTOTest, Request_ToJson) {
    TTSRequest req;
    req.requestId = "tts_001";
    req.text = "こんにちは";
    req.language = "ja";
    req.emotionType = "happy";
    req.emotionIntensity = 0.8f;
    
    json j = req.toJson();
    
    EXPECT_EQ(j["request_id"], "tts_001");
    EXPECT_EQ(j["text"], "こんにちは");
    EXPECT_EQ(j["language"], "ja");
    EXPECT_EQ(j["emotion"]["emotion_type"], "happy");
    EXPECT_FLOAT_EQ(j["emotion"]["intensity"], 0.8f);
    EXPECT_EQ(j["voice"]["preset"], "yachiyou_default");
}

TEST(TTSServiceDTOTest, Request_FromJson) {
    json j;
    j["request_id"] = "tts_002";
    j["text"] = "测试文本";
    j["language"] = "zh";
    j["voice"]["preset"] = "yachiyou_playful";
    j["voice"]["speaker_id"] = 1;
    j["emotion"]["emotion_type"] = "excited";
    j["emotion"]["intensity"] = 0.9;
    j["output"]["format"] = "mp3";
    j["output"]["sample_rate"] = 44100;
    
    auto req = TTSRequest::fromJson(j);
    
    EXPECT_EQ(req.requestId, "tts_002");
    EXPECT_EQ(req.text, "测试文本");
    EXPECT_EQ(req.language, "zh");
    EXPECT_EQ(req.voicePreset, "yachiyou_playful");
    EXPECT_EQ(req.speakerId, 1);
    EXPECT_EQ(req.emotionType, "excited");
    EXPECT_FLOAT_EQ(req.emotionIntensity, 0.9f);
    EXPECT_EQ(req.format, "mp3");
    EXPECT_EQ(req.sampleRate, 44100);
}

TEST(TTSServiceDTOTest, Request_Roundtrip) {
    TTSRequest original;
    original.requestId = "rt_tts";
    original.text = "往返测试";
    original.emotionType = "sad";
    original.pitchShift = 0.8f;
    original.speedFactor = 0.9f;
    
    json j = original.toJson();
    auto restored = TTSRequest::fromJson(j);
    
    EXPECT_EQ(original.requestId, restored.requestId);
    EXPECT_EQ(original.text, restored.text);
    EXPECT_EQ(original.emotionType, restored.emotionType);
    EXPECT_FLOAT_EQ(original.pitchShift, restored.pitchShift);
    EXPECT_FLOAT_EQ(original.speedFactor, restored.speedFactor);
}

TEST(TTSServiceDTOTest, Request_RefAudioPath) {
    TTSRequest req;
    req.refAudioPath = "reference/yachiyo_happy.wav";
    
    json j = req.toJson();
    EXPECT_TRUE(j.contains("ref_audio_path"));
    EXPECT_EQ(j["ref_audio_path"], "reference/yachiyo_happy.wav");
}

TEST(TTSServiceDTOTest, Request_NoRefAudioPath) {
    TTSRequest req;  // refAudioPath 为空
    
    json j = req.toJson();
    EXPECT_FALSE(j.contains("ref_audio_path"));
}

// ==================== TTSResponse 测试 ====================

TEST(TTSServiceDTOTest, Response_DefaultValues) {
    TTSResponse resp;
    EXPECT_FALSE(resp.success);
    EXPECT_EQ(resp.durationMs, 0);
    EXPECT_EQ(resp.processingTimeMs, 0);
    EXPECT_FALSE(resp.cacheHit);
    EXPECT_EQ(resp.fileSizeBytes, 0);
}

TEST(TTSServiceDTOTest, Response_SuccessfulSynthesis) {
    TTSResponse resp;
    resp.requestId = "tts_resp_001";
    resp.success = true;
    resp.audioUrl = "http://localhost:5000/audio/generated.wav";
    resp.durationMs = 3000;
    resp.emotionApplied = "happy";
    resp.processingTimeMs = 150;
    resp.cacheHit = false;
    
    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.audioUrl, "http://localhost:5000/audio/generated.wav");
    EXPECT_EQ(resp.durationMs, 3000);
}
