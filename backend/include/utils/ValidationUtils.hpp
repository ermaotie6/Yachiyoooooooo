#pragma once

#include <string>
#include <regex>

namespace yachiyo::utils {

/**
 * @brief 验证工具类
 * 
 * 提供通用数据验证方法
 */
class ValidationUtils {
public:
    /**
     * @brief 验证显示名称
     * @param displayName 显示名称
     * @return 是否有效 (1-50字符，允许中英文、数字、下划线)
     */
    static bool validateDisplayName(const std::string& displayName) {
        return !displayName.empty() && displayName.length() <= 50;
    }

    /**
     * @brief 验证邮箱地址
     * @param email 邮箱地址
     * @return 是否有效
     */
    static bool validateEmail(const std::string& email) {
        std::regex pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        return std::regex_match(email, pattern);
    }

    /**
     * @brief 验证个人简介
     * @param bio 个人简介
     * @return 是否有效 (长度不超过 500 字符)
     */
    static bool validateBio(const std::string& bio) {
        return bio.length() <= 500;
    }

    /**
     * @brief 验证密码强度
     * @param password 密码
     * @return 密码是否足够强 (至少8位，包含数字和字母)
     */
    static bool validatePasswordStrength(const std::string& password) {
        if (password.length() < 8 || password.length() > 128) return false;
        bool hasLetter = false, hasDigit = false;
        for (char c : password) {
            if (std::isalpha(c)) hasLetter = true;
            if (std::isdigit(c)) hasDigit = true;
        }
        return hasLetter && hasDigit;
    }

    /**
     * @brief 验证用户名
     * @param username 用户名
     * @return 是否有效 (3-50字符，字母数字下划线)
     */
    static bool validateUsername(const std::string& username) {
        if (username.length() < 3 || username.length() > 50) return false;
        for (char c : username) {
            if (!std::isalnum(c) && c != '_') return false;
        }
        return true;
    }
};

} // namespace yachiyo::utils

// 方便直接使用
namespace yachiyo::services {
    using ValidationUtils = yachiyo::utils::ValidationUtils;
}
