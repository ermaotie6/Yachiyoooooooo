#include "utils/JsonUtils.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Yachiyo {
namespace Utils {

JsonUtils::JsonUtils() {
    // 初始化默认配置
    prettyPrint = true;
    indentSize = 2;
    escapeUnicode = false;
    sortKeys = false;
}

std::string JsonUtils::toJson(const nlohmann::json& json) {
    try {
        nlohmann::json::serializer s;
        
        if (prettyPrint) {
            return json.dump(indentSize);
        } else {
            return json.dump();
        }
    } catch (const std::exception& e) {
        std::cerr << "JSON serialization error: " << e.what() << std::endl;
        return "{}";
    }
}

nlohmann::json JsonUtils::fromJson(const std::string& jsonString) {
    try {
        return nlohmann::json::parse(jsonString);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        throw std::runtime_error(std::string("JSON parsing error: ") + e.what());
    }
}

std::string JsonUtils::toJson(const std::map<std::string, std::string>& map) {
    try {
        nlohmann::json json;
        for (const auto& [key, value] : map) {
            json[key] = value;
        }
        return toJson(json);
    } catch (const std::exception& e) {
        std::cerr << "Map to JSON error: " << e.what() << std::endl;
        return "{}";
    }
}

std::string JsonUtils::toJson(const std::vector<std::string>& vector) {
    try {
        nlohmann::json json = vector;
        return toJson(json);
    } catch (const std::exception& e) {
        std::cerr << "Vector to JSON error: " << e.what() << std::endl;
        return "[]";
    }
}

std::string JsonUtils::toJson(const std::vector<std::map<std::string, std::string>>& vectorOfMaps) {
    try {
        nlohmann::json jsonArray = nlohmann::json::array();
        
        for (const auto& map : vectorOfMaps) {
            nlohmann::json jsonObj;
            for (const auto& [key, value] : map) {
                jsonObj[key] = value;
            }
            jsonArray.push_back(jsonObj);
        }
        
        return toJson(jsonArray);
    } catch (const std::exception& e) {
        std::cerr << "Vector of maps to JSON error: " << e.what() << std::endl;
        return "[]";
    }
}

std::map<std::string, std::string> JsonUtils::fromJsonToMap(const std::string& jsonString) {
    try {
        nlohmann::json json = fromJson(jsonString);
        std::map<std::string, std::string> result;
        
        if (json.is_object()) {
            for (auto it = json.begin(); it != json.end(); ++it) {
                if (it.value().is_string()) {
                    result[it.key()] = it.value().get<std::string>();
                } else {
                    result[it.key()] = it.value().dump();
                }
            }
        }
        
        return result;
    } catch (const std::exception& e) {
        std::cerr << "JSON to map error: " << e.what() << std::endl;
        return {};
    }
}

std::vector<std::string> JsonUtils::fromJsonToVector(const std::string& jsonString) {
    try {
        nlohmann::json json = fromJson(jsonString);
        std::vector<std::string> result;
        
        if (json.is_array()) {
            for (const auto& item : json) {
                if (item.is_string()) {
                    result.push_back(item.get<std::string>());
                } else {
                    result.push_back(item.dump());
                }
            }
        }
        
        return result;
    } catch (const std::exception& e) {
        std::cerr << "JSON to vector error: " << e.what() << std::endl;
        return {};
    }
}

std::vector<std::map<std::string, std::string>> JsonUtils::fromJsonToVectorOfMaps(const std::string& jsonString) {
    try {
        nlohmann::json json = fromJson(jsonString);
        std::vector<std::map<std::string, std::string>> result;
        
        if (json.is_array()) {
            for (const auto& item : json) {
                if (item.is_object()) {
                    std::map<std::string, std::string> map;
                    for (auto it = item.begin(); it != item.end(); ++it) {
                        if (it.value().is_string()) {
                            map[it.key()] = it.value().get<std::string>();
                        } else {
                            map[it.key()] = it.value().dump();
                        }
                    }
                    result.push_back(map);
                }
            }
        }
        
        return result;
    } catch (const std::exception& e) {
        std::cerr << "JSON to vector of maps error: " << e.what() << std::endl;
        return {};
    }
}

bool JsonUtils::validateJson(const std::string& jsonString) {
    try {
        nlohmann::json::parse(jsonString);
        return true;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

std::string JsonUtils::minifyJson(const std::string& jsonString) {
    try {
        nlohmann::json json = fromJson(jsonString);
        return json.dump();
    } catch (const std::exception& e) {
        std::cerr << "JSON minify error: " << e.what() << std::endl;
        return jsonString;
    }
}

std::string JsonUtils::prettifyJson(const std::string& jsonString, int indent) {
    try {
        nlohmann::json json = fromJson(jsonString);
        return json.dump(indent);
    } catch (const std::exception& e) {
        std::cerr << "JSON prettify error: " << e.what() << std::endl;
        return jsonString;
    }
}

std::string JsonUtils::escapeJsonString(const std::string& str) {
    std::ostringstream oss;
    
    for (char c : str) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (escapeUnicode && (static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) >= 0x7F)) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                        << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    oss << c;
                }
                break;
        }
    }
    
    return oss.str();
}

std::string JsonUtils::unescapeJsonString(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"':  result += '"'; i++; break;
                case '\\': result += '\\'; i++; break;
                case '/':  result += '/'; i++; break;
                case 'b':  result += '\b'; i++; break;
                case 'f':  result += '\f'; i++; break;
                case 'n':  result += '\n'; i++; break;
                case 'r':  result += '\r'; i++; break;
                case 't':  result += '\t'; i++; break;
                case 'u':
                    // 处理Unicode转义序列
                    if (i + 5 < str.length()) {
                        std::string hex = str.substr(i + 2, 4);
                        try {
                            int code = std::stoi(hex, nullptr, 16);
                            result += static_cast<char>(code);
                            i += 5;
                        } catch (...) {
                            // 解析失败，保留原样
                            result += str[i];
                        }
                    } else {
                        result += str[i];
                    }
                    break;
                default:
                    result += str[i];
                    break;
            }
        } else {
            result += str[i];
        }
    }
    
    return result;
}

