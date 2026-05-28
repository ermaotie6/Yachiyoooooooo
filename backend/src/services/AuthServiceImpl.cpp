#include "services/AuthServiceImpl.hpp"
#include "utils/Logger.hpp"
#include "utils/RedisUtil.hpp"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <regex>

namespace yachiyo::services {

// ==================== 辅助验证方法 ====================

bool AuthServiceImpl::isValidUsername(const std::string& username) const {
    // 长度检查：使用 UTF-8 字符计数而非字节长度
    size_t charCount = 0;
    for (size_t i = 0; i < username.size(); ) {
        unsigned char c = static_cast<unsigned char>(username[i]);
        if (c < 0x80) {
            // ASCII: 只允许字母、数字、下划线
            if (!std::isalnum(c) && c != '_') return false;
            i += 1;
        } else if ((c >> 5) == 0x06) {
            // 2字节 UTF-8
            if (i + 1 >= username.size()) return false;
            i += 2;
        } else if ((c >> 4) == 0x0E) {
            // 3字节 UTF-8 — 包含 CJK 统一汉字 (U+4E00-U+9FFF)
            if (i + 2 >= username.size()) return false;
            // 验证是否为合法的 CJK 汉字范围
            uint32_t codepoint = (static_cast<uint32_t>(c & 0x0F) << 12)
                               | (static_cast<uint32_t>(username[i+1] & 0x3F) << 6)
                               | static_cast<uint32_t>(username[i+2] & 0x3F);
            if (codepoint < 0x4E00 || codepoint > 0x9FFF) {
                // 不在 CJK 汉字范围内，拒绝
                return false;
            }
            i += 3;
        } else if ((c >> 3) == 0x1E) {
            // 4字节 UTF-8 (emoji 等) — 不允许在用户名中使用
            return false;
        } else {
            // 无效 UTF-8 字节
            return false;
        }
        ++charCount;
    }
    return charCount >= 3 && charCount <= 50;
}

bool AuthServiceImpl::isValidEmail(const std::string& email) const {
    std::regex pattern("^[^@]+@[^@]+\\.[^@]+$");
    return std::regex_match(email, pattern);
}

bool AuthServiceImpl::isValidPassword(const std::string& password) const {
    // 只限制长度，不限制字符类型
    return password.length() >= 1 && password.length() <= 128;
}

bool AuthServiceImpl::isUserBlacklisted(const std::string& identifier) const {
    try {
        auto result = dbUtil->query(
            "SELECT 1 FROM user_blacklist "
            "WHERE identifier = $1 "
            "AND (expires_at IS NULL OR expires_at > NOW())",
            {identifier}
        );
        return !result.empty();
    } catch (const std::exception& e) {
        LOG_ERROR("检查黑名单失败: " + std::string(e.what()));
        return false;
    }
}

// ==================== 注册用户 ====================

Result<std::shared_ptr<User>> AuthServiceImpl::registerUser(
    const std::string& username,
    const std::string& email,
    const std::string& password
) {
    try {
        // 1. 验证输入
        if (!isValidUsername(username)) {
            return Result<std::shared_ptr<User>>::Error(
                "用户名无效。必须3-50字符，只含字母、数字、下划线或中文"
            );
        }
        
        if (!isValidEmail(email)) {
            return Result<std::shared_ptr<User>>::Error("邮箱格式无效");
        }
        
        if (!isValidPassword(password)) {
            return Result<std::shared_ptr<User>>::Error(
                "密码无效。必须1-128字符"
            );
        }
        
        // 2. 检查黑名单
        if (isUserBlacklisted(email)) {
            return Result<std::shared_ptr<User>>::Error(
                "该邮箱已被禁止注册"
            );
        }
        
        // 3. 分别检查用户名和邮箱是否已存在，给出精确提示
        auto existingUsername = dbUtil->query(
            "SELECT 1 FROM users WHERE username = $1",
            {username}
        );
        if (!existingUsername.empty()) {
            return Result<std::shared_ptr<User>>::Error(
                "该用户名已被注册"
            );
        }
        
        auto existingEmail = dbUtil->query(
            "SELECT 1 FROM users WHERE email = $1",
            {email}
        );
        if (!existingEmail.empty()) {
            return Result<std::shared_ptr<User>>::Error(
                "该邮箱已被注册"
            );
        }
        
        // 4. 密码哈希
        auto [hash, salt] = hashUtil->hashPassword(password);
        
        // 5. 创建用户
        std::vector<std::string> insertParams = {
            username, email, hash, salt,
            std::to_string(static_cast<int>(UserRole::USER)), "1"
        };
        auto result = dbUtil->insert(
            "INSERT INTO users (username, email, password_hash, salt, role, status, created_at, updated_at) "
            "VALUES ($1, $2, $3, $4, $5, $6, NOW(), NOW()) "
            "RETURNING id, username, email, nickname, avatar_url, bio, role, status, created_at",
            insertParams
        );
        
        if (result.empty()) {
            return Result<std::shared_ptr<User>>::Error("创建用户失败");
        }
        
        // 6. 构建用户对象
        auto user = std::make_shared<User>();
        // 从 RETURNING 结果中提取用户 ID
        if (result[0].count("id")) {
            user->setId(result[0].at("id"));  // id 为 string 类型 (BaseModel::id)
        }
        user->setUsername(username);
        user->setEmail(email);
        user->setNickname(username);  // 默认昵称与用户名相同
        user->setRole(UserRole::USER);
        user->setStatus(UserStatus::ACTIVE);
        
        LOG_INFO("用户注册成功: " + username);
        return Result<std::shared_ptr<User>>::Success(user);
        
    } catch (const std::exception& e) {
        LOG_ERROR("用户注册异常: " + std::string(e.what()));
        return Result<std::shared_ptr<User>>::Error("注册失败，请稍后重试");
    }
}

// ==================== 用户登录 ====================

Result<json> AuthServiceImpl::login(
    const std::string& username,
    const std::string& password,
    const std::string& userIp
) {
    try {
        // 1. 查找用户
        std::vector<std::string> queryParams = {
            username, std::to_string(static_cast<int>(UserStatus::DISABLED))
        };
        auto result = dbUtil->query(
            "SELECT id, username, email, password_hash, salt, role, status, "
            "       is_banned, warnings_count FROM users "
            "WHERE (username = $1 OR email = $1) AND status != $2",
            queryParams
        );
        
        if (result.empty()) {
            return Result<json>::Error("用户名或密码错误");
        }
        
        // 2. 验证密码
        auto row = result[0];
        std::string passwordHash = row.at("password_hash");
        std::string salt = row.at("salt");
        
        // 数据库分开存储 hash 和 salt，直接用分离格式验证
        if (!Yachiyo::Utils::HashUtil::verifyPassword(password, passwordHash, salt)) {
            return Result<json>::Error("用户名或密码错误");
        }
        
        // 3. 检查账户状态
        int status = std::stoi(row.at("status"));
        if (status == static_cast<int>(UserStatus::BANNED)) {
            return Result<json>::Error("账户已被封禁");
        }
        
        int64_t userId = std::stoll(row.at("id"));
        
        // 4. 生成令牌
        std::string username_val = row.at("username");
        std::string role_val = row.at("role");
        // 将 SMALLINT 角色值转换为可读字符串，供 JWT 和前端使用
        std::string role_readable = (role_val == "99") ? "admin" : "user";
        
        // access_token: 使用默认过期时间 (由 JwtUtil 构造时设定, 通常 24 小时)
        auto accessToken = jwtUtil->generateToken(userId, username_val, role_readable);
        // refresh_token: 使用 7 天有效期 (604800 秒)
        nlohmann::json refreshPayload;
        refreshPayload["user_id"] = userId;
        refreshPayload["username"] = username_val;
        refreshPayload["role"] = role_readable;
        refreshPayload["token_type"] = "refresh";
        auto refreshToken = jwtUtil->generateToken(refreshPayload, 604800);
        
        // 5. 更新最后登录时间
        std::vector<std::string> updateParams = {userIp, std::to_string(userId)};
        dbUtil->execute(
            "UPDATE users SET last_login_at = NOW(), last_login_ip = $1 WHERE id = $2",
            updateParams
        );
        
        // 6. 构建响应
        json response;
        response["access_token"] = accessToken;
        response["refresh_token"] = refreshToken;
        json userJson;
        userJson["id"] = userId;
        userJson["username"] = row.at("username");
        userJson["email"] = row.at("email");
        userJson["role"] = role_readable;
        response["user"] = userJson;
        
        LOG_INFO("用户登录成功: " + row.at("username"));
        return Result<json>::Success(response);
        
    } catch (const std::exception& e) {
        LOG_ERROR("登录异常: " + std::string(e.what()));
        return Result<json>::Error("登录失败，请稍后重试");
    }
}

// ==================== 验证令牌 ====================

Result<std::shared_ptr<User>> AuthServiceImpl::verifyToken(
    const std::string& token
) {
    try {
        auto payload = jwtUtil->verifyTokenPayload(token);
        if (!payload) {
            return Result<std::shared_ptr<User>>::Error("令牌无效或已过期");
        }
        
        int64_t userId = payload->value("user_id", payload->value("sub", int64_t(0)));
        return getUserById(userId);
        
    } catch (const std::exception& e) {
        LOG_ERROR("令牌验证异常: " + std::string(e.what()));
        return Result<std::shared_ptr<User>>::Error("令牌验证失败");
    }
}

// ==================== 从令牌提取用户ID ====================

int64_t AuthServiceImpl::getUserIdFromToken(const std::string& token) {
    try {
        auto payload = jwtUtil->verifyTokenPayload(token);
        if (!payload) {
            return 0;
        }
        return payload->value("user_id", payload->value("sub", int64_t(0)));
    } catch (...) {
        return 0;
    }
}

// ==================== 从令牌提取角色 ====================

UserRole AuthServiceImpl::getRoleFromToken(const std::string& token) {
    try {
        auto payload = jwtUtil->verifyTokenPayload(token);
        if (!payload) {
            return UserRole::USER;
        }
        std::string roleStr = payload->value("role", std::string("user"));
        if (roleStr == "ADMIN" || roleStr == "admin" || roleStr == "99") {
            return UserRole::ADMIN;
        }
        return UserRole::USER;
    } catch (...) {
        return UserRole::USER;
    }
}

// ==================== 刷新令牌 ====================

Result<json> AuthServiceImpl::refreshToken(
    const std::string& refreshToken
) {
    try {
        auto [valid, message] = jwtUtil->verifyToken(refreshToken);
        if (!valid) {
            return Result<json>::Error("刷新令牌无效或已过期: " + message);
        }
        
        // 从旧令牌中提取信息生成新令牌
        try {
            std::string blacklistKey = "token_blacklist:" + refreshToken;
            if (Yachiyo::Utils::RedisUtil::cacheExists(blacklistKey)) {
                return Result<json>::Error("refresh token revoked");
            }
        } catch (...) {
            LOG_WARN("Unable to check refresh token blacklist");
        }

        auto payload = jwtUtil->verifyTokenPayload(refreshToken);
        if (!payload) {
            return Result<json>::Error("invalid refresh token payload");
        }
        if (payload->value("token_type", std::string("")) != "refresh") {
            return Result<json>::Error("invalid token type for refresh");
        }

        int64_t userId = payload->value("user_id", payload->value("sub", int64_t(0)));
        std::string username = payload->value("username", std::string(""));
        std::string role = payload->value("role", std::string("user"));
        
        auto newAccessToken = jwtUtil->generateToken(userId, username, role);
        
        json response;
        response["access_token"] = newAccessToken;
        response["refresh_token"] = refreshToken;
        
        return Result<json>::Success(response);
        
    } catch (const std::exception& e) {
        LOG_ERROR("刷新令牌异常: " + std::string(e.what()));
        return Result<json>::Error("刷新令牌失败");
    }
}

// ==================== 用户注销 ====================

Result<bool> AuthServiceImpl::logout(
    int64_t userId,
    const std::string& refreshToken
) {
    try {
        // 将 refresh token 加入 Redis 黑名单 (TTL 7天)
        if (!refreshToken.empty()) {
            try {
                std::string blacklistKey = "token_blacklist:" + refreshToken;
                Yachiyo::Utils::RedisUtil::setCache(blacklistKey, "1", 604800);  // 7天过期
            } catch (const std::exception& e) {
                // Redis 不可用时记录警告但不阻止注销
                // TODO: 考虑将 Redis 操作抽离到独立的 RedisCacheService
                LOG_WARN("注销时无法将 token 加入黑名单 (Redis 不可用): {}", e.what());
            }
        }
        
        LOG_INFO("用户注销: " + std::to_string(userId));
        return Result<bool>::Success(true);
        
    } catch (const std::exception& e) {
        LOG_ERROR("注销异常: " + std::string(e.what()));
        return Result<bool>::Error("注销失败");
    }
}

// ==================== 根据ID获取用户 ====================

Result<std::shared_ptr<User>> AuthServiceImpl::getUserById(int64_t userId) {
    try {
        auto result = dbUtil->query(
            "SELECT id, username, email, nickname, avatar_url, bio, role, status, "
            "       messages_sent, messages_approved, messages_rejected, warnings_count, "
            "       is_banned, ban_reason, ban_until FROM users WHERE id = $1",
            {std::to_string(userId)}
        );
        
        if (result.empty()) {
            return Result<std::shared_ptr<User>>::Error("用户不存在");
        }
        
        auto row = result[0];
        auto user = std::make_shared<User>();
        
        user->setId(std::to_string(userId));
        user->setUsername(row.at("username"));
        user->setEmail(row.at("email"));
        user->setNickname(row.at("nickname"));
        user->setAvatar(row.at("avatar_url"));
        user->setBio(row.at("bio"));
        user->setRole(static_cast<UserRole>(std::stoi(row.at("role"))));
        user->setStatus(static_cast<UserStatus>(std::stoi(row.at("status"))));
        user->setMessagesSent(std::stoll(row.at("messages_sent")));
        user->setMessagesApproved(std::stoll(row.at("messages_approved")));
        user->setMessagesRejected(std::stoll(row.at("messages_rejected")));
        user->setWarningsCount(std::stoi(row.at("warnings_count")));

        if (row.at("is_banned") == "true") {
            time_t banUntil = 0;
            if (!row.at("ban_until").empty()) {
                // 解析 PostgreSQL TIMESTAMP 格式 ("YYYY-MM-DD HH:MM:SS")
                struct tm tm = {};
                std::istringstream ss(row.at("ban_until"));
                ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                if (!ss.fail()) {
                    banUntil = mktime(&tm);
                } else {
                    // 兼容: 如果是纯数字时间戳则回退到 stoll
                    try {
                        banUntil = std::stoll(row.at("ban_until"));
                    } catch (...) {
                        LOG_WARN("无法解析 ban_until: {}", row.at("ban_until"));
                    }
                }
            }
            user->setBanned(true, row.at("ban_reason"), banUntil);
        }
        
        return Result<std::shared_ptr<User>>::Success(user);
        
    } catch (const std::exception& e) {
        LOG_ERROR("获取用户异常: " + std::string(e.what()));
        return Result<std::shared_ptr<User>>::Error("获取用户失败");
    }
}

// ==================== 根据用户名获取用户 ====================

Result<std::shared_ptr<User>> AuthServiceImpl::getUserByUsername(
    const std::string& username
) {
    try {
        auto result = dbUtil->query(
            "SELECT id FROM users WHERE username = $1",
            {username}
        );
        
        if (result.empty()) {
            return Result<std::shared_ptr<User>>::Error("用户不存在");
        }
        
        int64_t userId = std::stoll(result[0].at("id"));
        return getUserById(userId);
        
    } catch (const std::exception& e) {
        LOG_ERROR("获取用户异常: " + std::string(e.what()));
        return Result<std::shared_ptr<User>>::Error("获取用户失败");
    }
}

// ==================== 更新用户资料 ====================

Result<bool> AuthServiceImpl::updateProfile(
    int64_t userId,
    const std::string& nickname,
    const std::string& bio
) {
    try {
        std::vector<std::string> setClauses;
        std::vector<std::string> params;
        int paramIdx = 1;
        
        if (!nickname.empty()) {
            setClauses.push_back("nickname = $" + std::to_string(paramIdx++));
            params.push_back(nickname);
        }
        if (!bio.empty()) {
            setClauses.push_back("bio = $" + std::to_string(paramIdx++));
            params.push_back(bio);
        }
        
        if (setClauses.empty()) {
            return Result<bool>::Error("至少需要提供一个更新字段");
        }
        
        setClauses.push_back("updated_at = NOW()");
        
        std::string sql = "UPDATE users SET ";
        for (size_t i = 0; i < setClauses.size(); ++i) {
            if (i > 0) sql += ", ";
            sql += setClauses[i];
        }
        sql += " WHERE id = $" + std::to_string(paramIdx);
        params.push_back(std::to_string(userId));
        
        int affected = dbUtil->execute(sql, params);
        if (affected <= 0) {
            return Result<bool>::Error("更新失败，用户可能不存在");
        }
        
        LOG_INFO("用户资料更新成功: userId=" + std::to_string(userId));
        return Result<bool>::Success(true);
        
    } catch (const std::exception& e) {
        LOG_ERROR("更新用户资料异常: " + std::string(e.what()));
        return Result<bool>::Error("更新用户资料失败");
    }
}

// ==================== 检查用户是否可以发送消息 ====================

Result<bool> AuthServiceImpl::canUserSendMessage(int64_t userId) {
    try {
        auto userResult = getUserById(userId);
        if (!userResult.isSuccess()) {
            return Result<bool>::Error("用户不存在");
        }
        
        auto user = userResult.value();
        
        if (!user->canSendMessages()) {
            if (user->getIsBanned()) {
                return Result<bool>::Error("账户已被封禁：" + user->getBanReason());
            } else {
                return Result<bool>::Error("账户已禁用");
            }
        }
        
        return Result<bool>::Success(true);
        
    } catch (const std::exception& e) {
        LOG_ERROR("检查用户发送权限异常: " + std::string(e.what()));
        return Result<bool>::Error("检查权限失败");
    }
}

} // namespace yachiyo::services
