#include <gtest/gtest.h>
#include "utils/Result.hpp"

using Yachiyo::Utils::Result;

// ==================== Result<T> 基本功能 ====================

TEST(ResultTest, Success_WithData) {
    auto result = Result<int>::success(42);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.getValue(), 42);
    EXPECT_TRUE(result.getData().has_value());
}

TEST(ResultTest, Success_WithMessage) {
    auto result = Result<std::string>::success("hello", "操作成功");
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.getValue(), "hello");
    EXPECT_EQ(result.getMessage(), "操作成功");
}

TEST(ResultTest, Error_SingleParam) {
    auto result = Result<int>::error("出错了");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.getMessage(), "出错了");
    EXPECT_FALSE(result.getData().has_value());
}

TEST(ResultTest, Error_WithCode) {
    auto result = Result<int>::error("404", "未找到");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.getCode(), "404");
    EXPECT_EQ(result.getMessage(), "未找到");
}

TEST(ResultTest, GetValue_ThrowsOnError) {
    auto result = Result<int>::error("错误");
    EXPECT_THROW(result.getValue(), std::runtime_error);
}

// ==================== Result<void> ====================

TEST(ResultTest, VoidSuccess) {
    auto result = Result<void>::Success();
    EXPECT_TRUE(result.isSuccess());
}

TEST(ResultTest, VoidError) {
    auto result = Result<void>::error("操作失败");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.getMessage(), "操作失败");
}

// ==================== ErrorInfo ====================

TEST(ResultTest, GetError_ReturnsErrorInfo) {
    auto result = Result<int>::error("ERR_CODE", "详细错误信息");
    auto errInfo = result.getError();
    EXPECT_EQ(errInfo.code, "ERR_CODE");
    EXPECT_EQ(errInfo.message, "详细错误信息");
}

// ==================== fail() 方法 ====================

TEST(ResultTest, Fail_WithDefaultData) {
    auto result = Result<std::string>::fail(1001, "服务出错", "fallback");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.getCode(), "1001");
    EXPECT_TRUE(result.getData().has_value());
    EXPECT_EQ(result.getValue(), "fallback");
}

TEST(ResultTest, Fail_WithoutData) {
    auto result = Result<int>::fail(500, "内部错误");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_FALSE(result.getData().has_value());
}

// ==================== 别名兼容性 ====================

TEST(ResultTest, Success_UpperCase) {
    auto result = Result<int>::Success(100);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.getValue(), 100);
}

TEST(ResultTest, Error_UpperCase) {
    auto result = Result<int>::Error("大写错误");
    EXPECT_FALSE(result.isSuccess());
}

// ==================== JSON 序列化 ====================

TEST(ResultTest, ToJson_Success) {
    auto result = Result<std::string>::success("data_value");
    auto j = result.toJson();
    EXPECT_TRUE(j["success"]);
    EXPECT_EQ(j["data"], "data_value");
}

TEST(ResultTest, ToJson_Error) {
    auto result = Result<int>::error("500", "服务器错误");
    auto j = result.toJson();
    EXPECT_FALSE(j["success"]);
    EXPECT_EQ(j["code"], "500");
}

// ==================== Setters ====================

TEST(ResultTest, Setters) {
    Result<int> result;
    result.setSuccess(true);
    result.setCode("200");
    result.setMessage("ok");
    result.setData(123);
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.getCode(), "200");
    EXPECT_EQ(result.getMessage(), "ok");
    EXPECT_EQ(result.getValue(), 123);
}

// ==================== 复杂类型 ====================

TEST(ResultTest, VectorResult) {
    std::vector<std::string> data = {"a", "b", "c"};
    auto result = Result<std::vector<std::string>>::success(data);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.getValue().size(), 3u);
}
