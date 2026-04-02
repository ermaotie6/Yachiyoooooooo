#include "utils/ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace Yachiyo {
namespace Utils {

ConfigParser::ConfigParser() {
    // 默认配置
    config["server"]["host"] = "0.0.0.0";
    config["server"]["port"] = "8080";
    config["server"]["threads"] = "4";
    
    config["database"]["host"] = "localhost";
    config["database"]["port"] = "5432";
    config["database"]["name"] = "yachiyo";
    config["database"]["user"] = "postgres";
    config["database"]["password"] = "password";
    config["database"]["pool_size"] = "10";
    
    config["redis"]["host"] = "localhost";
    config["redis"]["port"] = "6379";
    config["redis"]["password"] = "";
    config["redis"]["db"] = "0";
    
    config["jwt"]["secret"] = "your-secret-key-change-this";
    config["jwt"]["expiration"] = "86400";
    
    config["ai"]["api_key"] = "";
    config["ai"]["model"] = "gpt-3.5-turbo";
    config["ai"]["temperature"] = "0.7";
    config["ai"]["max_tokens"] = "1000";
    
    config["logging"]["level"] = "info";
    config["logging"]["file"] = "logs/app.log";
    config["logging"]["max_size"] = "10485760";
    config["logging"]["max_files"] = "5";
    
    config["security"]["cors_origins"] = "*";
    config["security"]["rate_limit"] = "100";
    config["security"]["rate_limit_window"] = "60";
}

bool ConfigParser::loadFromFile(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filePath << std::endl;
            return false;
        }
        
        std::string line;
        std::string currentSection = "global";
        
        while (std::getline(file, line)) {
            // 移除注释
            size_t commentPos = line.find('#');
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }
            
            // 修剪空白字符
            line = trim(line);
            
            if (line.empty()) {
                continue;
            }
            
            // 检查是否是节定义
            if (line.front() == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
                currentSection = trim(currentSection);
                continue;
            }
            
            // 解析键值对
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = trim(line.substr(0, equalsPos));
                std::string value = trim(line.substr(equalsPos + 1));
                
                // 处理嵌套键（使用点号分隔）
                std::vector<std::string> keyParts = splitKey(key);
                
                if (keyParts.empty()) {
                    continue;
                }
                
                // 构建配置路径
                std::string fullKey = currentSection;
                for (const auto& part : keyParts) {
                    fullKey += "." + part;
                }
                
                // 设置值
                setValue(fullKey, value);
            }
        }
        
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigParser::loadFromString(const std::string& content) {
    try {
        std::stringstream ss(content);
        std::string line;
        std::string currentSection = "global";
        
        while (std::getline(ss, line)) {
            // 移除注释
            size_t commentPos = line.find('#');
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }
            
            // 修剪空白字符
            line = trim(line);
            
            if (line.empty()) {
                continue;
            }
            
            // 检查是否是节定义
            if (line.front() == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
                currentSection = trim(currentSection);
                continue;
            }
            
            // 解析键值对
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = trim(line.substr(0, equalsPos));
                std::string value = trim(line.substr(equalsPos + 1));
                
                // 处理嵌套键（使用点号分隔）
                std::vector<std::string> keyParts = splitKey(key);
                
                if (keyParts.empty()) {
                    continue;
                }
                
                // 构建配置路径
                std::string fullKey = currentSection;
                for (const auto& part : keyParts) {
                    fullKey += "." + part;
                }
                
                // 设置值
                setValue(fullKey, value);
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading config string: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigParser::saveToFile(const std::string& filePath) {
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to create config file: " << filePath << std::endl;
            return false;
        }
        
        // 按节分组配置项
        std::map<std::string, std::map<std::string, std::string>> sections;
        
        for (const auto& [fullKey, value] : config) {
            size_t dotPos = fullKey.find('.');
            
            if (dotPos == std::string::npos) {
                // 全局配置
                sections["global"][fullKey] = value;
            } else {
                std::string section = fullKey.substr(0, dotPos);
                std::string key = fullKey.substr(dotPos + 1);
                sections[section][key] = value;
            }
        }
        
        // 写入文件
        for (const auto& [section, items] : sections) {
            if (section != "global") {
                file << "[" << section << "]" << std::endl;
            }
            
            for (const auto& [key, value] : items) {
                file << key << " = " << value << std::endl;
            }
            
            file << std::endl;
        }
        
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving config file: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigParser::saveToString() {
    try {
        std::stringstream ss;
        
        // 按节分组配置项
        std::map<std::string, std::map<std::string, std::string>> sections;
        
        for (const auto& [fullKey, value] : config) {
            size_t dotPos = fullKey.find('.');
            
            if (dotPos == std::string::npos) {
                // 全局配置
                sections["global"][fullKey] = value;
            } else {
                std::string section = fullKey.substr(0, dotPos);
                std::string key = fullKey.substr(dotPos + 1);
                sections[section][key] = value;
            }
        }
        
        // 构建字符串
        for (const auto& [section, items] : sections) {
            if (section != "global") {
                ss << "[" << section << "]" << std::endl;
            }
            
            for (const auto& [key, value] : items) {
                ss << key << " = " << value << std::endl;
            }
            
            ss << std::endl;
        }
        
        return ss.str();
        
    } catch (const std::exception& e) {
        std::cerr << "Error serializing config: " << e.what() << std::endl;
        return "";
    }
}

std::string ConfigParser::get(const std::string& key, const std::string& defaultValue) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = config.find(key);
    if (it != config.end()) {
        return it->second;
    }
    
    return defaultValue;
}

bool ConfigParser::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        config[key] = value;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error setting config key '" << key << "': " << e.what() << std::endl;
        return false;
    }
}

