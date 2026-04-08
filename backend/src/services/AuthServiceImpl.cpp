#include "services/AuthServiceImpl.hpp"
#include "utils/Logger.hpp"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <regex>

namespace yachiyo::services {

// ==================== 辅助验证方法 ====================

bool AuthServiceImpl::isValidUsername(const std::string& username) const {
    if (username.length() < 3 || username.length() > 50) return false;
    // 允许字母、数字、下划线、中文
    std::regex pattern("^[a-zA-Z0-9_\\u4e00-\\u9fff]+$");
    return std::regex_match(username, pattern);
}

bool AuthServiceImpl::isValidEmail(const std::string& email) const {
    std::regex pattern("^[^@]+@[^@]+\\.[^@]+$");
    return std::regex_match(email, pattern);
}

bool AuthServiceImpl::isValidPassword(const std::string& password) const {
    if (password.length() < 8 || password.length() > 128) return false;
    
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    for (char c : password) {
        if (std::isupper(c)) hasUpper = true;
        else if (std::islower(c)) hasLower = true;
        else if (std::isdigit(c)) hasDigit = true;
        else hasSpecial = true;
    }
    
    return hasUpper && hasLower && hasDigit && hasSpecial;
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
                "密码无效。必须8-128字符，且包含大写字母、小写字母、数字和特殊字符"
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
        auto result = dbUtil->insert(
            "INSERT INTO users (username, email, password_hash, salt, role, status, created_at, updated_at) "
            "VALUES ($1, $2, $3, $4, $5, $6, NOW(), NOW()) "
            "RETURNING id, username, email, nickname, avatar_url, bio, role, status, created_at",
            {username, email, hash, salt, std::to_string(static_cast<int>(Models::UserRole::USER)), "1"}
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
        user->setRole(Models::UserRole::USER);
        user->setStatus(Models::UserStatus::ACTIVE);
        
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
        auto result = dbUtil->query(
            "SELECT id, username, email, password_hash, salt, role, status, "
            "       is_banned, warnings_count FROM users "
            "WHERE (username = $1 OR email = $1) AND status != $2",
            {username, std::to_string(static_cast<int>(Models::UserStatus::DISABLED))}
        );
        
        if (result.empty()) {
            return Result<json>::Error("用户名或密码错误");
        }
        
        // 2. 验证密码
        auto row = result[0];
        std::string passwordHash = row["password_hash"];
        std::string salt = row["salt"];
        
        // 数据库分开存储 hash 和 salt，直接用分离格式验证
        if (!Yachiyo::Utils::HashUtil::verifyPassword(password, passwordHash, salt)) {
            return Result<json>::Error("用户名或密码错误");
        }
        
        // 3. 检查账户状态
        int status = std::stoi(row["status"]);
        if (status == static_cast<int>(Models::UserStatus::BANNED)) {
            return Result<json>::Error("账户已被封禁");
        }
        
        int64_t userId = std::stoll(row["id"]);
        
        // 4. 生成令牌
        std::string username_val = row["username"];
        std::string role_val = row["role"];
        
        // access_token: 使用默认过期时间 (由 JwtUtil 构造时设定, 通常 24 小时)
        auto accessToken = jwtUtil->generateToken(userId, username_val, role_val);
        // refresh_token: 使用 7 天有效期 (604800 秒)
        nlohmann::json refreshPayload;
        refreshPayload["user_id"] = userId;
        refreshPayload["username"] = username_val;
        refreshPayload["role"] = role_val;
        refreshPayload["token_type"] = "refresh";
        auto refreshToken = jwtUtil->generateToken(refreshPayload, 604800);
        
        // 5. 更新最后登录时间
        dbUtil->execute(
            "UPDATE users SET last_login_at = NOW(), last_login_ip = $1 WHERE id = $2",
            {userIp, std::to_string(userId)}
        );
        
        // 6. 构建响应
        json response;
        response["access_token"] = accessToken;
        response["refresh_token"] = refreshToken;
        response["user"] = {
            {"id", userId},
            {"username", row["username"]},
            {"email", row["email"]},
            {"role", row["role"]}
        };
        
        LOG_INFO("用户登录成功: " + std::string(row["username"]));
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
        return jwtUtil->getUserIdFromToken(token);
    } catch (...) {
        return 0;
    }
}

// ==================== 从令牌提取角色 ====================

UserRole AuthServiceImpl::getRoleFromToken(const std::string& token) {
    try {
        std::string roleStr = jwtUtil->getRoleFromToken(token);
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
        int64_t userId = jwtUtil->getUserIdFromToken(refreshToken);
        std::string username = jwtUtil->getUsernameFromToken(refreshToken);
        std::string role = jwtUtil->getRoleFromToken(refreshToken);
        
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
                dbUtil->redisSet(blacklistKey, "1", 604800);  // 7天过期
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
        user->setUsername(row["username"]);
        user->setEmail(row["email"]);
        user->setNickname(row["nickname"]);
        user->setAvatar(row["avatar_url"]);
        user->setBio(row["bio"]);
        user->setRole(static_cast<UserRole>(std::stoi(row["role"])));
        user->setStatus(static_cast<UserStatus>(std::stoi(row["status"])));
        user->setMessagesSent(std::stoll(row["messages_sent"]));
        user->setMessagesApproved(std::stoll(row["messages_approved"]));
        user->setMessagesRejected(std::stoll(row["messages_rejected"]));
        user->setWarningsCount(std::stoi(row["warnings_count"]));
        
        if (row["is_banned"] == "true") {
            time_t banUntil = 0;
            if (!row["ban_until"].empty()) {
                // 解析 PostgreSQL TIMESTAMP 格式 ("YYYY-MM-DD HH:MM:SS")
                struct tm tm = {};
                std::istringstream ss(row["ban_until"]);
                ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                if (!ss.fail()) {
                    banUntil = mktime(&tm);
                } else {
                    // 兼容: 如果是纯数字时间戳则回退到 stoll
                    try {
                        banUntil = std::stoll(row["ban_until"]);
                    } catch (...) {
                        LOG_WARN("无法解析 ban_until: {}", row["ban_until"]);
                    }
                }
            }
            user->setBanned(true, row["ban_reason"], banUntil);
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
        
        int64_t userId = std::stoll(result[0]["id"]);
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
