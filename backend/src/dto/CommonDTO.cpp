#include "dto/CommonDTO.hpp"

// 此文件为 CommonDTO.hpp 中定义的DTO结构提供 nlohmann::json 的 ADL to_json / from_json 序列化。
// 所有结构已在 hpp 中提供了 toJson() / fromJson() 成员方法。

namespace Yachiyo {
namespace DTO {

// ==================== ResponseDTO ====================

void to_json(nlohmann::json& j, const ResponseDTO& dto) {
    j = dto.toJson();
}

void from_json(const nlohmann::json& j, ResponseDTO& dto) {
    dto = ResponseDTO::fromJson(j);
}

// ==================== PageRequestDTO ====================

void to_json(nlohmann::json& j, const PageRequestDTO& dto) {
    j = dto.toJson();
}

void from_json(const nlohmann::json& j, PageRequestDTO& dto) {
    dto = PageRequestDTO::fromJson(j);
}

// 注意: PageResponseDTO<T> 是模板类，其 toJson() 已在 hpp 中以 inline 方式实现，
// 无需在 cpp 中提供额外的序列化实现。

} // namespace DTO
} // namespace Yachiyo
