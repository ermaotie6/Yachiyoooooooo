#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <yaml-cpp/yaml.h>
#include "utils/HttpServer.hpp"
#include "utils/RedisUtil.hpp"

namespace Yachiyo {
namespace Utils {

// 应用配置
struct AppConfig {
    // 服务器配置
    HttpServerConfig server;
    
    // 数据库配置
    struct DatabaseConfig {
        std::string type = "mysql";
        std::string host = "localhost";
        int port = 3306;
        std::string name = "yachiyo_cpp";
        std::string username = "root";
        std::string password;
        int maxConnections = 20;
        int connectionTimeout = 10;
        int reconnectAttempts = 3;
    } database;
    
    // Redis配置
    RedisConfig redis;
    
    // JWT配置
    struct JWTConfig {
        std::string secret = "yachiyo-cpp-secret-key-change-in-production";
        int expirationHours = 24;
        std::string issuer = "yachiyo-cpp";
        std::string audience = "yachiyo-users";
    } jwt;
    
    // AI配置
    struct AIConfig {
        // OpenAI配置
        struct OpenAIConfig {
            std::string apiKey;
            std::string baseUrl = "https://api.openai.com/v1";
            std::string model = "gpt-3.5-turbo";
            double temperature = 0.7;
            int maxTokens = 1000;
        } openai;
        
        // Ollama配置
        struct OllamaConfig {
            std::string baseUrl = "http://localhost:11434";
            std::string model = "llama2";
            double temperature = 0.7;
            int maxTokens = 1000;
        } ollama;
        
        // 语音合成配置
        struct TTSConfig {
            std::string provider = "openai";  // openai, elevenlabs, local
            std::string voice = "alloy";
            double speed = 1.0;
        } tts;
        
        // 默认提供商
        std::string defaultProvider = "ollama";
    } ai;
    
    // 邮件配置
    struct EmailConfig {
        std::string smtpHost = "smtp.gmail.com";
        int smtpPort = 587;
        std::string username;
        std::string password;
        std::string fromAddress = "noreply@yachiyo.com";
        bool useTLS = true;
    } email;
    
    // 日志配置
    struct LoggingConfig {
        std::string level = "info";  // debug, info, warn, error
        std::string filePath = "logs/yachiyo.log";
        int maxSizeMB = 100;
        int maxFiles = 10;
        bool consoleOutput = true;
    } logging;
    
    // 安全配置
    struct SecurityConfig {
        int passwordMinLength = 8;
        bool passwordRequireUppercase = true;
        bool passwordRequireLowercase = true;
        bool passwordRequireDigits = true;
        bool passwordRequireSpecial = true;
        int loginAttemptsLimit = 5;
        int loginLockoutMinutes = 30;
        bool corsEnabled = true;
        std::string corsOrigins = "*";
    } security;
    
    // 缓存配置
    struct CacheConfig {
        bool enabled = true;
        int defaultTtlSeconds = 3600;
        int userCacheTtl = 1800;
        int conversationCacheTtl = 7200;
    } cache;
    
    // 性能配置
    struct PerformanceConfig {
        int maxRequestSizeMB = 10;
        int maxResponseTimeMs = 5000;
        bool rateLimitEnabled = true;
        int rateLimitRequestsPerMinute = 60;
        bool compressionEnabled = true;
    } performance;
    
    // 监控配置
    struct MonitoringConfig {
        bool enabled = true;
        int metricsPort = 9090;
        std::string healthCheckPath = "/health";
        bool prometheusEnabled = true;
    } monitoring;
};

/**
 * @brief 配置解析器
 */
class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();

    /**
     * @brief 从文件加载配置
     * @param filepath 配置文件路径
     * @return 是否加载成功
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * @brief 从字符串加载配置
     * @param yamlString YAML字符串
     * @return 是否加载成功
     */
    bool loadFromString(const std::string& yamlString);

    /**
     * @brief 获取应用配置
     * @return 应用配置
     */
    const AppConfig& getConfig() const;

    /**
     * @brief 更新配置
     * @param config 新配置
     */
    void updateConfig(const AppConfig& config);

    /**
     * @brief 保存配置到文件
     * @param filepath 文件路径
     * @return 是否保存成功
     */
    bool saveToFile(const std::string& filepath);

    /**
     * @brief 将配置转换为YAML字符串
     * @return YAML字符串
     */
    std::string toString() const;

    /**
     * @brief 获取配置值
     * @tparam T 值类型
     * @param key 配置键（支持点分隔符，如 "server.port"）
     * @param defaultValue 默认值
     * @return 配置值
     */
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue = T()) const;

    /**
     * @brief 设置配置值
     * @tparam T 值类型
     * @param key 配置键
     * @param value 配置值
     * @return 是否设置成功
     */
    template<typename T>
    bool setValue(const std::string& key, const T& value);

    /**
     * @brief 检查配置键是否存在
     * @param key 配置键
     * @return 是否存在
     */
    bool hasKey(const std::string& key) const;

    /**
     * @brief 重新加载配置
     * @return 是否重新加载成功
     */
    bool reload();

