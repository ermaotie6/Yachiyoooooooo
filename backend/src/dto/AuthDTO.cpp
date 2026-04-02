#include "dto/AuthDTO.hpp"
#include "utils/JsonUtils.hpp"

namespace Yachiyo {
namespace dto {

// LoginRequestDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const LoginRequestDTO& dto) {
    j = nlohmann::json{
        {"username", dto.username},
        {"password", dto.password},
        {"remember_me", dto.rememberMe}
    };
}

void from_json(const nlohmann::json& j, LoginRequestDTO& dto) {
    j.at("username").get_to(dto.username);
    j.at("password").get_to(dto.password);
    
    if (j.contains("remember_me")) {
        j.at("remember_me").get_to(dto.rememberMe);
    } else {
        dto.rememberMe = false;
    }
}

// LoginResponseDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const LoginResponseDTO& dto) {
    j = nlohmann::json{
        {"success", dto.success},
        {"message", dto.message},
        {"access_token", dto.accessToken},
        {"refresh_token", dto.refreshToken},
        {"token_type", dto.tokenType},
        {"expires_in", dto.expiresIn},
        {"user_id", dto.userId},
        {"username", dto.username}
    };
}

void from_json(const nlohmann::json& j, LoginResponseDTO& dto) {
    j.at("success").get_to(dto.success);
    j.at("message").get_to(dto.message);
    j.at("access_token").get_to(dto.accessToken);
    j.at("refresh_token").get_to(dto.refreshToken);
    j.at("token_type").get_to(dto.tokenType);
    j.at("expires_in").get_to(dto.expiresIn);
    j.at("user_id").get_to(dto.userId);
    j.at("username").get_to(dto.username);
}

// RegisterRequestDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const RegisterRequestDTO& dto) {
    j = nlohmann::json{
        {"username", dto.username},
        {"email", dto.email},
        {"password", dto.password},
        {"confirm_password", dto.confirmPassword},
        {"first_name", dto.firstName},
        {"last_name", dto.lastName}
    };
}

void from_json(const nlohmann::json& j, RegisterRequestDTO& dto) {
    j.at("username").get_to(dto.username);
    j.at("email").get_to(dto.email);
    j.at("password").get_to(dto.password);
    j.at("confirm_password").get_to(dto.confirmPassword);
    
    if (j.contains("first_name")) {
        j.at("first_name").get_to(dto.firstName);
    }
    
    if (j.contains("last_name")) {
        j.at("last_name").get_to(dto.lastName);
    }
}

// RegisterResponseDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const RegisterResponseDTO& dto) {
    j = nlohmann::json{
        {"success", dto.success},
        {"message", dto.message},
        {"user_id", dto.userId},
        {"username", dto.username},
        {"email", dto.email},
        {"created_at", dto.createdAt}
    };
}

void from_json(const nlohmann::json& j, RegisterResponseDTO& dto) {
    j.at("success").get_to(dto.success);
    j.at("message").get_to(dto.message);
    j.at("user_id").get_to(dto.userId);
    j.at("username").get_to(dto.username);
    j.at("email").get_to(dto.email);
    j.at("created_at").get_to(dto.createdAt);
}

// TokenResponseDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const TokenResponseDTO& dto) {
    j = nlohmann::json{
        {"access_token", dto.accessToken},
        {"refresh_token", dto.refreshToken},
        {"token_type", dto.tokenType},
        {"expires_in", dto.expiresIn},
        {"scope", dto.scope}
    };
}

void from_json(const nlohmann::json& j, TokenResponseDTO& dto) {
    j.at("access_token").get_to(dto.accessToken);
    j.at("refresh_token").get_to(dto.refreshToken);
    j.at("token_type").get_to(dto.tokenType);
    j.at("expires_in").get_to(dto.expiresIn);
    
    if (j.contains("scope")) {
        j.at("scope").get_to(dto.scope);
    }
}

// TokenValidationDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const TokenValidationDTO& dto) {
    j = nlohmann::json{
        {"valid", dto.valid},
        {"user_id", dto.userId},
        {"username", dto.username},
        {"expires_at", dto.expiresAt},
        {"issued_at", dto.issuedAt},
        {"error", dto.error}
    };
}

void from_json(const nlohmann::json& j, TokenValidationDTO& dto) {
    j.at("valid").get_to(dto.valid);
    j.at("user_id").get_to(dto.userId);
    j.at("username").get_to(dto.username);
    j.at("expires_at").get_to(dto.expiresAt);
    j.at("issued_at").get_to(dto.issuedAt);
    
    if (j.contains("error")) {
        j.at("error").get_to(dto.error);
    }
}

// PasswordResetRequestDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const PasswordResetRequestDTO& dto) {
    j = nlohmann::json{
        {"email", dto.email},
        {"reset_token", dto.resetToken},
        {"new_password", dto.newPassword},
        {"confirm_password", dto.confirmPassword}
    };
}

void from_json(const nlohmann::json& j, PasswordResetRequestDTO& dto) {
    j.at("email").get_to(dto.email);
    
    if (j.contains("reset_token")) {
        j.at("reset_token").get_to(dto.resetToken);
    }
    
    if (j.contains("new_password")) {
        j.at("new_password").get_to(dto.newPassword);
    }
    
    if (j.contains("confirm_password")) {
        j.at("confirm_password").get_to(dto.confirmPassword);
    }
}

// PasswordResetResponseDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const PasswordResetResponseDTO& dto) {
    j = nlohmann::json{
        {"success", dto.success},
        {"message", dto.message},
        {"reset_token_sent", dto.resetTokenSent},
        {"email", dto.email}
    };
}

void from_json(const nlohmann::json& j, PasswordResetResponseDTO& dto) {
    j.at("success").get_to(dto.success);
    j.at("message").get_to(dto.message);
    j.at("reset_token_sent").get_to(dto.resetTokenSent);
    j.at("email").get_to(dto.email);
}

// ChangePasswordRequestDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const ChangePasswordRequestDTO& dto) {
    j = nlohmann::json{
        {"user_id", dto.userId},
        {"old_password", dto.oldPassword},
        {"new_password", dto.newPassword},
        {"confirm_password", dto.confirmPassword}
    };
}

void from_json(const nlohmann::json& j, ChangePasswordRequestDTO& dto) {
    j.at("user_id").get_to(dto.userId);
    j.at("old_password").get_to(dto.oldPassword);
    j.at("new_password").get_to(dto.newPassword);
    j.at("confirm_password").get_to(dto.confirmPassword);
}

// ChangePasswordResponseDTO 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const ChangePasswordResponseDTO& dto) {
    j = nlohmann::json{
        {"success", dto.success},
        {"message", dto.message},
        {"password_changed_at", dto.passwordChangedAt}
    };
}

void from_json(const nlohmann::json& j, ChangePasswordResponseDTO& dto) {
    j.at("success").get_to(dto.success);
    j.at("message").get_to(dto.message);
    j.at("password_changed_at").get_to(dto.passwordChangedAt);
}

} // namespace dto
} // namespace Yachiyo