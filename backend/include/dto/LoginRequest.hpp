#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace Yachiyo {
namespace DTO {

/**
 * @brief 登录请求DTO
 */
class LoginRequest {
public:
    LoginRequest() = default;
    
    LoginRequest(const std::string& email, const std::string& password)
        : email(email), password(password) {}

    // Getters
    const std::string& getEmail() const { return email; }
    const std::string& getPassword() const { return password; }

    // Setters
    void setEmail(const std::string& newEmail) { email = newEmail; }
    void setPassword(const std::string& newPassword) { password = newPassword; }

    /**
     * @brief 验证请求数据是否有效
     * @return 验证结果和错误信息
     */
    std::pair<bool, std::string> validate() const {
        if (email.empty()) {
            return {false, "邮箱不能为空"};
        }
        if (password.empty()) {
            return {false, "密码不能为空"};
        }
        // 简单的邮箱格式验证
        if (email.find('@') == std::string::npos) {
            return {false, "邮箱格式不正确"};
        }
        return {true, ""};
    }

    // JSON 序列化/反序列化
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LoginRequest, email, password)

private:
    std::string email;
    std::string password;
};

} // namespace DTO
} // namespace Yachiyo