    /**
     * @brief 验证配置
     * @return 验证结果和错误信息
     */
    std::pair<bool, std::string> validate() const;

    /**
     * @brief 获取环境变量覆盖的配置
     * @param prefix 环境变量前缀
     * @return 应用配置
     */
    AppConfig getConfigWithEnvOverrides(const std::string& prefix = "YACHIYO_") const;

private:
    /**
     * @brief 解析YAML节点
     * @param node YAML节点
     * @param config 应用配置
     */
    void parseYAMLNode(const YAML::Node& node, AppConfig& config);

    /**
     * @brief 解析服务器配置
     * @param node YAML节点
     * @param config 服务器配置
     */
    void parseServerConfig(const YAML::Node& node, HttpServerConfig& config);

    /**
     * @brief 解析数据库配置
     * @param node YAML节点
     * @param config 数据库配置
     */
    void parseDatabaseConfig(const YAML::Node& node, AppConfig::DatabaseConfig& config);

    /**
     * @brief 解析Redis配置
     * @param node YAML节点
     * @param config Redis配置
     */
    void parseRedisConfig(const YAML::Node& node, RedisConfig& config);

    /**
     * @brief 解析JWT配置
     * @param node YAML节点
     * @param config JWT配置
     */
    void parseJWTConfig(const YAML::Node& node, AppConfig::JWTConfig& config);

    /**
     * @brief 解析AI配置
     * @param node YAML节点
     * @param config AI配置
     */
    void parseAIConfig(const YAML::Node& node, AppConfig::AIConfig& config);

    /**
     * @brief 解析邮件配置
     * @param node YAML节点
     * @param config 邮件配置
     */
    void parseEmailConfig(const YAML::Node& node, AppConfig::EmailConfig& config);

    /**
     * @brief 将应用配置转换为YAML节点
     * @param config 应用配置
     * @return YAML节点
     */
    YAML::Node configToYAMLNode(const AppConfig& config) const;

    /**
     * @brief 将服务器配置转换为YAML节点
     * @param config 服务器配置
     * @return YAML节点
     */
    YAML::Node serverConfigToYAMLNode(const HttpServerConfig& config) const;

    /**
     * @brief 将数据库配置转换为YAML节点
     * @param config 数据库配置
     * @return YAML节点
     */
    YAML::Node databaseConfigToYAMLNode(const AppConfig::DatabaseConfig& config) const;

    /**
     * @brief 将Redis配置转换为YAML节点
     * @param config Redis配置
     * @return YAML节点
     */
    YAML::Node redisConfigToYAMLNode(const RedisConfig& config) const;

    /**
     * @brief 获取环境变量值
     * @param key 环境变量键
     * @return 环境变量值
     */
    std::string getEnv(const std::string& key) const;

    // 配置数据
    AppConfig config;
    
    // 配置文件路径
    std::string configFilePath;
    
    // YAML文档
    YAML::Node yamlRoot;
    
    // 最后修改时间
    std::time_t lastModifiedTime;
};

/**
 * @brief 配置管理器
 */
class ConfigManager {
public:
    static ConfigManager& getInstance();

    /**
     * @brief 加载配置
     * @param filepath 配置文件路径
     * @return 是否加载成功
     */
    bool loadConfig(const std::string& filepath = "config/config.yaml");

    /**
     * @brief 获取配置解析器
     * @return 配置解析器
     */
    std::shared_ptr<ConfigParser> getParser();

    /**
     * @brief 获取应用配置
     * @return 应用配置
     */
    AppConfig getAppConfig() const;

    /**
     * @brief 获取服务器配置
     * @return 服务器配置
     */
    HttpServerConfig getServerConfig() const;

    /**
     * @brief 获取数据库配置
     * @return 数据库配置
     */
    AppConfig::DatabaseConfig getDatabaseConfig() const;

    /**
     * @brief 获取Redis配置
     * @return Redis配置
     */
    RedisConfig getRedisConfig() const;

    /**
     * @brief 获取JWT配置
     * @return JWT配置
     */
    AppConfig::JWTConfig getJWTConfig() const;

    /**
     * @brief 获取AI配置
     * @return AI配置
     */
    AppConfig::AIConfig getAIConfig() const;

    /**
     * @brief 监听配置变化
     * @param callback 变化回调函数
     */
    void watchConfigChanges(std::function<void(const AppConfig&)> callback);

    /**
     * @brief 停止监听配置变化
     */
    void stopWatchingConfigChanges();

private:
    ConfigManager();
    ~ConfigManager();

    // 配置变化监听线程
    void watchThreadFunc();

    // 配置解析器
    std::shared_ptr<ConfigParser> parser;
    
    // 配置文件路径
    std::string configFilePath;
    
    // 配置变化回调
    std::function<void(const AppConfig&)> configChangeCallback;
    
    // 监听线程
    std::thread watchThread;
    std::atomic<bool> watching;
    
    // 线程安全
    mutable std::mutex mutex;
};

} // namespace Utils
} // namespace Yachiyo