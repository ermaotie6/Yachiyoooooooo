#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <crow.h>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

namespace yachiyo::utils {

/**
 * @brief JSON工具类
 */
class JsonUtils {
public:
    /**
     * @brief 创建错误响应 (用于 Controller 层)
     * @param statusCode HTTP 状态码
     * @param message 错误信息
     * @return crow::response
     */
    static crow::response createErrorResponse(int statusCode, const std::string& message) {
        nlohmann::json body;
        body["success"] = false;
        body["code"] = statusCode;
        body["message"] = message;
        
        auto resp = crow::response(statusCode, body.dump());
        resp.set_header("Content-Type", "application/json");
        return resp;
    }
    
    /**
     * @brief 创建成功响应 (用于 Controller 层)
     * @param data 响应数据
     * @param message 成功信息
     * @return crow::response
     */
    static crow::response createSuccessResponse(const nlohmann::json& data = nullptr,
                                                 const std::string& message = "操作成功") {
        nlohmann::json body;
        body["success"] = true;
        body["code"] = 200;
        body["message"] = message;
        if (!data.is_null()) {
            body["data"] = data;
        }
        
        auto resp = crow::response(200, body.dump());
        resp.set_header("Content-Type", "application/json");
        return resp;
    }

    /**
     * @brief 从文件读取JSON
     */
    static json readFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("无法打开文件: " + filename);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        return json::parse(buffer.str());
    }
    
    /**
     * @brief 将JSON写入文件
     */
    static void writeToFile(const std::string& filename, const json& j, int indent = 4) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("无法打开文件: " + filename);
        }
        
        file << j.dump(indent);
        file.close();
    }
    
    /**
     * @brief 从字符串解析JSON
     */
    static json parse(const std::string& jsonStr) {
        return json::parse(jsonStr);
    }
    
    /**
     * @brief 将JSON转换为字符串
     */
    static std::string stringify(const json& j, int indent = -1) {
        return j.dump(indent);
    }
    
    /**
     * @brief 安全获取JSON值，如果不存在则返回默认值
     */
    template<typename T>
    static T getValue(const json& j, const std::string& key, const T& defaultValue) {
        if (j.contains(key) && !j[key].is_null()) {
            try {
                return j[key].get<T>();
            } catch (const json::exception&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    /**
     * @brief 安全获取JSON值（字符串特化）
     */
    static std::string getString(const json& j, const std::string& key, const std::string& defaultValue = "") {
        return getValue<std::string>(j, key, defaultValue);
    }
    
    /**
     * @brief 安全获取JSON值（整数特化）
     */
    static int getInt(const json& j, const std::string& key, int defaultValue = 0) {
        return getValue<int>(j, key, defaultValue);
    }
    
    /**
     * @brief 安全获取JSON值（布尔值特化）
     */
    static bool getBool(const json& j, const std::string& key, bool defaultValue = false) {
        return getValue<bool>(j, key, defaultValue);
    }
    
    /**
     * @brief 安全获取JSON值（浮点数特化）
     */
    static double getDouble(const json& j, const std::string& key, double defaultValue = 0.0) {
        return getValue<double>(j, key, defaultValue);
    }
    
    /**
     * @brief 合并两个JSON对象
     */
    static json merge(const json& a, const json& b) {
        json result = a;
        result.update(b);
        return result;
    }
    
    /**
     * @brief 检查JSON是否包含所有必需的键
     */
    static bool hasRequiredKeys(const json& j, const std::vector<std::string>& requiredKeys) {
        for (const auto& key : requiredKeys) {
            if (!j.contains(key) || j[key].is_null()) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief 验证JSON模式
     */
    static bool validateSchema(const json& j, const json& schema) {
        // 简化的模式验证
        // 实际实现中可以使用更复杂的验证库
        if (!schema.contains("type") || !schema.contains("properties")) {
            return true; // 没有模式定义，直接通过
        }
        
        std::string type = schema["type"].get<std::string>();
        if (type == "object") {
            const json& properties = schema["properties"];
            for (auto it = properties.begin(); it != properties.end(); ++it) {
                const std::string& key = it.key();
                const json& propSchema = it.value();
                
                if (propSchema.contains("required") && propSchema["required"].get<bool>()) {
                    if (!j.contains(key) || j[key].is_null()) {
                        return false;
                    }
                }
            }
        }
        
        return true;
    }
    
    /**
     * @brief 美化JSON字符串
     */
    static std::string prettyPrint(const json& j) {
        return j.dump(4);
    }
    
    /**
     * @brief 压缩JSON字符串（移除所有空白）
     */
    static std::string minify(const json& j) {
        return j.dump(-1);
    }
};

} // namespace yachiyo::utils