#include <gtest/gtest.h>
#include "dto/OpenClawDTO.hpp"

using Yachiyo::DTO::OpenClawRequest;
using Yachiyo::DTO::OpenClawResponse;

// ==================== OpenClawRequest 序列化测试 ====================

TEST(OpenClawDTOTest, Request_ToJson) {
    OpenClawRequest req;
    req.requestId = "req_001";
    req.text = "你好";
    req.userId = "user_42";
    req.context = "previous context";
    req.maxTokens = 500;
    req.temperature = 0.8f;
    req.emotionHints = {"happy", "excited"};
    
    json j = req.toJson();
    
    EXPECT_EQ(j["request_id"], "req_001");
    EXPECT_EQ(j["text"], "你好");
    EXPECT_EQ(j["user_id"], "user_42");
    EXPECT_EQ(j["context"], "previous context");
    EXPECT_EQ(j["max_tokens"], 500);
    EXPECT_FLOAT_EQ(j["temperature"], 0.8f);
    EXPECT_EQ(j["emotion_hints"].size(), 2u);
    EXPECT_EQ(j["emotion_hints"][0], "happy");
}

TEST(OpenClawDTOTest, Request_FromJson) {
    json j;
    j["request_id"] = "req_002";
    j["text"] = "今天天气如何";
    j["user_id"] = "user_99";
    j["max_tokens"] = 2000;
    j["temperature"] = 0.5;
    j["emotion_hints"] = {"calm"};
    
    auto req = OpenClawRequest::fromJson(j);
    
    EXPECT_EQ(req.requestId, "req_002");
    EXPECT_EQ(req.text, "今天天气如何");
    EXPECT_EQ(req.userId, "user_99");
    EXPECT_EQ(req.maxTokens, 2000);
    EXPECT_FLOAT_EQ(req.temperature, 0.5f);
    EXPECT_EQ(req.emotionHints.size(), 1u);
}

TEST(OpenClawDTOTest, Request_FromJson_Partial) {
    // 只有部分字段
    json j;
    j["text"] = "hello";
    
    auto req = OpenClawRequest::fromJson(j);
    EXPECT_EQ(req.text, "hello");
    EXPECT_TRUE(req.requestId.empty());
    EXPECT_EQ(req.maxTokens, 1000);  // 默认值
}

TEST(OpenClawDTOTest, Request_Roundtrip) {
    OpenClawRequest original;
    original.requestId = "roundtrip_test";
    original.text = "测试往返序列化";
    original.userId = "user_1";
    original.emotionHints = {"neutral", "curious"};
    
    json j = original.toJson();
    auto restored = OpenClawRequest::fromJson(j);
    
    EXPECT_EQ(original.requestId, restored.requestId);
    EXPECT_EQ(original.text, restored.text);
    EXPECT_EQ(original.userId, restored.userId);
    EXPECT_EQ(original.emotionHints, restored.emotionHints);
}

// ==================== OpenClawResponse 序列化测试 ====================

TEST(OpenClawDTOTest, Response_ToJson) {
    OpenClawResponse resp;
    resp.requestId = "req_001";
    resp.success = true;
    resp.text = "你好！很高兴见到你。";
    resp.emotions = {"happy"};
    resp.actions = {"wave", "smile"};
    resp.processingTimeMs = 150;
    
    json j = resp.toJson();
    
    EXPECT_EQ(j["request_id"], "req_001");
    EXPECT_TRUE(j["success"]);
    EXPECT_EQ(j["text"], "你好！很高兴见到你。");
    EXPECT_EQ(j["emotions"].size(), 1u);
    EXPECT_EQ(j["actions"].size(), 2u);
    EXPECT_EQ(j["processing_time_ms"], 150);
    EXPECT_FALSE(j.contains("error"));  // 没有错误时不应包含 error 字段
}

TEST(OpenClawDTOTest, Response_FromJson) {
    json j;
    j["request_id"] = "req_003";
    j["success"] = true;
    j["text"] = "AI 回复内容";
    j["emotions"] = {"sad", "thoughtful"};
    j["actions"] = {"nod"};
    j["processing_time_ms"] = 200;
    
    auto resp = OpenClawResponse::fromJson(j);
    
    EXPECT_EQ(resp.requestId, "req_003");
    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.text, "AI 回复内容");
    EXPECT_EQ(resp.emotions.size(), 2u);
    EXPECT_EQ(resp.actions.size(), 1u);
    EXPECT_EQ(resp.processingTimeMs, 200);
}

TEST(OpenClawDTOTest, Response_WithError) {
    OpenClawResponse resp;
    resp.success = false;
    resp.errorMessage = "OpenClaw 服务不可用";
    
    json j = resp.toJson();
    EXPECT_FALSE(j["success"]);
    EXPECT_EQ(j["error"], "OpenClaw 服务不可用");
}

TEST(OpenClawDTOTest, Response_Roundtrip) {
    OpenClawResponse original;
    original.requestId = "rt_resp";
    original.success = true;
    original.text = "往返测试回复";
    original.emotions = {"neutral"};
    original.actions = {"idle"};
    original.processingTimeMs = 100;
    
    json j = original.toJson();
    auto restored = OpenClawResponse::fromJson(j);
    
    EXPECT_EQ(original.requestId, restored.requestId);
    EXPECT_EQ(original.success, restored.success);
    EXPECT_EQ(original.text, restored.text);
    EXPECT_EQ(original.emotions, restored.emotions);
    EXPECT_EQ(original.actions, restored.actions);
    EXPECT_EQ(original.processingTimeMs, restored.processingTimeMs);
}
