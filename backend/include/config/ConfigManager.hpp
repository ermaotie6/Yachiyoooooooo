#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include "../utils/JsonUtils.hpp"

using json = nlohmann::json;

namespace yachiyo::config {

/**
 * @brief 配置管理器
 */
class ConfigManager {
private:
    static std::shared_ptr<ConfigManager> instance;
    std::unordered_map<std::string, json> configCache;
    std::string configPath;
    std::string environment;
    
    ConfigManager();
    
public:
    ~ConfigManager() = default;
    
    // 删除拷贝构造函数和赋值运算符
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    /**
     * @brief 获取单例实例
     */
    static std::shared_ptr<ConfigManager> getInstance();
    
    /**
     * @brief 初始化配置管理器
     */
    void initialize(const std::string& configDir = "", const std::string& env = "");
    
    /**
     * @brief 加载配置文件
     */
    json loadConfig(const std::string& configName);
    
    /**
     * @brief 获取配置值
     */
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) {
        std::vector<std::string> parts;
        std::stringstream ss(key);
        std::string part;
        
        // 分割key
        while (std::getline(ss, part, '.')) {
            parts.push_back(part);
        }
        
        if (parts.empty()) {
            return defaultValue;
        }
        
        // 加载主配置文件
        json config = loadConfig("application");
        
        // 遍历配置树
        json current = config;
        for (const auto& p : parts) {
            if (current.contains(p)) {
                current = current[p];
            } else {
                return defaultValue;
            }
        }
        
        try {
            return current.get<T>();
        } catch (const json::exception&) {
            return defaultValue;
        }
    }
    
    /**
     * @brief 获取字符串配置值
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "");
    
    /**
     * @brief 获取整数配置值
     */
    int getInt(const std::string& key, int defaultValue = 0);
    
    /**
     * @brief 获取布尔配置值
     */
    bool getBool(const std::string& key, bool defaultValue = false);
    
    /**
     * @brief 获取浮点数配置值
     */
    double getDouble(const std::string& key, double defaultValue = 0.0);
    
    /**
     * @brief 获取数组配置值
     */
    json getArray(const std::string& key);
    
    /**
     * @brief 获取对象配置值
     */
    json getObject(const std::string& key);
    
    /**
     * @brief 重新加载所有配置
     */
    void reload();
    
    /**
     * @brief 获取当前环境
     */
    std::string getEnvironment() const { return environment; }
    
    /**
     * @brief 获取配置路径
     */
    std::string getConfigPath() const { return configPath; }
    
    /**
     * @brief 检查是否是开发环境
     */
    bool isDevelopment() const { return environment == "dev" || environment == "development"; }
    
    /**
     * @brief 检查是否是生产环境
     */
    bool isProduction() const { return environment == "prod" || environment == "production"; }
    
    /**
     * @brief 检查是否是测试环境
     */
    bool isTest() const { return environment == "test"; }
    
private:
    /**
     * @brief 从YAML文件加载配置
     */
    json loadYaml(const std::string& filename);
    
    /**
     * @brief 从JSON文件加载配置
     */
    json loadJson(const std::string& filename);
    
    /**
     * @brief 合并配置
     */
    json mergeConfigs(const json& base, const json& override);
    
    /**
     * @brief 解析环境变量
     */
    void parseEnvironmentVariables(json& config);
    
    /**
     * @brief 递归解析环境变量
     */
    void parseEnvRecursive(json& node, const std::string& prefix = "");
};

} // namespace yachiyo::config