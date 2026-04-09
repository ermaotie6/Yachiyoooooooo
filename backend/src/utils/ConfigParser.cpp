#include "utils/ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sys/stat.h>

namespace Yachiyo {
namespace Utils {

// ==================== ConfigParser ====================

ConfigParser::ConfigParser() 
    : lastModifiedTime(0) {
}

ConfigParser::~ConfigParser() = default;

bool ConfigParser::loadFromFile(const std::string& filepath) {
    try {
        configFilePath = filepath;
        yamlRoot = YAML::LoadFile(filepath);
        
        // 解析到 AppConfig
        parseYAMLNode(yamlRoot, config);
        
        // 记录文件修改时间
        struct stat fileStat;
        if (stat(filepath.c_str(), &fileStat) == 0) {
            lastModifiedTime = fileStat.st_mtime;
        }
        
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "YAML 配置文件解析失败 (" << filepath << "): " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "加载配置文件失败 (" << filepath << "): " << e.what() << std::endl;
        return false;
    }
}

bool ConfigParser::loadFromString(const std::string& yamlString) {
    try {
        yamlRoot = YAML::Load(yamlString);
        parseYAMLNode(yamlRoot, config);
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "YAML 字符串解析失败: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "加载配置字符串失败: " << e.what() << std::endl;
        return false;
    }
}

const AppConfig& ConfigParser::getConfig() const {
    return config;
}

void ConfigParser::updateConfig(const AppConfig& newConfig) {
    config = newConfig;
}

bool ConfigParser::saveToFile(const std::string& filepath) {
    try {
        YAML::Node node = configToYAMLNode(config);
        std::ofstream fout(filepath);
        if (!fout.is_open()) {
            std::cerr << "无法打开文件进行写入: " << filepath << std::endl;
            return false;
        }
        fout << node;
        fout.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存配置文件失败: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigParser::toString() const {
    try {
        YAML::Node node = configToYAMLNode(config);
        YAML::Emitter emitter;
        emitter << node;
        return emitter.c_str();
    } catch (const std::exception& e) {
        std::cerr << "序列化配置失败: " << e.what() << std::endl;
        return "";
    }
}

bool ConfigParser::hasKey(const std::string& key) const {
    // 支持点分隔键: "server.port"
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }
    
    YAML::Node current = yamlRoot;
    for (const auto& p : parts) {
        if (!current || !current.IsMap() || !current[p]) {
            return false;
        }
        current = current[p];
    }
    return current.IsDefined();
}

bool ConfigParser::reload() {
    if (configFilePath.empty()) {
        return false;
    }
    return loadFromFile(configFilePath);
}

std::pair<bool, std::string> ConfigParser::validate() const {
    std::string errors;
    
    // 验证服务器端口范围
    if (config.server.port <= 0 || config.server.port > 65535) {
        errors += "无效的服务器端口: " + std::to_string(config.server.port) + "\n";
    }
    
    // 验证数据库配置
    if (config.database.host.empty()) {
        errors += "数据库主机不能为空\n";
    }
    if (config.database.port <= 0 || config.database.port > 65535) {
        errors += "无效的数据库端口: " + std::to_string(config.database.port) + "\n";
    }
    
    // 验证JWT密钥
    if (config.jwt.secret.empty()) {
        errors += "JWT 密钥不能为空\n";
    }
    
    if (errors.empty()) {
        return {true, "配置验证通过"};
    }
    return {false, errors};
}

AppConfig ConfigParser::getConfigWithEnvOverrides(const std::string& prefix) const {
    AppConfig result = config;
    
    // 尝试从环境变量覆盖常见配置
    std::string envVal;
    
    envVal = getEnv(prefix + "SERVER_HOST");
    if (!envVal.empty()) result.server.host = envVal;
    
    envVal = getEnv(prefix + "SERVER_PORT");
    if (!envVal.empty()) result.server.port = std::stoi(envVal);
    
    envVal = getEnv(prefix + "DB_HOST");
    if (!envVal.empty()) result.database.host = envVal;
    
    envVal = getEnv(prefix + "DB_PORT");
    if (!envVal.empty()) result.database.port = std::stoi(envVal);
    
    envVal = getEnv(prefix + "DB_NAME");
    if (!envVal.empty()) result.database.name = envVal;
    
    envVal = getEnv(prefix + "DB_USER");
    if (!envVal.empty()) result.database.username = envVal;
    
    envVal = getEnv(prefix + "DB_PASSWORD");
    if (!envVal.empty()) result.database.password = envVal;
    
    envVal = getEnv(prefix + "REDIS_HOST");
    if (!envVal.empty()) result.redis.host = envVal;
    
    envVal = getEnv(prefix + "REDIS_PORT");
    if (!envVal.empty()) result.redis.port = std::stoi(envVal);
    
    envVal = getEnv(prefix + "REDIS_PASSWORD");
    if (!envVal.empty()) result.redis.password = envVal;
    
    envVal = getEnv(prefix + "JWT_SECRET");
    if (!envVal.empty()) result.jwt.secret = envVal;
    
    envVal = getEnv(prefix + "LOG_LEVEL");
    if (!envVal.empty()) result.logging.level = envVal;
    
    return result;
}

// ==================== 私有解析方法 ====================

void ConfigParser::parseYAMLNode(const YAML::Node& node, AppConfig& cfg) {
    if (!node.IsMap()) return;
    
    if (node["server"]) parseServerConfig(node["server"], cfg.server);
    if (node["database"]) parseDatabaseConfig(node["database"], cfg.database);
    if (node["redis"]) parseRedisConfig(node["redis"], cfg.redis);
    if (node["jwt"]) parseJWTConfig(node["jwt"], cfg.jwt);
    if (node["ai"]) parseAIConfig(node["ai"], cfg.ai);
    if (node["email"]) parseEmailConfig(node["email"], cfg.email);
    
    // 日志配置
    if (node["logging"]) {
        auto& logNode = node["logging"];
        if (logNode["level"]) cfg.logging.level = logNode["level"].as<std::string>();
        if (logNode["file"]) cfg.logging.filePath = logNode["file"].as<std::string>();
        if (logNode["max_size_mb"]) cfg.logging.maxSizeMB = logNode["max_size_mb"].as<int>();
        if (logNode["max_files"]) cfg.logging.maxFiles = logNode["max_files"].as<int>();
        if (logNode["console_output"]) cfg.logging.consoleOutput = logNode["console_output"].as<bool>();
    }
    
    // 安全配置
    if (node["security"]) {
        auto& secNode = node["security"];
        if (secNode["password_min_length"]) cfg.security.passwordMinLength = secNode["password_min_length"].as<int>();
        if (secNode["login_attempts_limit"]) cfg.security.loginAttemptsLimit = secNode["login_attempts_limit"].as<int>();
        if (secNode["login_lockout_minutes"]) cfg.security.loginLockoutMinutes = secNode["login_lockout_minutes"].as<int>();
        if (secNode["cors_enabled"]) cfg.security.corsEnabled = secNode["cors_enabled"].as<bool>();
        if (secNode["cors_origins"]) cfg.security.corsOrigins = secNode["cors_origins"].as<std::string>();
    }
    
    // 缓存配置
    if (node["cache"]) {
        auto& cacheNode = node["cache"];
        if (cacheNode["enabled"]) cfg.cache.enabled = cacheNode["enabled"].as<bool>();
        if (cacheNode["default_ttl"]) cfg.cache.defaultTtlSeconds = cacheNode["default_ttl"].as<int>();
    }
    
    // 监控配置
    if (node["monitoring"]) {
        auto& monNode = node["monitoring"];
        if (monNode["enabled"]) cfg.monitoring.enabled = monNode["enabled"].as<bool>();
        if (monNode["metrics_port"]) cfg.monitoring.metricsPort = monNode["metrics_port"].as<int>();
        if (monNode["prometheus_enabled"]) cfg.monitoring.prometheusEnabled = monNode["prometheus_enabled"].as<bool>();
    }
}

void ConfigParser::parseServerConfig(const YAML::Node& node, HttpServerConfig& cfg) {
    if (!node.IsMap()) return;
    
    if (node["host"]) cfg.host = node["host"].as<std::string>();
    if (node["port"]) cfg.port = node["port"].as<int>();
    if (node["workers"]) cfg.workers = node["workers"].as<int>();
    if (node["enable_ssl"]) cfg.enableSSL = node["enable_ssl"].as<bool>();
    if (node["ssl_cert"]) cfg.sslCertPath = node["ssl_cert"].as<std::string>();
    if (node["ssl_key"]) cfg.sslKeyPath = node["ssl_key"].as<std::string>();
    if (node["enable_cors"]) cfg.enableCORS = node["enable_cors"].as<bool>();
    if (node["cors_origin"]) cfg.corsOrigin = node["cors_origin"].as<std::string>();
    if (node["cors_methods"]) cfg.corsMethods = node["cors_methods"].as<std::string>();
    if (node["cors_headers"]) cfg.corsHeaders = node["cors_headers"].as<std::string>();
    if (node["cors_credentials"]) cfg.corsCredentials = node["cors_credentials"].as<bool>();
    if (node["enable_compression"]) cfg.enableCompression = node["enable_compression"].as<bool>();
    if (node["compression_level"]) cfg.compressionLevel = node["compression_level"].as<int>();
    if (node["enable_rate_limit"]) cfg.enableRateLimit = node["enable_rate_limit"].as<bool>();
    if (node["rate_limit_max_requests"]) cfg.rateLimitMaxRequests = node["rate_limit_max_requests"].as<int>();
    if (node["rate_limit_window_seconds"]) cfg.rateLimitWindowSeconds = node["rate_limit_window_seconds"].as<int>();
}

void ConfigParser::parseDatabaseConfig(const YAML::Node& node, AppConfig::DatabaseConfig& cfg) {
    if (!node.IsMap()) return;
    
    if (node["type"]) cfg.type = node["type"].as<std::string>();
    if (node["host"]) cfg.host = node["host"].as<std::string>();
    if (node["port"]) cfg.port = node["port"].as<int>();
    if (node["name"]) cfg.name = node["name"].as<std::string>();
    if (node["username"]) cfg.username = node["username"].as<std::string>();
    if (node["password"]) cfg.password = node["password"].as<std::string>();
    if (node["max_connections"]) cfg.maxConnections = node["max_connections"].as<int>();
    if (node["connection_timeout"]) cfg.connectionTimeout = node["connection_timeout"].as<int>();
    if (node["reconnect_attempts"]) cfg.reconnectAttempts = node["reconnect_attempts"].as<int>();
}

void ConfigParser::parseRedisConfig(const YAML::Node& node, RedisConfig& cfg) {
    if (!node.IsMap()) return;
    
    if (node["host"]) cfg.host = node["host"].as<std::string>();
    if (node["port"]) cfg.port = node["port"].as<int>();
    if (node["password"]) cfg.password = node["password"].as<std::string>();
    if (node["database"]) cfg.database = node["database"].as<int>();
    if (node["timeout"]) cfg.timeoutSeconds = node["timeout"].as<int>();
    if (node["max_connections"]) cfg.maxConnections = node["max_connections"].as<int>();
}

void ConfigParser::parseJWTConfig(const YAML::Node& node, AppConfig::JWTConfig& cfg) {
    if (!node.IsMap()) return;
    
    if (node["secret"]) cfg.secret = node["secret"].as<std::string>();
    if (node["expiration_hours"]) cfg.expirationHours = node["expiration_hours"].as<int>();
    if (node["issuer"]) cfg.issuer = node["issuer"].as<std::string>();
    if (node["audience"]) cfg.audience = node["audience"].as<std::string>();
}

void ConfigParser::parseAIConfig(const YAML::Node& node, AppConfig::AIConfig& cfg) {
    if (!node.IsMap()) return;
    
    if (node["default_provider"]) cfg.defaultProvider = node["default_provider"].as<std::string>();
    
    if (node["openai"]) {
        auto& oaiNode = node["openai"];
        if (oaiNode["api_key"]) cfg.openai.apiKey = oaiNode["api_key"].as<std::string>();
        if (oaiNode["base_url"]) cfg.openai.baseUrl = oaiNode["base_url"].as<std::string>();
        if (oaiNode["model"]) cfg.openai.model = oaiNode["model"].as<std::string>();
        if (oaiNode["temperature"]) cfg.openai.temperature = oaiNode["temperature"].as<double>();
        if (oaiNode["max_tokens"]) cfg.openai.maxTokens = oaiNode["max_tokens"].as<int>();
    }
    
    if (node["ollama"]) {
        auto& ollamaNode = node["ollama"];
        if (ollamaNode["base_url"]) cfg.ollama.baseUrl = ollamaNode["base_url"].as<std::string>();
        if (ollamaNode["model"]) cfg.ollama.model = ollamaNode["model"].as<std::string>();
        if (ollamaNode["temperature"]) cfg.ollama.temperature = ollamaNode["temperature"].as<double>();
        if (ollamaNode["max_tokens"]) cfg.ollama.maxTokens = ollamaNode["max_tokens"].as<int>();
    }
    
    if (node["tts"]) {
        auto& ttsNode = node["tts"];
        if (ttsNode["provider"]) cfg.tts.provider = ttsNode["provider"].as<std::string>();
        if (ttsNode["voice"]) cfg.tts.voice = ttsNode["voice"].as<std::string>();
        if (ttsNode["speed"]) cfg.tts.speed = ttsNode["speed"].as<double>();
    }
}

void ConfigParser::parseEmailConfig(const YAML::Node& node, AppConfig::EmailConfig& cfg) {
    if (!node.IsMap()) return;
    
    if (node["smtp_host"]) cfg.smtpHost = node["smtp_host"].as<std::string>();
    if (node["smtp_port"]) cfg.smtpPort = node["smtp_port"].as<int>();
    if (node["username"]) cfg.username = node["username"].as<std::string>();
    if (node["password"]) cfg.password = node["password"].as<std::string>();
    if (node["from_address"]) cfg.fromAddress = node["from_address"].as<std::string>();
    if (node["use_tls"]) cfg.useTLS = node["use_tls"].as<bool>();
}

YAML::Node ConfigParser::configToYAMLNode(const AppConfig& cfg) const {
    YAML::Node node;
    
    node["server"] = serverConfigToYAMLNode(cfg.server);
    node["database"] = databaseConfigToYAMLNode(cfg.database);
    node["redis"] = redisConfigToYAMLNode(cfg.redis);
    
    // JWT
    node["jwt"]["secret"] = cfg.jwt.secret;
    node["jwt"]["expiration_hours"] = cfg.jwt.expirationHours;
    node["jwt"]["issuer"] = cfg.jwt.issuer;
    node["jwt"]["audience"] = cfg.jwt.audience;
    
    // Logging
    node["logging"]["level"] = cfg.logging.level;
    node["logging"]["file"] = cfg.logging.filePath;
    node["logging"]["max_size_mb"] = cfg.logging.maxSizeMB;
    node["logging"]["max_files"] = cfg.logging.maxFiles;
    node["logging"]["console_output"] = cfg.logging.consoleOutput;
    
    return node;
}

YAML::Node ConfigParser::serverConfigToYAMLNode(const HttpServerConfig& cfg) const {
    YAML::Node node;
    node["host"] = cfg.host;
    node["port"] = cfg.port;
    node["workers"] = cfg.workers;
    node["enable_ssl"] = cfg.enableSSL;
    node["enable_cors"] = cfg.enableCORS;
    node["cors_origin"] = cfg.corsOrigin;
    return node;
}

YAML::Node ConfigParser::databaseConfigToYAMLNode(const AppConfig::DatabaseConfig& cfg) const {
    YAML::Node node;
    node["type"] = cfg.type;
    node["host"] = cfg.host;
    node["port"] = cfg.port;
    node["name"] = cfg.name;
    node["username"] = cfg.username;
    node["password"] = cfg.password;
    node["max_connections"] = cfg.maxConnections;
    node["connection_timeout"] = cfg.connectionTimeout;
    return node;
}

YAML::Node ConfigParser::redisConfigToYAMLNode(const RedisConfig& cfg) const {
    YAML::Node node;
    node["host"] = cfg.host;
    node["port"] = cfg.port;
    node["password"] = cfg.password;
    node["database"] = cfg.database;
    node["timeout"] = cfg.timeoutSeconds;
    node["max_connections"] = cfg.maxConnections;
    return node;
}

std::string ConfigParser::getEnv(const std::string& key) const {
    const char* val = std::getenv(key.c_str());
    return val ? std::string(val) : "";
}

// ==================== getValue / setValue 模板特化 ====================

template<>
std::string ConfigParser::getValue<std::string>(const std::string& key, const std::string& defaultValue) const {
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }
    
    YAML::Node current = yamlRoot;
    for (const auto& p : parts) {
        if (!current || !current.IsMap() || !current[p]) {
            return defaultValue;
        }
        current = current[p];
    }
    
    try {
        return current.as<std::string>();
    } catch (...) {
        return defaultValue;
    }
}

template<>
int ConfigParser::getValue<int>(const std::string& key, const int& defaultValue) const {
    std::string strVal = getValue<std::string>(key, "");
    if (strVal.empty()) return defaultValue;
    try {
        return std::stoi(strVal);
    } catch (...) {
        return defaultValue;
    }
}

template<>
double ConfigParser::getValue<double>(const std::string& key, const double& defaultValue) const {
    std::string strVal = getValue<std::string>(key, "");
    if (strVal.empty()) return defaultValue;
    try {
        return std::stod(strVal);
    } catch (...) {
        return defaultValue;
    }
}

template<>
bool ConfigParser::getValue<bool>(const std::string& key, const bool& defaultValue) const {
    std::string strVal = getValue<std::string>(key, "");
    if (strVal.empty()) return defaultValue;
    
    std::string lower = strVal;
    std::transform(lower.begin(), lower.end(), lower.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    if (lower == "true" || lower == "yes" || lower == "1" || lower == "on") return true;
    if (lower == "false" || lower == "no" || lower == "0" || lower == "off") return false;
    return defaultValue;
}

template<>
bool ConfigParser::setValue<std::string>(const std::string& key, const std::string& value) {
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }
    
    if (parts.empty()) return false;
    
    // 在 yamlRoot 中设置值
    YAML::Node current = yamlRoot;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        current = current[parts[i]];
    }
    current[parts.back()] = value;
    