bool ConfigParser::has(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);
    return config.find(key) != config.end();
}

void ConfigParser::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    config.clear();
}

std::vector<std::string> ConfigParser::getKeys() {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<std::string> keys;
    for (const auto& [key, value] : config) {
        keys.push_back(key);
    }
    
    return keys;
}

std::map<std::string, std::string> ConfigParser::getAll() {
    std::lock_guard<std::mutex> lock(mutex);
    return config;
}

std::string ConfigParser::trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    // 修剪前导空白字符
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }
    
    // 修剪尾随空白字符
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        end--;
    }
    
    return str.substr(start, end - start);
}

std::vector<std::string> ConfigParser::splitKey(const std::string& key) {
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;
    
    while (std::getline(ss, part, '.')) {
        part = trim(part);
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    return parts;
}

void ConfigParser::setValue(const std::string& fullKey, const std::string& value) {
    // 处理环境变量引用
    std::string processedValue = expandEnvironmentVariables(value);
    
    // 处理布尔值
    std::string normalizedValue = normalizeValue(processedValue);
    
    config[fullKey] = normalizedValue;
}

std::string ConfigParser::expandEnvironmentVariables(const std::string& value) {
    std::string result = value;
    size_t startPos = 0;
    
    while ((startPos = result.find("${", startPos)) != std::string::npos) {
        size_t endPos = result.find("}", startPos);
        if (endPos == std::string::npos) {
            break;
        }
        
        std::string varName = result.substr(startPos + 2, endPos - startPos - 2);
        const char* envValue = std::getenv(varName.c_str());
        
        if (envValue != nullptr) {
            result.replace(startPos, endPos - startPos + 1, envValue);
            startPos += strlen(envValue);
        } else {
            // 环境变量未找到，保留原样
            startPos = endPos + 1;
        }
    }
    
    return result;
}

std::string ConfigParser::normalizeValue(const std::string& value) {
    std::string lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    if (lowerValue == "true" || lowerValue == "yes" || lowerValue == "on") {
        return "true";
    }
    
    if (lowerValue == "false" || lowerValue == "no" || lowerValue == "off") {
        return "false";
    }
    
    // 尝试解析为数字
    try {
        // 检查是否是整数
        std::stoi(value);
        return value;
    } catch (...) {
        // 不是整数，继续
    }
    
    try {
        // 检查是否是浮点数
        std::stod(value);
        return value;
    } catch (...) {
        // 不是数字，返回原始值
    }
    
    return value;
}

} // namespace Utils
} // namespace Yachiyo