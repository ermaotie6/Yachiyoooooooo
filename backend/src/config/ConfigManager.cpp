#include "config/ConfigManager.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>

namespace yachiyo::config {

// ==================== 静态成员初始化 ====================

std::shared_ptr<ConfigManager> ConfigManager::instance = nullptr;

// ==================== 构造函数 ====================

ConfigManager::ConfigManager()
    : configPath("config"), environment("dev") {}

// ==================== 单例获取 ====================

std::shared_ptr<ConfigManager> ConfigManager::getInstance() {
    if (!instance) {
        instance = std::shared_ptr<ConfigManager>(new ConfigManager());
    }
    return instance;
}

// ==================== 初始化 ====================

void ConfigManager::initialize(const std::string& configDir, const std::string& env) {
    if (!configDir.empty()) {
        configPath = configDir;
    }
    if (!env.empty()) {
        environment = env;
    }

    // 预加载主配置文件
    configCache.clear();
    loadConfig("config");
}

// ==================== 加载配置 ====================

json ConfigManager::loadConfig(const std::string& configName) {
    // 检查缓存
    auto it = configCache.find(configName);
    if (it != configCache.end()) {
        return it->second;
    }

    json config = json::object();

    // 尝试加载 YAML 文件
    std::string yamlPath = configPath + "/" + configName + ".yaml";
    if (std::filesystem::exists(yamlPath)) {
        config = loadYaml(yamlPath);
    } else {
        // 尝试加载 JSON 文件
        std::string jsonPath = configPath + "/" + configName + ".json";
        if (std::filesystem::exists(jsonPath)) {
            config = loadJson(jsonPath);
        }
    }

    // 加载环境特定配置并合并
    if (!environment.empty()) {
        std::string envYamlPath = configPath + "/" + configName + "." + environment + ".yaml";
        if (std::filesystem::exists(envYamlPath)) {
            json envConfig = loadYaml(envYamlPath);
            config = mergeConfigs(config, envConfig);
        }
    }

    // 解析环境变量
    parseEnvironmentVariables(config);

    // 缓存
    configCache[configName] = config;
    return config;
}

// ==================== 类型化获取方法 ====================

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) {
    return get<std::string>(key, defaultValue);
}

int ConfigManager::getInt(const std::string& key, int defaultValue) {
    return get<int>(key, defaultValue);
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) {
    return get<bool>(key, defaultValue);
}

double ConfigManager::getDouble(const std::string& key, double defaultValue) {
    return get<double>(key, defaultValue);
}

json ConfigManager::getArray(const std::string& key) {
    return get<json>(key, json::array());
}

json ConfigManager::getObject(const std::string& key) {
    return get<json>(key, json::object());
}

// ==================== 重新加载 ====================

void ConfigManager::reload() {
    // 清除缓存并重新加载
    auto cachedKeys = std::vector<std::string>();
    for (const auto& pair : configCache) {
        cachedKeys.push_back(pair.first);
    }
    configCache.clear();
    for (const auto& key : cachedKeys) {
        loadConfig(key);
    }
}

// ==================== 私有方法 ====================

json ConfigManager::loadYaml(const std::string& filename) {
    try {
        YAML::Node yamlNode = YAML::LoadFile(filename);
        // 递归转换 YAML → JSON
        std::function<json(const YAML::Node&)> yamlToJson = [&](const YAML::Node& node) -> json {
            switch (node.Type()) {
                case YAML::NodeType::Null:
                    return json(nullptr);
                case YAML::NodeType::Scalar: {
                    // 尝试解析为不同类型
                    const std::string& scalar = node.Scalar();
                    // 布尔值
                    if (scalar == "true" || scalar == "True" || scalar == "TRUE") return json(true);
                    if (scalar == "false" || scalar == "False" || scalar == "FALSE") return json(false);
                    // 整数
                    try {
                        size_t pos;
                        long long intVal = std::stoll(scalar, &pos);
                        if (pos == scalar.size()) return json(intVal);
                    } catch (...) {}
                    // 浮点数
                    try {
                        size_t pos;
                        double dblVal = std::stod(scalar, &pos);
                        if (pos == scalar.size()) return json(dblVal);
                    } catch (...) {}
                    // 字符串
                    return json(scalar);
                }
                case YAML::NodeType::Sequence: {
                    json arr = json::array();
                    for (const auto& item : node) {
                        arr.push_back(yamlToJson(item));
                    }
                    return arr;
                }
                case YAML::NodeType::Map: {
                    json obj = json::object();
                    for (const auto& pair : node) {
                        obj[pair.first.as<std::string>()] = yamlToJson(pair.second);
                    }
                    return obj;
                }
                default:
                    return json(nullptr);
            }
        };
        return yamlToJson(yamlNode);
    } catch (const std::exception& e) {
        std::cerr << "加载 YAML 配置文件失败: " << filename << " - " << e.what() << std::endl;
        return json::object();
    }
}

json ConfigManager::loadJson(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开 JSON 配置文件: " << filename << std::endl;
            return json::object();
        }
        json config;
        file >> config;
        return config;
    } catch (const std::exception& e) {
        std::cerr << "加载 JSON 配置文件失败: " << filename << " - " << e.what() << std::endl;
        return json::object();
    }
}

json ConfigManager::mergeConfigs(const json& base, const json& override) {
    json result = base;
    for (auto it = override.begin(); it != override.end(); ++it) {
        if (result.contains(it.key()) && result[it.key()].is_object() && it.value().is_object()) {
            result[it.key()] = mergeConfigs(result[it.key()], it.value());
        } else {
            result[it.key()] = it.value();
        }
    }
    return result;
}

void ConfigManager::parseEnvironmentVariables(json& config) {
    parseEnvRecursive(config);
}

void ConfigManager::parseEnvRecursive(json& node, const std::string& prefix) {
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            parseEnvRecursive(it.value(), prefix.empty() ? it.key() : prefix + "." + it.key());
        }
    } else if (node.is_string()) {
        std::string value = node.get<std::string>();
        // 匹配 ${ENV_VAR} 或 ${ENV_VAR:default} 模式
        if (value.size() >= 4 && value.substr(0, 2) == "${" && value.back() == '}') {
            std::string envExpr = value.substr(2, value.size() - 3);
            std::string envName;
            std::string defaultVal;
            auto colonPos = envExpr.find(':');
            if (colonPos != std::string::npos) {
                envName = envExpr.substr(0, colonPos);
                defaultVal = envExpr.substr(colonPos + 1);
            } else {
                envName = envExpr;
            }
            // 从系统环境变量读取
            const char* envValue = std::getenv(envName.c_str());
            if (envValue) {
                node = std::string(envValue);
            } else if (!defaultVal.empty()) {
                node = defaultVal;
            }
            // 否则保持原始 ${...} 字符串
        }
    }
}

} // namespace yachiyo::config