    return true;
}

// ==================== ConfigManager ====================

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() 
    : watching(false) {
    parser = std::make_shared<ConfigParser>();
}

ConfigManager::~ConfigManager() {
    stopWatchingConfigChanges();
}

bool ConfigManager::loadConfig(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex);
    
    configFilePath = filepath;
    return parser->loadFromFile(filepath);
}

std::shared_ptr<ConfigParser> ConfigManager::getParser() {
    std::lock_guard<std::mutex> lock(mutex);
    return parser;
}

AppConfig ConfigManager::getAppConfig() const {
    std::lock_guard<std::mutex> lock(mutex);
    return parser->getConfig();
}

HttpServerConfig ConfigManager::getServerConfig() const {
    std::lock_guard<std::mutex> lock(mutex);
    return parser->getConfig().server;
}

AppConfig::DatabaseConfig ConfigManager::getDatabaseConfig() const {
    std::lock_guard<std::mutex> lock(mutex);
    return parser->getConfig().database;
}

RedisConfig ConfigManager::getRedisConfig() const {
    std::lock_guard<std::mutex> lock(mutex);
    return parser->getConfig().redis;
}

AppConfig::JWTConfig ConfigManager::getJWTConfig() const {
    std::lock_guard<std::mutex> lock(mutex);
    return parser->getConfig().jwt;
}

AppConfig::AIConfig ConfigManager::getAIConfig() const {
    std::lock_guard<std::mutex> lock(mutex);
    return parser->getConfig().ai;
}

void ConfigManager::watchConfigChanges(std::function<void(const AppConfig&)> callback) {
    std::lock_guard<std::mutex> lock(mutex);
    configChangeCallback = callback;
    
    if (!watching.load()) {
        watching = true;
        watchThread = std::thread(&ConfigManager::watchThreadFunc, this);
    }
}

void ConfigManager::stopWatchingConfigChanges() {
    watching = false;
    if (watchThread.joinable()) {
        watchThread.join();
    }
}

void ConfigManager::watchThreadFunc() {
    while (watching.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (!watching.load()) break;
        
        // 检查文件是否修改
        struct stat fileStat;
        if (stat(configFilePath.c_str(), &fileStat) == 0) {
            std::lock_guard<std::mutex> lock(mutex);
            if (parser->reload() && configChangeCallback) {
                configChangeCallback(parser->getConfig());
            }
        }
    }
}

} // namespace Utils
} // namespace Yachiyo
