#include "utils/ConfigManager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace Yachiyo {
namespace Utils {

ConfigManager::ConfigManager() {
    // 默认配置
    config["server"]["host"] = "0.0.0.0";
    config["server"]["port"] = 8080;
    config["server"]["threads"] = 4;
    
    config["database"]["host"] = "localhost";
    config["database"]["port"] = 5432;
    config["database"]["name"] = "yachiyo";
    config["database"]["user"] = "postgres";
    config["database"]["password"] = "password";
    config["database"]["pool_size"] = 10;
    
    config["redis"]["host"] = "localhost";
    config["redis"]["port"] = 6379;
    config["redis"]["password"] = "";
    config["redis"]["db"] = 0;
    
    config["jwt"]["secret"] = "your-secret-key-change-this";
    config["jwt"]["expiration"] = 86400; // 24小时
    
    config["ai"]["api_key"] = "";
    config["ai"]["model"] = "gpt-3.5-turbo";
    config["ai"]["temperature"] = 0.7;
    config["ai"]["max_tokens"] = 1000;
    
    config["logging"]["level"] = "info";
    config["logging"]["file"] = "logs/app.log";
    config["logging"]["max_size"] = 10485760; // 10MB
    config["logging"]["max_files"] = 5;
    
    config["security"]["cors_origins"] = "*";
    config["security"]["rate_limit"] = 100;
    config["security"]["rate_limit_window"] = 60; // 秒
}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadFromFile(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filePath << std::endl;
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        return loadFromString(content);
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::loadFromString(const std::string& content) {
    try {
        YAML::Node newConfig = YAML::Load(content);
        
        // 合并配置
        mergeConfig(config, newConfig);
        
        // 验证配置
        if (!validateConfig()) {
            std::cerr << "Config validation failed" << std::endl;
            return false;
        }
        
        // 触发配置变更事件
        notifyConfigChanged();
        
        return true;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "YAML parsing error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::saveToFile(const std::string& filePath) {
    try {
        // 确保目录存在
        std::filesystem::path path(filePath);
        std::filesystem::create_directories(path.parent_path());
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to create config file: " << filePath << std::endl;
            return false;
        }
        
        file << YAML::Dump(config);
        file.close();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving config file: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigManager::saveToString() {
    try {
        return YAML::Dump(config);
    } catch (const std::exception& e) {
        std::cerr << "Error serializing config: " << e.what() << std::endl;
        return "";
    }
}

YAML::Node ConfigManager::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        return getNodeByPath(key);
    } catch (const std::exception& e) {
        std::cerr << "Error getting config key '" << key << "': " << e.what() << std::endl;
        return YAML::Node();
    }
}

bool ConfigManager::set(const std::string& key, const YAML::Node& value) {
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        setNodeByPath(key, value);
        
        // 验证配置
        if (!validateConfig()) {
            std::cerr << "Config validation failed after setting key: " << key << std::endl;
            return false;
        }
        
        // 触发配置变更事件
        notifyConfigChanged();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error setting config key '" << key << "': " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::has(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        YAML::Node node = getNodeByPath(key);
        return node.IsDefined() && !node.IsNull();
    } catch (const std::exception& e) {
        return false;
    }
}

void ConfigManager::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    config = YAML::Node();
}

void ConfigManager::addListener(ConfigChangeListener listener) {
    std::lock_guard<std::mutex> lock(mutex);
    listeners.push_back(listener);
}

void ConfigManager::removeListener(ConfigChangeListener listener) {
    std::lock_guard<std::mutex> lock(mutex);
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(),
            [&listener](const ConfigChangeListener& l) {
                // 比较函数指针
                return l.target_type() == listener.target_type();
            }),
        listeners.end()
    );
}

YAML::Node ConfigManager::getNodeByPath(const std::string& path) {
    std::vector<std::string> keys = splitPath(path);
    YAML::Node currentNode = config;
    
    for (const auto& key : keys) {
        if (currentNode.IsMap()) {
            currentNode = currentNode[key];
        } else {
            throw std::runtime_error("Invalid config path: " + path);
        }
    }
    
    return currentNode;
}

void ConfigManager::setNodeByPath(const std::string& path, const YAML::Node& value) {
    std::vector<std::string> keys = splitPath(path);
    
    if (keys.empty()) {
        config = value;
        return;
    }
    
    YAML::Node currentNode = config;
    
    // 遍历到倒数第二个键
    for (size_t i = 0; i < keys.size() - 1; i++) {
        const auto& key = keys[i];
        
        if (!currentNode[key]) {
            currentNode[key] = YAML::Node(YAML::NodeType::Map);
        }
        
        currentNode = currentNode[key];
    }
    
    // 设置最后一个键的值
    currentNode[keys.back()] = value;
}

std::vector<std::string> ConfigManager::splitPath(const std::string& path) {
    std::vector<std::string> keys;
    std::stringstream ss(path);
    std::string key;
    
    while (std::getline(ss, key, '.')) {
        if (!key.empty()) {
            keys.push_back(key);
        }
    }
    
    return keys;
}

void ConfigManager::mergeConfig(YAML::Node& target, const YAML::Node& source) {
    if (!source.IsMap()) {
        target = source;
        return;
    }
    
    if (!target.IsMap()) {
        target = YAML::Node(YAML::NodeType::Map);
    }
    
    for (const auto& pair : source) {
        const std::string& key = pair.first.as<std::string>();
        const YAML::Node& value = pair.second;
        
        if (target[key] && target[key].IsMap() && value.IsMap()) {
            // 递归合并映射
            mergeConfig(target[key], value);
        } else {
            // 直接赋值
            target[key] = value;
        }
    }
}

bool ConfigManager::validateConfig() {
    try {
        // 验证服务器配置
        if (!config["server"]["port"].IsDefined() || !config["server"]["port"].IsScalar()) {
            std::cerr << "Invalid server port configuration" << std::endl;
            return false;
        }
        
        int port = config["server"]["port"].as<int>();
        if (port < 1 || port > 65535) {
            std::cerr << "Invalid server port: " << port << std::endl;
            return false;
        }
        
        // 验证数据库配置
        if (!config["database"]["host"].IsDefined() || config["database"]["host"].as<std::string>().empty()) {
            std::cerr << "Database host is required" << std::endl;
            return false;
        }
        
        if (!config["database"]["name"].IsDefined() || config["database"]["name"].as<std::string>().empty()) {
            std::cerr << "Database name is required" << std::endl;
            return false;
        }
        
        // 验证JWT配置
        if (!config["jwt"]["secret"].IsDefined() || config["jwt"]["secret"].as<std::string>().empty()) {
            std::cerr << "JWT secret is required" << std::endl;
            return false;
        }
        
        // 验证日志配置
        if (config["logging"]["level"].IsDefined()) {
            std::string level = config["logging"]["level"].as<std::string>();
            std::vector<std::string> validLevels = {"trace", "debug", "info", "warn", "error", "fatal"};
            
            if (std::find(validLevels.begin(), validLevels.end(), level) == validLevels.end()) {
                std::cerr << "Invalid log level: " << level << std::endl;
                return false;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Config validation error: " << e.what() << std::endl;
        return false;
    }
}

void ConfigManager::notifyConfigChanged() {
    for (const auto& listener : listeners) {
        try {
            listener(config);
        } catch (const std::exception& e) {
            std::cerr << "Error in config change listener: " << e.what() << std::endl;
        }
    }
}

} // namespace Utils
} // namespace Yachiyo