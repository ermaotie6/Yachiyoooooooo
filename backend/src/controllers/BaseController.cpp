#include "../../include/controllers/BaseController.hpp"
#include "../../include/utils/LogUtils.hpp"
#include "../../include/utils/JsonUtils.hpp"
#include "../../include/utils/JwtUtil.hpp"
#include <crow.h>
#include <chrono>

namespace yachiyo::controllers {

BaseController::BaseController() {
    logger = LogUtils::getLogger("BaseController");
    logger->info("基础控制器初始化完成");
}

BaseController::~BaseController() {
    logger->info("基础控制器销毁");
}

crow::response BaseController::successResponse(const std::string& message, 
                                              const nlohmann::json& data) {
    nlohmann::json response = {
        {"success", true},
        {"message", message},
        {"data", data},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    
    crow::response res;
    res.code = 200;
    res.set_header("Content-Type", "application/json");
    res.body = response.dump();
    
    logger->debug("成功响应: {}", message);
    return res;
}

crow::response BaseController::createdResponse(const std::string& message, 
                                              const nlohmann::json& data) {
    nlohmann::json response = {
        {"success", true},
        {"message", message},
        {"data", data},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    
    crow::response res;
    res.code = 201;
    res.set_header("Content-Type", "application/json");
    res.body = response.dump();
    
    logger->debug("创建成功响应: {}", message);
    return res;
}

crow::response BaseController::errorResponse(int statusCode, 
                                            const std::string& message, 
                                            const std::string& errorCode) {
    nlohmann::json response = {
        {"success", false},
        {"message", message},
        {"error", {
            {"code", errorCode},
            {"message", message}
        }},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    
    crow::response res;
    res.code = statusCode;
    res.set_header("Content-Type", "application/json");
    res.body = response.dump();
    
    logger->warn("错误响应: status={}, message={}, errorCode={}", 
                statusCode, message, errorCode);
    return res;
}

crow::response BaseController::badRequestResponse(const std::string& message) {
    return errorResponse(400, message, "BAD_REQUEST");
}

crow::response BaseController::unauthorizedResponse(const std::string& message) {
    return errorResponse(401, message, "UNAUTHORIZED");
}

crow::response BaseController::forbiddenResponse(const std::string& message) {
    return errorResponse(403, message, "FORBIDDEN");
}

crow::response BaseController::notFoundResponse(const std::string& message) {
    return errorResponse(404, message, "NOT_FOUND");
}

crow::response BaseController::internalServerErrorResponse(const std::string& message) {
    return errorResponse(500, message, "INTERNAL_SERVER_ERROR");
}

crow::response BaseController::validationErrorResponse(const std::vector<std::string>& errors) {
    nlohmann::json response = {
        {"success", false},
        {"message", "验证失败"},
        {"error", {
            {"code", "VALIDATION_ERROR"},
            {"message", "请求数据验证失败"},
            {"details", errors}
        }},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    
    crow::response res;
    res.code = 422;
    res.set_header("Content-Type", "application/json");
    res.body = response.dump();
    
    logger->warn("验证错误响应: errors={}", nlohmann::json(errors).dump());
    return res;
}

std::string BaseController::getAuthToken(const crow::request& req) {
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty()) {
        return "";
    }
    
    // 检查Bearer token格式
    const std::string bearerPrefix = "Bearer ";
    if (authHeader.find(bearerPrefix) == 0) {
        return authHeader.substr(bearerPrefix.length());
    }
    
    return authHeader;
}

std::string BaseController::getUserIdFromToken(const crow::request& req) {
    std::string token = getAuthToken(req);
    if (token.empty()) {
        return "";
    }
    
    try {
        // 这里应该验证JWT令牌并提取用户ID
        // 暂时返回模拟用户ID
        return JwtUtil::verifyToken(token);
    } catch (const std::exception& e) {
        logger->error("从令牌获取用户ID失败: {}", e.what());
        return "";
    }
}

bool BaseController::validateJson(const crow::request& req, nlohmann::json& json) {
    try {
        json = nlohmann::json::parse(req.body);
        return true;
    } catch (const std::exception& e) {
        logger->error("JSON解析失败: {}", e.what());
        return false;
    }
}

bool BaseController::validateRequiredFields(const nlohmann::json& json, 
                                           const std::vector<std::string>& requiredFields,
                                           std::vector<std::string>& missingFields) {
    missingFields.clear();
    
    for (const auto& field : requiredFields) {
        if (!json.contains(field) || json[field].is_null()) {
            missingFields.push_back(field);
        }
    }
    
    return missingFields.empty();
}

std::string BaseController::getClientIp(const crow::request& req) {
    std::string ip = req.get_header_value("X-Forwarded-For");
    if (ip.empty()) {
        ip = req.get_header_value("X-Real-IP");
    }
    if (ip.empty()) {
        ip = req.remote_ip_address;
    }
    
    // 处理多个IP的情况（如X-Forwarded-For可能包含多个IP）
    size_t commaPos = ip.find(',');
    if (commaPos != std::string::npos) {
        ip = ip.substr(0, commaPos);
    }
    
    return ip;
}

std::string BaseController::getUserAgent(const crow::request& req) {
    return req.get_header_value("User-Agent");
}

void BaseController::logRequest(const crow::request& req, const std::string& endpoint) {
    std::string method = crow::method_name(req.method);
    std::string path = req.url;
    std::string clientIp = getClientIp(req);
    std::string userAgent = getUserAgent(req);
    
    logger->info("请求: {} {} | 客户端IP: {} | User-Agent: {} | 端点: {}", 
                method, path, clientIp, userAgent, endpoint);
}

void BaseController::logResponse(const crow::request& req, const crow::response& res, 
                                const std::string& endpoint) {
    std::string method = crow::method_name(req.method);
    std::string path = req.url;
    int statusCode = res.code;
    
    logger->info("响应: {} {} -> {} | 端点: {}", 
                method, path, statusCode, endpoint);
}

crow::response BaseController::handleException(const std::exception& e, 
                                              const std::string& context) {
    logger->error("处理异常 [{}]: {}", context, e.what());
    return internalServerErrorResponse("服务器内部错误: " + std::string(e.what()));
}

crow::response BaseController::paginatedResponse(const nlohmann::json& data, 
                                                int page, int pageSize, 
                                                int totalItems, int totalPages) {
    nlohmann::json response = {
        {"success", true},
        {"data", data},
        {"pagination", {
            {"page", page},
            {"pageSize", pageSize},
            {"totalItems", totalItems},
            {"totalPages", totalPages},
            {"hasNext", page < totalPages},
            {"hasPrev", page > 1}
        }},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    
    crow::response res;
    res.code = 200;
    res.set_header("Content-Type", "application/json");
    res.body = response.dump();
    
    logger->debug("分页响应: page={}, pageSize={}, totalItems={}", 
                 page, pageSize, totalItems);
    return res;
}

bool BaseController::validatePaginationParams(int& page, int& pageSize, 
                                             int maxPageSize) {
    // 验证页码
    if (page < 1) {
        page = 1;
    }
    
    // 验证每页大小
    if (pageSize < 1) {
        pageSize = 10;
    } else if (pageSize > maxPageSize) {
        pageSize = maxPageSize;
    }
    
    return true;
}

std::string BaseController::sanitizeInput(const std::string& input) {
    // 简单的输入清理，防止XSS攻击
    std::string sanitized = input;
    
    // 替换HTML特殊字符
    std::unordered_map<std::string, std::string> replacements = {
        {"&", "&amp;"},
        {"<", "&lt;"},
        {">", "&gt;"},
        {"\"", "&quot;"},
        {"'", "&#x27;"},
        {"/", "&#x2F;"}
    };
    
    for (const auto& [search, replace] : replacements) {
        size_t pos = 0;
        while ((pos = sanitized.find(search, pos)) != std::string::npos) {
            sanitized.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    }
    
    return sanitized;
}

bool BaseController::validateStringLength(const std::string& str, 
                                         int minLength, int maxLength) {
    if (str.length() < minLength) {
        return false;
    }
    if (maxLength > 0 && str.length() > maxLength) {
        return false;
    }
    return true;
}

bool BaseController::validateEmailFormat(const std::string& email) {
    // 简单的邮箱格式验证
    std::regex emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(email, emailRegex);
}

bool BaseController::validateUrlFormat(const std::string& url) {
    // 简单的URL格式验证
    std::regex urlRegex(R"(^(https?|ftp)://[^\s/$.?#].[^\s]*$)");
    return std::regex_match(url, urlRegex);
}

} // namespace yachiyo::controllers