#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <optional>

namespace Yachiyo {
namespace Utils {

/**
 * @brief 统一响应结果模板类
 */
template<typename T>
class Result {
public:
    Result() : success(false), code(""), message("") {}
    
    Result(bool success, const std::string& code, const std::string& message, 
           const std::optional<T>& data = std::nullopt)
        : success(success), code(code), message(message), data(data) {}

    // 成功响应
    static Result<T> successResult(const T& data, const std::string& message = "操作成功") {
        return Result(true, "200", message, data);
    }

    static Result<T> successResult(const std::string& message = "操作成功") {
        return Result(true, "200", message);
    }

    // 错误响应
    static Result<T> errorResult(const std::string& code, const std::string& message) {
        return Result(false, code, message);
    }

    // Getters
    bool isSuccess() const { return success; }
    const std::string& getCode() const { return code; }
    const std::string& getMessage() const { return message; }
    const std::optional<T>& getData() const { return data; }

    // Setters
    void setSuccess(bool value) { success = value; }
    void setCode(const std::string& value) { code = value; }
    void setMessage(const std::string& value) { message = value; }
    void setData(const T& value) { data = value; }

    // JSON 序列化
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["success"] = success;
        j["code"] = code;
        j["message"] = message;
        if (data.has_value()) {
            j["data"] = data.value();
        }
        return j;
    }

    // 转换为字符串
    std::string toString() const {
        return toJson().dump();
    }

private:
    bool success;
    std::string code;
    std::string message;
    std::optional<T> data;
};

// 特化 void 类型的 Result
template<>
class Result<void> {
public:
    Result() : success(false), code(""), message("") {}
    
    Result(bool success, const std::string& code, const std::string& message)
        : success(success), code(code), message(message) {}

    // 成功响应
    static Result<void> successResult(const std::string& message = "操作成功") {
        return Result(true, "200", message);
    }

    // 错误响应
    static Result<void> errorResult(const std::string& code, const std::string& message) {
        return Result(false, code, message);
    }

    // Getters
    bool isSuccess() const { return success; }
    const std::string& getCode() const { return code; }
    const std::string& getMessage() const { return message; }

    // Setters
    void setSuccess(bool value) { success = value; }
    void setCode(const std::string& value) { code = value; }
    void setMessage(const std::string& value) { message = value; }

    // JSON 序列化
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["success"] = success;
        j["code"] = code;
        j["message"] = message;
        return j;
    }

    // 转换为字符串
    std::string toString() const {
        return toJson().dump();
    }

private:
    bool success;
    std::string code;
    std::string message;
};

} // namespace Utils
} // namespace Yachiyo