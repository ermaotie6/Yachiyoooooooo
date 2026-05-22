#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <optional>

namespace Yachiyo {
namespace Utils {

/**
 * @brief 错误信息结构体 (用于 getError() 返回)
 */
struct ErrorInfo {
    std::string code;
    std::string message;
    
    ErrorInfo() = default;
    ErrorInfo(const std::string& code, const std::string& message) 
        : code(code), message(message) {}
};

/**
 * @brief 统一响应结果模板类
 */
template<typename T>
class Result {
public:
    Result() : success_(false), code(""), message("") {}
    
    Result(bool success, const std::string& code, const std::string& message, 
           const std::optional<T>& data = std::nullopt)
        : success_(success), code(code), message(message), data(data) {}

    // 成功响应
    static Result<T> successResult(const T& data, const std::string& message = "操作成功") {
        Result r;
        r.success_ = true;
        r.code = "200";
        r.message = message;
        r.data = data;
        return r;
    }

    static Result<T> successResult(const std::string& message = "操作成功") {
        return Result(true, "200", message);
    }

    // 成功响应 (别名)
    static Result<T> Success(const T& data, const std::string& message = "操作成功") {
        return successResult(data, message);
    }

    static Result<T> Success(const std::string& message = "操作成功") {
        return successResult(message);
    }

    // 成功响应 (小写别名)
    static Result<T> success(const T& data, const std::string& message = "操作成功") {
        return successResult(data, message);
    }

    // 错误响应
    static Result<T> errorResult(const std::string& code, const std::string& message) {
        return Result(false, code, message);
    }

    // 错误响应 (别名 - 单参数版本, 大写)
    static Result<T> Error(const std::string& message) {
        return Result(false, "500", message);
    }

    // 错误响应 (别名 - 单参数版本, 小写)
    static Result<T> error(const std::string& message) {
        return Result(false, "500", message);
    }

    // 错误响应 (别名 - 双参数版本)
    static Result<T> error(const std::string& code, const std::string& message) {
        return errorResult(code, message);
    }

    // 失败响应 (带默认值 - 兼容 AvatarResponseService)
    static Result<T> fail(int errorCode, const std::string& message, const T& defaultData) {
        Result r;
        r.success_ = false;
        r.code = std::to_string(errorCode);
        r.message = message;
        r.data = defaultData;
        return r;
    }

    static Result<T> fail(int errorCode, const std::string& message) {
        return Result(false, std::to_string(errorCode), message);
    }

    // Getters
    bool isSuccess() const { return success_; }
    const std::string& getCode() const { return code; }
    const std::string& getMessage() const { return message; }
    const std::string& getErrorMsg() const { return message; }
    
    // getError() 返回 ErrorInfo 结构体 (支持 .message 访问模式)
    ErrorInfo getError() const { return ErrorInfo(code, message); }
    
    const std::optional<T>& getData() const { return data; }

    // 便捷方法: 获取值，如果没有则抛异常
    const T& value() const {
        if (!data.has_value()) {
            throw std::runtime_error("Result has no value: " + message);
        }
        return data.value();
    }

    // getValue() 别名
    const T& getValue() const { return value(); }

    // Setters
    void setSuccess(bool value) { success_ = value; }
    void setCode(const std::string& value) { code = value; }
    void setMessage(const std::string& value) { message = value; }
    void setData(const T& value) { data = value; }

    // JSON 序列化
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["success"] = success_;
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
    bool success_;
    std::string code;
    std::string message;
    std::optional<T> data;
};

// 特化 void 类型的 Result
template<>
class Result<void> {
public:
    Result() : success_(false), code(""), message("") {}
    
    Result(bool success, const std::string& code, const std::string& message)
        : success_(success), code(code), message(message) {}

    // 成功响应
    static Result<void> successResult(const std::string& message = "操作成功") {
        return Result(true, "200", message);
    }

    // 成功响应 (别名)
    static Result<void> Success(const std::string& message = "操作成功") {
        return successResult(message);
    }

    // 错误响应
    static Result<void> errorResult(const std::string& code, const std::string& message) {
        return Result(false, code, message);
    }

    // 错误响应 (别名 - 单参数版本, 大写)
    static Result<void> Error(const std::string& message) {
        return Result(false, "500", message);
    }

    // 错误响应 (别名 - 单参数版本, 小写)
    static Result<void> error(const std::string& message) {
        return Result(false, "500", message);
    }

    // 错误响应 (别名 - 双参数版本)
    static Result<void> error(const std::string& code, const std::string& message) {
        return errorResult(code, message);
    }

    // Getters
    bool isSuccess() const { return success_; }
    const std::string& getCode() const { return code; }
    const std::string& getMessage() const { return message; }
    const std::string& getErrorMsg() const { return message; }
    
    // getError() 返回 ErrorInfo 结构体
    ErrorInfo getError() const { return ErrorInfo(code, message); }

    // Setters
    void setSuccess(bool value) { success_ = value; }
    void setCode(const std::string& value) { code = value; }
    void setMessage(const std::string& value) { message = value; }

    // JSON 序列化
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["success"] = success_;
        j["code"] = code;
        j["message"] = message;
        return j;
    }

    // 转换为字符串
    std::string toString() const {
        return toJson().dump();
    }

private:
    bool success_;
    std::string code;
    std::string message;
};

} // namespace Utils
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::utils {
    template<typename T>
    using Result = Yachiyo::Utils::Result<T>;
}