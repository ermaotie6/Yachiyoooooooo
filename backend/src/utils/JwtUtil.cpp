#include "utils/JwtUtil.hpp"
#include <iostream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <regex>

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

    // 简单的HMAC-SHA256实现（使用OpenSSL如果可用）
    // 这里使用简化版本，实际生产环境应该使用OpenSSL或类似库
    std::string hmac_sha256(const std::string& message, const std::string& secret) {
        // 简化实现：直接拼接用于演示
        // 实际应该使用 OpenSSL 的 HMAC_SHA256
        std::string combined = message + secret;
        unsigned char hash[32] = {0};
        
        // 这里应该使用真实的SHA256实现
        // 为了演示，我们使用一个简单的哈希
        for (size_t i = 0; i < combined.size(); i++) {
            hash[i % 32] ^= combined[i];
        }
        
        return base64_encode(hash, 32);
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

    // JSON简单反序列化
    std::map<std::string, std::string> json_decode(const std::string& json) {
        std::map<std::string, std::string> result;
        
        // 简单的JSON解析
        size_t pos = 0;
        while ((pos = json.find("\"", pos)) != std::string::npos) {
            // 查找键
            size_t key_start = pos + 1;
            size_t key_end = json.find("\"", key_start);
            if (key_end == std::string::npos) break;
            
            std::string key = json.substr(key_start, key_end - key_start);
            
            // 查找冒号
            size_t colon_pos = json.find(":", key_end);
            if (colon_pos == std::string::npos) break;
            
            // 查找值
            size_t value_start = colon_pos + 1;
            while (value_start < json.size() && isspace(json[value_start])) {
                value_start++;
            }
            
            std::string value;
            if (json[value_start] == '"') {
                // 字符串值
                size_t value_end = json.find("\"", value_start + 1);
                if (value_end != std::string::npos) {
                    value = json.substr(value_start + 1, value_end - value_start - 1);
                    pos = value_end + 1;
                } else {
                    break;
                }
            } else {
                // 数字、布尔值或null
                size_t value_end = value_start;
                while (value_end < json.size() && 
                       json[value_end] != ',' && json[value_end] != '}') {
                    value_end++;
                }
                value = json.substr(value_start, value_end - value_start);
                pos = value_end;
                
                // 移除末尾空格
                while (!value.empty() && isspace(value.back())) {
                    value.pop_back();
                }
            }
            
            result[key] = value;
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
