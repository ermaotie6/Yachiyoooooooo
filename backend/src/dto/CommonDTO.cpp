#include "dto/CommonDTO.hpp"
#include "utils/JsonUtils.hpp"

namespace Yachiyo {
namespace dto {

// PaginationRequest 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const PaginationRequest& dto) {
    j = nlohmann::json{
        {"page", dto.page},
        {"page_size", dto.pageSize},
        {"sort_by", dto.sortBy},
        {"sort_order", dto.sortOrder}
    };
}

void from_json(const nlohmann::json& j, PaginationRequest& dto) {
    if (j.contains("page")) {
        j.at("page").get_to(dto.page);
    } else {
        dto.page = 1;
    }
    
    if (j.contains("page_size")) {
        j.at("page_size").get_to(dto.pageSize);
    } else {
        dto.pageSize = 20;
    }
    
    if (j.contains("sort_by")) {
        j.at("sort_by").get_to(dto.sortBy);
    }
    
    if (j.contains("sort_order")) {
        std::string order;
        j.at("sort_order").get_to(order);
        dto.sortOrder = (order == "desc") ? SortOrder::DESC : SortOrder::ASC;
    } else {
        dto.sortOrder = SortOrder::ASC;
    }
}

// PaginationResponse 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const PaginationResponse& dto) {
    j = nlohmann::json{
        {"page", dto.page},
        {"page_size", dto.pageSize},
        {"total_pages", dto.totalPages},
        {"total_items", dto.totalItems},
        {"has_next", dto.hasNext},
        {"has_previous", dto.hasPrevious}
    };
}

void from_json(const nlohmann::json& j, PaginationResponse& dto) {
    j.at("page").get_to(dto.page);
    j.at("page_size").get_to(dto.pageSize);
    j.at("total_pages").get_to(dto.totalPages);
    j.at("total_items").get_to(dto.totalItems);
    j.at("has_next").get_to(dto.hasNext);
    j.at("has_previous").get_to(dto.hasPrevious);
}

// ErrorResponse 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const ErrorResponse& dto) {
    j = nlohmann::json{
        {"success", dto.success},
        {"error_code", dto.errorCode},
        {"error_message", dto.errorMessage},
        {"timestamp", dto.timestamp},
        {"request_id", dto.requestId}
    };
    
    if (!dto.details.empty()) {
        j["details"] = dto.details;
    }
}

void from_json(const nlohmann::json& j, ErrorResponse& dto) {
    j.at("success").get_to(dto.success);
    j.at("error_code").get_to(dto.errorCode);
    j.at("error_message").get_to(dto.errorMessage);
    j.at("timestamp").get_to(dto.timestamp);
    j.at("request_id").get_to(dto.requestId);
    
    if (j.contains("details")) {
        j.at("details").get_to(dto.details);
    }
}

// SuccessResponse 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const SuccessResponse& dto) {
    j = nlohmann::json{
        {"success", dto.success},
        {"message", dto.message},
        {"timestamp", dto.timestamp},
        {"request_id", dto.requestId}
    };
    
    if (dto.data != nullptr) {
        j["data"] = *dto.data;
    }
}

void from_json(const nlohmann::json& j, SuccessResponse& dto) {
    j.at("success").get_to(dto.success);
    j.at("message").get_to(dto.message);
    j.at("timestamp").get_to(dto.timestamp);
    j.at("request_id").get_to(dto.requestId);
    
    // 注意：data 字段需要根据具体类型反序列化
    // 这里不实现通用的 data 反序列化
}

// ValidationError 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const ValidationError& dto) {
    j = nlohmann::json{
        {"field", dto.field},
        {"message", dto.message},
        {"code", dto.code}
    };
}

void from_json(const nlohmann::json& j, ValidationError& dto) {
    j.at("field").get_to(dto.field);
    j.at("message").get_to(dto.message);
    j.at("code").get_to(dto.code);
}

// ValidationErrorResponse 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const ValidationErrorResponse& dto) {
    j = nlohmann::json{
        {"success", dto.success},
        {"error_code", dto.errorCode},
        {"error_message", dto.errorMessage},
        {"timestamp", dto.timestamp},
        {"validation_errors", dto.validationErrors}
    };
}

void from_json(const nlohmann::json& j, ValidationErrorResponse& dto) {
    j.at("success").get_to(dto.success);
    j.at("error_code").get_to(dto.errorCode);
    j.at("error_message").get_to(dto.errorMessage);
    j.at("timestamp").get_to(dto.timestamp);
    j.at("validation_errors").get_to(dto.validationErrors);
}

// FileUploadRequest 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const FileUploadRequest& dto) {
    j = nlohmann::json{
        {"filename", dto.filename},
        {"content_type", dto.contentType},
        {"file_size", dto.fileSize},
        {"upload_type", dto.uploadType}
    };
    
    if (!dto.metadata.empty()) {
        j["metadata"] = dto.metadata;
    }
}

void from_json(const nlohmann::json& j, FileUploadRequest& dto) {
    j.at("filename").get_to(dto.filename);
    j.at("content_type").get_to(dto.contentType);
    j.at("file_size").get_to(dto.fileSize);
    j.at("upload_type").get_to(dto.uploadType);
    
    if (j.contains("metadata")) {
        j.at("metadata").get_to(dto.metadata);
    }
}

// FileUploadResponse 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const FileUploadResponse& dto) {
    j = nlohmann::json{
        {"success", dto.success},
        {"file_id", dto.fileId},
        {"filename", dto.filename},
        {"file_url", dto.fileUrl},
        {"file_size", dto.fileSize},
        {"content_type", dto.contentType},
        {"uploaded_at", dto.uploadedAt}
    };
}

void from_json(const nlohmann::json& j, FileUploadResponse& dto) {
    j.at("success").get_to(dto.success);
    j.at("file_id").get_to(dto.fileId);
    j.at("filename").get_to(dto.filename);
    j.at("file_url").get_to(dto.fileUrl);
    j.at("file_size").get_to(dto.fileSize);
    j.at("content_type").get_to(dto.contentType);
    j.at("uploaded_at").get_to(dto.uploadedAt);
}

// SearchRequest 的 JSON 序列化/反序列化
void to_json(nlohmann::json& j, const SearchRequest& dto) {
    j = nlohmann::json{
        {"query", dto.query},
        {"filters", dto.filters},
        {"pagination", dto.pagination}
    };
}

void from_json(const nlohmann::json& j, SearchRequest& dto) {
    j.at("query").get_to(dto.query);
    j.at("filters").get_to(dto.filters);
    j.at("pagination").get_to(dto.pagination);
}

// SearchResponse 的 JSON 序列化/反序列化
template<typename T>
void to_json(nlohmann::json& j, const SearchResponse<T>& dto) {
    j = nlohmann::json{
        {"results", dto.results},
        {"pagination", dto.pagination},
        {"total_hits", dto.totalHits},
        {"search_time_ms", dto.searchTimeMs}
    };
}

template<typename T>
void from_json(const nlohmann::json& j, SearchResponse<T>& dto) {
    j.at("results").get_to(dto.results);
    j.at("pagination").get_to(dto.pagination);
    j.at("total_hits").get_to(dto.totalHits);
    j.at("search_time_ms").get_to(dto.searchTimeMs);
}

} // namespace dto
} // namespace Yachiyo