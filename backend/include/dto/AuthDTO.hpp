#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "CommonDTO.hpp"

using json = nlohmann::json;

namespace Yachiyo {
namespace DTO {

/**
 * @brief 注册请求DTO
 */
struct RegisterRequestDTO {
    std::string username;
    std::string email;
    std::string password;
    std::string nickname;
    
    RegisterRequestDTO() = default;
    
    RegisterRequestDTO(const std::string& username, const std::string& email, 
                      const std::string& password, const std::string& nickname)
        : username(username), email(email), password(password), nickname(nickname) {}
    
    json toJson() const {
        return {
            {"username", username},
            {"email", email},
            {"password", password},
            {"nickname", nickname}
        };
    }
    
    static RegisterRequestDTO fromJson(const json& j) {
        return RegisterRequestDTO(
            j.value("username", ""),
            j.value("email", ""),
            j.value("password", ""),
            j.value("nickname", "")
        );
    }
    
    bool isValid() const {
        return !username.empty() && !email.empty() && !password.empty();
    }
};

/**
 * @brief 登录请求DTO
 */
struct LoginRequestDTO {
    std::string username;
    std::string password;
    
    LoginRequestDTO() = default;
    
    LoginRequestDTO(const std::string& username, const std::string& password)
        : username(username), password(password) {}
    
    json toJson() const {
        return {
            {"username", username},
            {"password", password}
        };
    }
    
    static LoginRequestDTO fromJson(const json& j) {
        return LoginRequestDTO(
            j.value("username", ""),
            j.value("password", "")
        );
    }
    
    bool isValid() const {
        return !username.empty() && !password.empty();
    }
};

/**
 * @brief 登录响应DTO
 */
struct LoginResponseDTO {
    std::string token;
    std::string refreshToken;
    int expiresIn;
    std::string userId;
    std::string username;
    std::string nickname;
    
    LoginResponseDTO() = default;
    
    LoginResponseDTO(const std::string& token, const std::string& refreshToken, 
                    int expiresIn, const std::string& userId,
                    const std::string& username, const std::string& nickname)
        : token(token), refreshToken(refreshToken), expiresIn(expiresIn),
          userId(userId), username(username), nickname(nickname) {}
    
    json toJson() const {
        return {
            {"token", token},
            {"refreshToken", refreshToken},
            {"expiresIn", expiresIn},
            {"userId", userId},
            {"username", username},
            {"nickname", nickname}
        };
    }
    
    static LoginResponseDTO fromJson(const json& j) {
        return LoginResponseDTO(
            j.value("token", ""),
            j.value("refreshToken", ""),
            j.value("expiresIn", 3600),
            j.value("userId", ""),
            j.value("username", ""),
            j.value("nickname", "")
        );
    }
};

/**
 * @brief 刷新令牌请求DTO
 */
struct RefreshTokenRequestDTO {
    std::string refreshToken;
    
    RefreshTokenRequestDTO() = default;
    
    RefreshTokenRequestDTO(const std::string& refreshToken)
        : refreshToken(refreshToken) {}
    
    json toJson() const {
        return {
            {"refreshToken", refreshToken}
        };
    }
    
    static RefreshTokenRequestDTO fromJson(const json& j) {
        return RefreshTokenRequestDTO(
            j.value("refreshToken", "")
        );
    }
    
    bool isValid() const {
        return !refreshToken.empty();
    }
};

} // namespace DTO
} // namespace Yachiyo

// 后向兼容别名
namespace yachiyo::dto {
    using Yachiyo::DTO::RegisterRequestDTO;
    using Yachiyo::DTO::LoginRequestDTO;
    using Yachiyo::DTO::LoginResponseDTO;
    using Yachiyo::DTO::RefreshTokenRequestDTO;
}