nlohmann::json JsonUtils::mergeJson(const nlohmann::json& json1, const nlohmann::json& json2) {
    nlohmann::json result = json1;
    
    if (json1.is_object() && json2.is_object()) {
        for (auto it = json2.begin(); it != json2.end(); ++it) {
            std::string key = it.key();
            if (result.contains(key) && result[key].is_object() && it.value().is_object()) {
                // 递归合并对象
                result[key] = mergeJson(result[key], it.value());
            } else {
                // 直接赋值
                result[key] = it.value();
            }
        }
    } else if (json1.is_array() && json2.is_array()) {
        // 合并数组
        for (const auto& item : json2) {
            result.push_back(item);
        }
    } else {
        // 类型不匹配，返回第二个JSON
        result = json2;
    }
    
    return result;
}

nlohmann::json JsonUtils::diffJson(const nlohmann::json& json1, const nlohmann::json& json2) {
    nlohmann::json diff = nlohmann::json::object();
    
    if (json1.is_object() && json2.is_object()) {
        // 收集所有键
        std::set<std::string> allKeys;
        for (auto it = json1.begin(); it != json1.end(); ++it) {
            allKeys.insert(it.key());
        }
        for (auto it = json2.begin(); it != json2.end(); ++it) {
            allKeys.insert(it.key());
        }
        
        // 比较每个键
        for (const auto& key : allKeys) {
            bool hasKey1 = json1.contains(key);
            bool hasKey2 = json2.contains(key);
            
            if (hasKey1 && hasKey2) {
                // 两个JSON都有这个键，递归比较
                nlohmann::json subDiff = diffJson(json1[key], json2[key]);
                if (!subDiff.empty()) {
                    diff[key] = subDiff;
                }
            } else if (hasKey1) {
                // 只在第一个JSON中有
                diff[key] = {{"old", json1[key]}, {"new", nullptr}};
            } else if (hasKey2) {
                // 只在第二个JSON中有
                diff[key] = {{"old", nullptr}, {"new", json2[key]}};
            }
        }
    } else if (json1.is_array() && json2.is_array()) {
        // 比较数组
        if (json1 != json2) {
            diff = {{"old", json1}, {"new", json2}};
        }
    } else {
        // 比较标量值
        if (json1 != json2) {
            diff = {{"old", json1}, {"new", json2}};
        }
    }
    
    return diff;
}

std::string JsonUtils::getJsonType(const nlohmann::json& json) {
    if (json.is_null()) return "null";
    if (json.is_boolean()) return "boolean";
    if (json.is_number_integer()) return "integer";
    if (json.is_number_float()) return "float";
    if (json.is_string()) return "string";
    if (json.is_array()) return "array";
    if (json.is_object()) return "object";
    if (json.is_binary()) return "binary";
    return "unknown";
}

bool JsonUtils::jsonContains(const nlohmann::json& json, const std::string& key) {
    if (!json.is_object()) {
        return false;
    }
    
    return json.contains(key);
}

nlohmann::json JsonUtils::jsonGet(const nlohmann::json& json, const std::string& key, const nlohmann::json& defaultValue) {
    if (!json.is_object() || !json.contains(key)) {
        return defaultValue;
    }
    
    return json[key];
}

std::string JsonUtils::jsonGetString(const nlohmann::json& json, const std::string& key, const std::string& defaultValue) {
    if (!json.is_object() || !json.contains(key) || !json[key].is_string()) {
        return defaultValue;
    }
    
    return json[key].get<std::string>();
}

int JsonUtils::jsonGetInt(const nlohmann::json& json, const std::string& key, int defaultValue) {
    if (!json.is_object() || !json.contains(key) || !json[key].is_number_integer()) {
        return defaultValue;
    }
    
    return json[key].get<int>();
}

double JsonUtils::jsonGetDouble(const nlohmann::json& json, const std::string& key, double defaultValue) {
    if (!json.is_object() || !json.contains(key) || !json[key].is_number()) {
        return defaultValue;
    }
    
    return json[key].get<double>();
}

bool JsonUtils::jsonGetBool(const nlohmann::json& json, const std::string& key, bool defaultValue) {
    if (!json.is_object() || !json.contains(key) || !json[key].is_boolean()) {
        return defaultValue;
    }
    
    return json[key].get<bool>();
}

std::vector<std::string> JsonUtils::jsonGetStringArray(const nlohmann::json& json, const std::string& key) {
    std::vector<std::string> result;
    
    if (!json.is_object() || !json.contains(key) || !json[key].is_array()) {
        return result;
    }
    
    for (const auto& item : json[key]) {
        if (item.is_string()) {
            result.push_back(item.get<std::string>());
        }
    }
    
    return result;
}

std::map<std::string, std::string> JsonUtils::jsonGetStringMap(const nlohmann::json& json, const std::string& key) {
    std::map<std::string, std::string> result;
    
    if (!json.is_object() || !json.contains(key) || !json[key].is_object()) {
        return result;
    }
    
    for (auto it = json[key].begin(); it != json[key].end(); ++it) {
        if (it.value().is_string()) {
            result[it.key()] = it.value().get<std::string>();
        }
    }
    
    return result;
}

} // namespace Utils
} // namespace Yachiyo