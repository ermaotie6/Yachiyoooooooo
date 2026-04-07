#include "utils/JwtUtil.hpp"
#include <iostream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <regex>
#include <optional>
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>

// 简单的Base64编码实现
namespace {
    const std::string BASE64_CHARS = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string base64_encode(const unsigned char* data, size_t len) {
        std::string ret;
        int i = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (len--) {
            char_array_3[i++] = *(data++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for(i = 0; i < 4; i++) {
                    ret += BASE64_CHARS[char_array_4[i]];
                }
                i = 0;
            }
        }

        if (i) {
            for(int j = i; j < 3; j++) {
                char_array_3[j] = '\0';
            }

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

            for (int j = 0; j <= i; j++) {
                ret += BASE64_CHARS[char_array_4[j]];
            }

            while(i++ < 3) {
                ret += '=';
            }
        }

        return ret;
    }

    std::string base64_decode(const std::string& encoded_string) {
        int in_len = encoded_string.size();
        int i = 0, j = 0, in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string ret;

        while (in_len-- && (encoded_string[in_] != '=') && 
               (isalnum(encoded_string[in_]) || encoded_string[in_] == '+' || 
                encoded_string[in_] == '/')) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++) {
                    size_t pos = BASE64_CHARS.find(char_array_4[i]);
                    if (pos != std::string::npos) {
                        char_array_4[i] = pos;
                    }
                }

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; i < 3; i++) {
                    ret += char_array_3[i];
                }
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++) {
                char_array_4[j] = 0;
            }

            for (j = 0; j < 4; j++) {
                size_t pos = BASE64_CHARS.find(char_array_4[j]);
                if (pos != std::string::npos) {
                    char_array_4[j] = pos;
                }
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

            for (j = 0; j < i - 1; j++) {
                ret += char_array_3[j];
            }
        }

        return ret;
    }

    // HMAC-SHA256 实现（使用 OpenSSL）
    std::string hmac_sha256(const std::string& message, const std::string& secret) {
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len = 0;
        
        HMAC(EVP_sha256(),
             secret.c_str(), static_cast<int>(secret.size()),
             reinterpret_cast<const unsigned char*>(message.c_str()), message.size(),
             hash, &hash_len);
        
        return base64_encode(hash, hash_len);
    }

    // JSON简单序列化
    std::string json_encode(const std::map<std::string, std::string>& data) {
        std::string result = "{";
        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first) result += ",";
            result += "\"" + key + "\":";
            
            // 简单的JSON值编码
            if (value == "true" || value == "false" || value == "null" || 
                (value.size() > 0 && isdigit(value[0]))) {
                result += value;
            } else {
                // 字符串值需要转义
                result += "\"";
                for (char c : value) {
                    if (c == '"') result += "\\\"";
                    else if (c == '\\') result += "\\\\";
                    else if (c == '\n') result += "\\n";
                    else if (c == '\r') result += "\\r";
                    else result += c;
                }
                result += "\"";
            }
            first = false;
        }
        result += "}";
        return result;
    }

    // JSON反序列化 — 使用 nlohmann::json 正确处理转义字符
    std::map<std::string, std::string> json_decode(const std::string& jsonStr) {
        std::map<std::string, std::string> result;
        
        try {
            auto j = nlohmann::json::parse(jsonStr);
            for (auto it = j.begin(); it != j.end(); ++it) {
                if (it.value().is_string()) {
                    result[it.key()] = it.value().get<std::string>();
                } else {
                    // 数字、布尔值等转为字符串
                    result[it.key()] = it.value().dump();
                }
            }
        } catch (const std::exception&) {
            // 解析失败返回空 map
        }
        
        return result;
    }
}

namespace Yachiyo {
namespace Utils {

class JwtUtil::Impl {
public:
    std::string secret;
    int expirationHours;

    Impl(const std::string& secret, int expirationHours)
        : secret(secret), expirationHours(expirationHours) {
    }

    std::string encodeHeader() {
        std::map<std::string, std::string> header;
        header["alg"] = "HS256";
        header["typ"] = "JWT";
        
        std::string json = json_encode(header);
        return base64_encode((unsigned char*)json.c_str(), json.size());
    }

    std::string encodePayload(const std::map<std::string, std::string>& claims) {
        std::map<std::string, std::string> payload = claims;
        
        // 添加标准声明
        auto now = std::chrono::system_clock::now();
        auto since_epoch = now.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
        
        payload["iat"] = std::to_string(seconds.count());
        payload["exp"] = std::to_string(seconds.count() + expirationHours * 3600);
        
        std::string json = json_encode(payload);
        return base64_encode((unsigned char*)json.c_str(), json.size());
    }

