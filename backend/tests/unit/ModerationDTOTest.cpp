#include <gtest/gtest.h>
#include "dto/ModerationDTO.hpp"

using Yachiyo::DTO::ModerationRequest;
using Yachiyo::DTO::ModerationResult;
using Yachiyo::DTO::ModerationResponse;

// ==================== ModerationRequest 测试 ====================

TEST(ModerationDTOTest, Request_ToJson) {
    ModerationRequest req;
    req.requestId = "mod_001";
    req.content = "测试内容";
    req.emotionTags = {"happy", "excited"};
    req.language = "zh";
    req.threshold = 0.7f;
    
    json j = req.toJson();
    
    EXPECT_EQ(j["request_id"], "mod_001");
    EXPECT_EQ(j["content"], "测试内容");
    EXPECT_EQ(j["emotion_tags"].size(), 2u);
    EXPECT_EQ(j["language"], "zh");
    EXPECT_FLOAT_EQ(j["threshold"], 0.7f);
}

TEST(ModerationDTOTest, Request_FromJson) {
    json j;
    j["request_id"] = "mod_002";
    j["content"] = "待审核内容";
    j["emotion_tags"] = {"calm"};
    j["language"] = "en";
    j["threshold"] = 0.3;
    
    auto req = ModerationRequest::fromJson(j);
    
    EXPECT_EQ(req.requestId, "mod_002");
    EXPECT_EQ(req.content, "待审核内容");
    EXPECT_EQ(req.emotionTags.size(), 1u);
    EXPECT_EQ(req.language, "en");
    EXPECT_FLOAT_EQ(req.threshold, 0.3f);
}

TEST(ModerationDTOTest, Request_DefaultValues) {
    ModerationRequest req;
    EXPECT_EQ(req.language, "zh");
    EXPECT_FLOAT_EQ(req.threshold, 0.5f);
    EXPECT_TRUE(req.emotionTags.empty());
}

TEST(ModerationDTOTest, Request_Roundtrip) {
    ModerationRequest original;
    original.requestId = "rt_mod";
    original.content = "往返测试";
    original.emotionTags = {"neutral", "curious"};
    original.threshold = 0.6f;
    
    json j = original.toJson();
    auto restored = ModerationRequest::fromJson(j);
    
    EXPECT_EQ(original.requestId, restored.requestId);
    EXPECT_EQ(original.content, restored.content);
    EXPECT_EQ(original.emotionTags, restored.emotionTags);
    EXPECT_FLOAT_EQ(original.threshold, restored.threshold);
}

// ==================== ModerationResult 测试 ====================

TEST(ModerationDTOTest, Result_ToJson) {
    ModerationResult result;
    result.category = "violence";
    result.riskScore = 0.8f;
    result.verdict = "block";
    result.reason = "包含暴力内容";
    
    json j = result.toJson();
    
    EXPECT_EQ(j["category"], "violence");
    EXPECT_FLOAT_EQ(j["risk_score"], 0.8f);
    EXPECT_EQ(j["verdict"], "block");
    EXPECT_EQ(j["reason"], "包含暴力内容");
}

TEST(ModerationDTOTest, Result_DefaultValues) {
    ModerationResult result;
    EXPECT_FLOAT_EQ(result.riskScore, 0.0f);
    EXPECT_TRUE(result.category.empty());
    EXPECT_TRUE(result.verdict.empty());
}

// ==================== ModerationResponse 测试 ====================

TEST(ModerationDTOTest, Response_ToJson) {
    ModerationResponse resp;
    resp.requestId = "mod_resp_001";
    resp.success = true;
    resp.content = "审核后的内容";
    resp.overallVerdict = "pass";
    resp.overallRiskScore = 0.1f;
    resp.processingTimeMs = 50;
    
    ModerationResult mr;
    mr.category = "spam";
    mr.riskScore = 0.1f;
    mr.verdict = "pass";
    mr.reason = "低风险";
    resp.results.push_back(mr);
    
    json j = resp.toJson();
    
    EXPECT_EQ(j["request_id"], "mod_resp_001");
    EXPECT_TRUE(j["success"]);
    EXPECT_EQ(j["overall_verdict"], "pass");
    EXPECT_FLOAT_EQ(j["overall_risk_score"], 0.1f);
    EXPECT_EQ(j["results"].size(), 1u);
    EXPECT_EQ(j["results"][0]["category"], "spam");
}

TEST(ModerationDTOTest, Response_DefaultValues) {
    ModerationResponse resp;
    EXPECT_EQ(resp.overallVerdict, "pass");
    EXPECT_FLOAT_EQ(resp.overallRiskScore, 0.0f);
    EXPECT_FALSE(resp.success);
    EXPECT_TRUE(resp.results.empty());
}
