#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Yachiyo {
namespace DTO {

/**
 * @brief 通用响应DTO
 */
struct ResponseDTO {
    int code;
    std::string message;
    json data;
    
    ResponseDTO(int code = 200, const std::string& message = "success", const json& data = json::object())
        : code(code), message(message), data(data) {}
    
    // 转换为JSON
    json toJson() const {
        return {
            {"code", code},
            {"message", message},
            {"data", data}
        };
    }
    
    // 从JSON解析
    static ResponseDTO fromJson(const json& j) {
        return ResponseDTO(
            j.value("code", 200),
            j.value("message", "success"),
            j.value("data", json::object())
        );
    }
};

/**
 * @brief 分页请求DTO
 */
struct PageRequestDTO {
    int page;
    int size;
    
    PageRequestDTO(int page = 1, int size = 20)
        : page(page), size(size) {}
    
    json toJson() const {
        return {
            {"page", page},
            {"size", size}
        };
    }
    
    static PageRequestDTO fromJson(const json& j) {
        return PageRequestDTO(
            j.value("page", 1),
            j.value("size", 20)
        );
    }
};

/**
 * @brief 分页响应DTO
 */
template<typename T>
struct PageResponseDTO {
    int total;
    int page;
    int size;
    std::vector<T> items;
    
    PageResponseDTO(int total = 0, int page = 1, int size = 20, const std::vector<T>& items = {})
        : total(total), page(page), size(size), items(items) {}
    
    json toJson() const {
        json itemsJson = json::array();
        for (const auto& item : items) {
            itemsJson.push_back(item.toJson());
        }
        
        return {
            {"total", total},
            {"page", page},
            {"size", size},
            {"items", itemsJson}
        };
    }
};

} // namespace DTO
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::dto {
    using Yachiyo::DTO::ResponseDTO;
}