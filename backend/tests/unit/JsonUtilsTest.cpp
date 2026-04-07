#include <gtest/gtest.h>
#include "utils/JsonUtils.hpp"

using yachiyo::utils::JsonUtils;

class JsonUtilsTest : public ::testing::Test {};

// ==================== JSON 解析测试 ====================

TEST_F(JsonUtilsTest, Parse_ValidJson) {
    std::string jsonStr = R"({"name": "yachiyo", "version": 1})";
    auto j = JsonUtils::parse(jsonStr);
    EXPECT_EQ(j["name"], "yachiyo");
    EXPECT_EQ(j["version"], 1);
}

TEST_F(JsonUtilsTest, Parse_InvalidJson_Throws) {
    EXPECT_THROW(JsonUtils::parse("{invalid json}"), nlohmann::json::parse_error);
}

TEST_F(JsonUtilsTest, Parse_EmptyObject) {
    auto j = JsonUtils::parse("{}");
    EXPECT_TRUE(j.is_object());
    EXPECT_TRUE(j.empty());
}

TEST_F(JsonUtilsTest, Parse_Array) {
    auto j = JsonUtils::parse("[1, 2, 3]");
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 3u);
}

// ==================== JSON 序列化测试 ====================

TEST_F(JsonUtilsTest, Stringify_Object) {
    json j;
    j["key"] = "value";
    j["num"] = 42;
    
    std::string str = JsonUtils::stringify(j);
    EXPECT_FALSE(str.empty());
    
    // 反序列化验证
    auto parsed = JsonUtils::parse(str);
    EXPECT_EQ(parsed["key"], "value");
    EXPECT_EQ(parsed["num"], 42);
}

TEST_F(JsonUtilsTest, Stringify_PrettyPrint) {
    json j;
    j["key"] = "value";
    
    std::string compact = JsonUtils::stringify(j);
    std::string pretty = JsonUtils::stringify(j, 4);
    
    // Pretty print 应该更长（包含换行和缩进）
    EXPECT_GT(pretty.length(), compact.length());
}

// ==================== 错误响应测试 ====================

TEST_F(JsonUtilsTest, CreateErrorResponse_StatusCode) {
    auto resp = JsonUtils::createErrorResponse(404, "Not Found");
    EXPECT_EQ(resp.code, 404);
}

TEST_F(JsonUtilsTest, CreateErrorResponse_ContainsMessage) {
    auto resp = JsonUtils::createErrorResponse(500, "内部服务器错误");
    // 响应体应该包含错误信息
    EXPECT_NE(resp.body.find("内部服务器错误"), std::string::npos);
}

TEST_F(JsonUtilsTest, CreateErrorResponse_JsonFormat) {
    auto resp = JsonUtils::createErrorResponse(400, "参数错误");
    auto j = json::parse(resp.body);
    EXPECT_FALSE(j["success"]);
    EXPECT_EQ(j["code"], 400);
    EXPECT_EQ(j["message"], "参数错误");
}

// ==================== 成功响应测试 ====================

TEST_F(JsonUtilsTest, CreateSuccessResponse_Default) {
    auto resp = JsonUtils::createSuccessResponse();
    auto j = json::parse(resp.body);
    EXPECT_TRUE(j["success"]);
    EXPECT_EQ(j["code"], 200);
}

TEST_F(JsonUtilsTest, CreateSuccessResponse_WithData) {
    json data;
    data["id"] = 1;
    data["name"] = "test";
    
    auto resp = JsonUtils::createSuccessResponse(data, "查询成功");
    auto j = json::parse(resp.body);
    EXPECT_TRUE(j["success"]);
    EXPECT_EQ(j["data"]["id"], 1);
    EXPECT_EQ(j["data"]["name"], "test");
    EXPECT_EQ(j["message"], "查询成功");
}

// ==================== Unicode/中文测试 ====================

TEST_F(JsonUtilsTest, Parse_ChineseContent) {
    std::string jsonStr = R"({"message": "你好世界", "count": 4})";
    auto j = JsonUtils::parse(jsonStr);
    EXPECT_EQ(j["message"], "你好世界");
}

TEST_F(JsonUtilsTest, Stringify_ChineseContent) {
    json j;
    j["message"] = "测试中文内容";
    
    std::string str = JsonUtils::stringify(j);
    auto parsed = JsonUtils::parse(str);
    EXPECT_EQ(parsed["message"], "测试中文内容");
}

// ==================== 嵌套 JSON 测试 ====================

TEST_F(JsonUtilsTest, Parse_NestedJson) {
    std::string jsonStr = R"({
        "user": {
            "id": 1,
            "profile": {
                "name": "yachiyo",
                "tags": ["ai", "live2d"]
            }
        }
    })";
    
    auto j = JsonUtils::parse(jsonStr);
    EXPECT_EQ(j["user"]["id"], 1);
    EXPECT_EQ(j["user"]["profile"]["name"], "yachiyo");
    EXPECT_EQ(j["user"]["profile"]["tags"].size(), 2u);
    EXPECT_EQ(j["user"]["profile"]["tags"][0], "ai");
}