    std::string createSignature(const std::string& message) {
        // 使用HMAC-SHA256签名
        return hmac_sha256(message, secret);
    }
};

JwtUtil::JwtUtil(const std::string& secret, int expirationHours)
    : pImpl(std::make_unique<Impl>(secret, expirationHours)) {
}

JwtUtil::~JwtUtil() = default;

std::string JwtUtil::generateToken(int64_t userId, const std::string& username, 
                                   const std::string& role) {
    try {
        // 创建声明
        std::map<std::string, std::string> claims;
        claims["sub"] = std::to_string(userId);
        claims["username"] = username;
        claims["role"] = role;
        claims["jti"] = std::to_string(std::chrono::system_clock::now()
                                       .time_since_epoch()
                                       .count());
        
        // 编码头部和负载
        std::string header = pImpl->encodeHeader();
        std::string payload = pImpl->encodePayload(claims);
        
        // 创建签名
        std::string message = header + "." + payload;
        std::string signature = pImpl->createSignature(message);
        
        // 返回完整的JWT
        std::string token = message + "." + signature;
        
        std::cout << "[INFO] JWT令牌生成成功: userId=" << userId 
                  << ", username=" << username << std::endl;
        
        return token;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] JWT令牌生成失败: " << e.what() << std::endl;
        return "";
    }
}

std::pair<bool, std::string> JwtUtil::verifyToken(const std::string& token) {
    try {
        // 验证令牌格式
        int dot_count = std::count(token.begin(), token.end(), '.');
        if (dot_count != 2) {
            return {false, "无效的JWT格式"};
        }

        // 分割令牌
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos) {
            return {false, "无效的JWT格式"};
        }

        std::string header = token.substr(0, first_dot);
        std::string payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string signature = token.substr(second_dot + 1);

        // 验证签名
        std::string message = header + "." + payload;
        std::string expectedSignature = pImpl->createSignature(message);

        if (signature != expectedSignature) {
            return {false, "签名验证失败"};
        }

        // 解码负载
        std::string decodedPayload = base64_decode(payload);
        auto claims = json_decode(decodedPayload);

        // 验证过期时间
        if (claims.find("exp") != claims.end()) {
            try {
                long long exp = std::stoll(claims["exp"]);
                auto now = std::chrono::system_clock::now();
                auto since_epoch = now.time_since_epoch();
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
                
                if (seconds.count() > exp) {
                    return {false, "令牌已过期"};
                }
            } catch (...) {
                return {false, "无效的过期时间"};
            }
        }

        return {true, "令牌验证成功"};

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] JWT令牌验证异常: " << e.what() << std::endl;
        return {false, "令牌验证异常"};
    }
}

int64_t JwtUtil::getUserIdFromToken(const std::string& token) {
    try {
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos) {
            return 0;
        }

        std::string payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string decodedPayload = base64_decode(payload);
        auto claims = json_decode(decodedPayload);

        if (claims.find("sub") != claims.end()) {
            return std::stoll(claims["sub"]);
        }

        return 0;

    } catch (...) {
        return 0;
    }
}

std::string JwtUtil::getUsernameFromToken(const std::string& token) {
    try {
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos) {
            return "";
        }

        std::string payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string decodedPayload = base64_decode(payload);
        auto claims = json_decode(decodedPayload);

        if (claims.find("username") != claims.end()) {
            return claims["username"];
        }

        return "";

    } catch (...) {
        return "";
    }
}

std::string JwtUtil::getRoleFromToken(const std::string& token) {
    try {
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos) {
            return "";
        }

        std::string payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string decodedPayload = base64_decode(payload);
        auto claims = json_decode(decodedPayload);

        if (claims.find("role") != claims.end()) {
            return claims["role"];
        }

        return "";

    } catch (...) {
        return "";
    }
}

std::map<std::string, std::string> JwtUtil::getClaimsFromToken(const std::string& token) {
    try {
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos) {
            return {};
        }

        std::string payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string decodedPayload = base64_decode(payload);
        return json_decode(decodedPayload);

    } catch (...) {
        return {};
    }
}

std::string JwtUtil::generateToken(const nlohmann::json& payload, int ttlSeconds) {
    try {
        // 从 JSON payload 中提取字段
        int64_t userId = payload.value("user_id", int64_t(0));
        std::string username = payload.value("username", std::string(""));
        std::string role = payload.value("role", std::string(""));

        // 临时覆盖过期时间以使用 ttlSeconds
        int originalExpHours = pImpl->expirationHours;
        // ttlSeconds 转换为小时（向上取整）
        pImpl->expirationHours = (ttlSeconds + 3599) / 3600;

        std::string token = generateToken(userId, username, role);

        // 恢复原始过期时间
        pImpl->expirationHours = originalExpHours;

        return token;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] JWT令牌生成失败(JSON): " << e.what() << std::endl;
        return "";
    }
}

std::optional<nlohmann::json> JwtUtil::verifyTokenPayload(const std::string& token) {
    auto [valid, message] = verifyToken(token);
    if (!valid) {
        return std::nullopt;
    }

    try {
        auto claims = getClaimsFromToken(token);
        nlohmann::json payload;
        for (const auto& [key, value] : claims) {
            // 尝试解析为数字
            if (key == "sub" || key == "user_id" || key == "exp" || key == "iat") {
                try {
                    payload[key] = std::stoll(value);
                } catch (...) {
                    payload[key] = value;
                }
            } else {
                payload[key] = value;
            }
        }
        // 确保 user_id 字段存在 (从 sub 映射)
        if (payload.contains("sub") && !payload.contains("user_id")) {
            payload["user_id"] = payload["sub"];
        }
        return payload;
    } catch (...) {
        return std::nullopt;
    }
}

std::string JwtUtil::refreshToken(const std::string& token) {
    try {
        // 验证令牌
        auto [valid, message] = verifyToken(token);
        if (!valid) {
            return "";
        }

        // 从旧令牌中提取声明
        auto claims = getClaimsFromToken(token);
        
        if (claims.find("sub") == claims.end()) {
            return "";
        }

        int64_t userId = std::stoll(claims["sub"]);
        std::string username = claims["username"];
        std::string role = claims["role"];

        // 生成新令牌
        return generateToken(userId, username, role);

    } catch (...) {
        return "";
    }
}

} // namespace Utils
} // namespace Yachiyo
