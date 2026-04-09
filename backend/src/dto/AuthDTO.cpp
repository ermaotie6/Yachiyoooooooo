#include "dto/AuthDTO.hpp"

// 此文件为 AuthDTO.hpp 中定义的DTO结构提供 nlohmann::json 的 ADL to_json / from_json 序列化。
// 所有结构已在 hpp 中提供了 toJson() / fromJson() 成员方法。
// 以下 ADL 自由函数可供 nlohmann::json 的隐式转换使用。

namespace Yachiyo {
namespace DTO {

// ==================== RegisterRequestDTO ====================

void to_json(nlohmann::json& j, const RegisterRequestDTO& dto) {
    j = dto.toJson();
}

void from_json(const nlohmann::json& j, RegisterRequestDTO& dto) {
    dto = RegisterRequestDTO::fromJson(j);
}

// ==================== LoginRequestDTO ====================

void to_json(nlohmann::json& j, const LoginRequestDTO& dto) {
    j = dto.toJson();
}

void from_json(const nlohmann::json& j, LoginRequestDTO& dto) {
    dto = LoginRequestDTO::fromJson(j);
}

// ==================== LoginResponseDTO ====================

void to_json(nlohmann::json& j, const LoginResponseDTO& dto) {
    j = dto.toJson();
}

void from_json(const nlohmann::json& j, LoginResponseDTO& dto) {
    dto = LoginResponseDTO::fromJson(j);
}

// ==================== RefreshTokenRequestDTO ====================

void to_json(nlohmann::json& j, const RefreshTokenRequestDTO& dto) {
    j = dto.toJson();
}

void from_json(const nlohmann::json& j, RefreshTokenRequestDTO& dto) {
    dto = RefreshTokenRequestDTO::fromJson(j);
}

} // namespace DTO
} // namespace Yachiyo
