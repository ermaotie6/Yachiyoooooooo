#include "services/UserService.hpp"
#include "services/DatabaseService.hpp"
#include "utils/LogUtils.hpp"
#include "utils/JsonUtils.hpp"
#include "utils/ValidationUtils.hpp"
#include "utils/HashUtil.hpp"
#include <crow.h>
#include <chrono>
#include <algorithm>
#include <regex>

// 全局数据库服务引用 — 定义在 Application.cpp (全局命名空间)
extern std::shared_ptr<Yachiyo::Services::DatabaseService> g_databaseService;

namespace Yachiyo::services {

using yachiyo::utils::LogUtils;
using yachiyo::utils::ValidationUtils;
using Yachiyo::Utils::HashUtil;
namespace dto = Yachiyo::dto;

UserServiceImpl::UserServiceImpl() {
    logger = LogUtils::getLogger("UserServiceImpl");
    logger->info("用户服务初始化完成 (已接入 PostgreSQL)");
}

UserServiceImpl::~UserServiceImpl() {
    logger->info("用户服务销毁");
}

// ==================== 辅助: DB User → DTO ====================

static dto::UserDTO buildUserDTO(const Yachiyo::Models::User& dbUser) {
    dto::UserDTO user;
    user.id = std::to_string(dbUser.id);
    user.username = dbUser.username;
    user.email = dbUser.email;

    // 从 profile_data JSON 提取字段
    if (dbUser.profile_data.contains("displayName")) {
        user.displayName = dbUser.profile_data["displayName"].get<std::string>();
    } else {
        user.displayName = dbUser.username;
    }
    if (dbUser.profile_data.contains("avatarUrl")) {
        user.avatarUrl = dbUser.profile_data["avatarUrl"].get<std::string>();
    }
    if (dbUser.profile_data.contains("bio")) {
        user.bio = dbUser.profile_data["bio"].get<std::string>();
    }
    // 从数据库 role SMALLINT 列读取角色 (1=user, 99=admin)
    if (dbUser.role == 99) {
        user.role = "admin";
    } else {
        user.role = "user";
    }

    user.isActive = dbUser.is_active;
    user.isVerified = dbUser.profile_data.value("isVerified", false);
    user.followersCount = dbUser.profile_data.value("followersCount", 0);
    user.followingCount = dbUser.profile_data.value("followingCount", 0);
    user.postsCount = dbUser.profile_data.value("postsCount", 0);

    user.statistics.totalLikes = dbUser.profile_data.value("totalLikes", 0);
    user.statistics.totalComments = dbUser.profile_data.value("totalComments", 0);
    user.statistics.totalViews = dbUser.profile_data.value("totalViews", 0);

    return user;
}

// ==================== 获取用户 ====================

dto::UserDTO UserServiceImpl::getUserById(const std::string& userId) {
    try {
        logger->debug("获取用户信息: userId={}", userId);

        if (!g_databaseService || !g_databaseService->isInitialized()) {
            throw std::runtime_error("数据库服务未初始化");
        }

        int64_t uid = std::stoll(userId);
        auto result = g_databaseService->userDAO().getById(uid);

        if (!result.success) {
            throw std::runtime_error("用户不存在: " + userId);
        }

        return buildUserDTO(result.data.value());

    } catch (const std::exception& e) {
        logger->error("获取用户信息失败: {}", e.what());
        throw;
    }
}

dto::UserDTO UserServiceImpl::getUserByUsername(const std::string& username) {
    try {
        logger->debug("通过用户名获取用户: username={}", username);

        if (!g_databaseService || !g_databaseService->isInitialized()) {
            throw std::runtime_error("数据库服务未初始化");
        }

        auto result = g_databaseService->userDAO().getByUsername(username);
        if (!result.success) {
            throw std::runtime_error("用户不存在: " + username);
        }

        return buildUserDTO(result.data.value());

    } catch (const std::exception& e) {
        logger->error("通过用户名获取用户失败: {}", e.what());
        throw;
    }
}

// ==================== 更新用户 ====================

dto::UserDTO UserServiceImpl::updateUser(const std::string& userId, const dto::UserDTO& userUpdate) {
    try {
        logger->debug("更新用户信息: userId={}", userId);

        if (!g_databaseService || !g_databaseService->isInitialized()) {
            throw std::runtime_error("数据库服务未初始化");
        }

        if (!userUpdate.displayName.empty() &&
            !ValidationUtils::validateDisplayName(userUpdate.displayName)) {
            throw std::runtime_error("显示名称无效");
        }
        if (!userUpdate.email.empty() &&
            !ValidationUtils::validateEmail(userUpdate.email)) {
            throw std::runtime_error("邮箱地址无效");
        }
        if (!userUpdate.bio.empty() &&
            !ValidationUtils::validateBio(userUpdate.bio)) {
            throw std::runtime_error("个人简介过长");
        }

        int64_t uid = std::stoll(userId);
        auto existingResult = g_databaseService->userDAO().getById(uid);
        if (!existingResult.success) {
            throw std::runtime_error("用户不存在");
        }

        auto existingUser = existingResult.data.value();
        nlohmann::json profileData = existingUser.profile_data;
        if (!userUpdate.displayName.empty()) profileData["displayName"] = userUpdate.displayName;
        if (!userUpdate.bio.empty())         profileData["bio"] = userUpdate.bio;
        if (!userUpdate.avatarUrl.empty())   profileData["avatarUrl"] = userUpdate.avatarUrl;

        g_databaseService->userDAO().updateProfile(uid, profileData);

        // 重新查询返回最新数据
        auto freshResult = g_databaseService->userDAO().getById(uid);
        logger->info("用户信息更新成功: userId={}", userId);
        return buildUserDTO(freshResult.data.value());

    } catch (const std::exception& e) {
        logger->error("更新用户信息失败: {}", e.what());
        throw;
    }
}

// ==================== 密码 ====================

bool UserServiceImpl::updatePassword(const std::string& userId,
                                    const std::string& oldPassword,
                                    const std::string& newPassword) {
    try {
        if (!validatePassword(userId, oldPassword)) {
            throw std::runtime_error("旧密码不正确");
        }
        if (!ValidationUtils::validatePasswordStrength(newPassword)) {
            throw std::runtime_error("新密码强度不足");
        }

        if (!g_databaseService || !g_databaseService->isInitialized()) {
            throw std::runtime_error("数据库服务未初始化");
        }

        // 生成新的密码哈希（分离存储 hash 和 salt，与 AuthServiceImpl 保持一致）
        auto [newHash, newSalt] = HashUtil::hashPassword(newPassword);
        
        int64_t uid = std::stoll(userId);
        auto result = g_databaseService->userDAO().getById(uid);
        if (!result.success) {
            throw std::runtime_error("用户不存在");
        }

        // 同时更新 password_hash 和 salt 两个列
        auto conn = Yachiyo::Services::DatabasePool::getInstance().getConnection();
        if (conn && conn->is_open()) {
            try {
                pqxx::work txn(*conn);
                txn.exec_params(
                    "UPDATE users SET password_hash = $1, salt = $2, updated_at = NOW() WHERE id = $3",
                    newHash, newSalt, uid
                );
                txn.commit();
                Yachiyo::Services::DatabasePool::getInstance().releaseConnection(conn);
            } catch (...) {
                Yachiyo::Services::DatabasePool::getInstance().releaseConnection(conn);
                throw;
            }
        }

        logger->info("密码更新成功: userId={}", userId);
        return true;
    } catch (const std::exception& e) {
        logger->error("更新密码失败: {}", e.what());
        return false;
    }
}

bool UserServiceImpl::validatePassword(const std::string& userId, const std::string& password) {
    try {
        if (!g_databaseService || !g_databaseService->isInitialized()) return false;

        int64_t uid = std::stoll(userId);
        auto result = g_databaseService->userDAO().getById(uid);
        if (!result.success) return false;

        // 使用分离格式验证：User 模型现在包含 salt 字段
        const auto& user = result.data.value();
        if (!user.salt.empty()) {
            // 分离存储格式：使用 3 参数 verifyPassword
            return HashUtil::verifyPassword(password, user.password_hash, user.salt);
        }
        // 兼容旧的合并格式 (salt:hash)
        return HashUtil::verifyPassword(password, user.password_hash);
    } catch (const std::exception& e) {
        logger->error("验证密码失败: {}", e.what());
        return false;
    }
}

// ==================== 搜索 ====================

std::vector<dto::UserDTO> UserServiceImpl::searchUsers(const std::string& keyword,
                                                      int page, int pageSize) {
    try {
        logger->debug("搜索用户: keyword={}", keyword);

        std::vector<dto::UserDTO> users;
        if (!g_databaseService || !g_databaseService->isInitialized()) {
            return users;
        }

        page = std::max(1, page);
        pageSize = std::clamp(pageSize, 1, 100);
        int offset = (page - 1) * pageSize;

        auto conn = Yachiyo::Services::DatabasePool::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res;
        if (keyword.empty()) {
            res = txn.exec_params(
                "SELECT id, username, email, password_hash, salt, nickname, avatar_url, role, status, "
                "profile_data, preferences, created_at, updated_at, last_login_at, is_active "
                "FROM users ORDER BY id DESC LIMIT $1 OFFSET $2",
                pageSize, offset
            );
        } else {
            std::string like = "%" + keyword + "%";
            res = txn.exec_params(
                "SELECT id, username, email, password_hash, salt, nickname, avatar_url, role, status, "
                "profile_data, preferences, created_at, updated_at, last_login_at, is_active "
                "FROM users WHERE username ILIKE $1 OR email ILIKE $1 OR nickname ILIKE $1 "
                "ORDER BY id DESC LIMIT $2 OFFSET $3",
                like, pageSize, offset
            );
        }
        txn.commit();
        Yachiyo::Services::DatabasePool::getInstance().releaseConnection(conn);

        for (const auto& row : res) {
            Yachiyo::Models::User u;
            u.id = row.at("id").as<int64_t>();
            u.username = row.at("username").as<std::string>();
            u.email = row.at("email").as<std::string>();
            u.password_hash = row.at("password_hash").as<std::string>();
            u.salt = row.at("salt").is_null() ? "" : row.at("salt").as<std::string>();
            u.nickname = row.at("nickname").is_null() ? "" : row.at("nickname").as<std::string>();
            u.avatar_url = row.at("avatar_url").is_null() ? "" : row.at("avatar_url").as<std::string>();
            u.role = row.at("role").is_null() ? 1 : row.at("role").as<int>();
            u.status = row.at("status").is_null() ? 1 : row.at("status").as<int>();
            u.profile_data = row.at("profile_data").is_null() ? nlohmann::json::object() : nlohmann::json::parse(row.at("profile_data").as<std::string>("{}"));
            u.preferences = row.at("preferences").is_null() ? nlohmann::json::object() : nlohmann::json::parse(row.at("preferences").as<std::string>("{}"));
            u.is_active = row.at("is_active").as<bool>();
            users.push_back(buildUserDTO(u));
        }

        return users;
    } catch (const std::exception& e) {
        logger->error("搜索用户失败: {}", e.what());
        throw;
    }
}

// ==================== 关注 (需要 user_follows 表) ====================

bool UserServiceImpl::followUser(const std::string& followerId, const std::string& followingId) {
    if (followerId == followingId) return false;
    // TODO: INSERT INTO user_follows
    logger->info("关注成功 (待建表): {} → {}", followerId, followingId);
    return true;
}

bool UserServiceImpl::unfollowUser(const std::string& followerId, const std::string& followingId) {
    // TODO: DELETE FROM user_follows
    return true;
}

bool UserServiceImpl::isFollowing(const std::string& followerId, const std::string& followingId) {
    // TODO: SELECT FROM user_follows
    return false;
}

std::vector<dto::UserDTO> UserServiceImpl::getFollowers(const std::string& userId, int page, int pageSize) {
    // TODO: JOIN user_follows
    return {};
}

std::vector<dto::UserDTO> UserServiceImpl::getFollowing(const std::string& userId, int page, int pageSize) {
    // TODO: JOIN user_follows
    return {};
}

// ==================== 统计 ====================

dto::UserStatisticsDTO UserServiceImpl::getUserStatistics(const std::string& userId) {
    try {
        dto::UserStatisticsDTO stats;
        stats.userId = userId;

        if (!g_databaseService || !g_databaseService->isInitialized()) return stats;

        int64_t uid = std::stoll(userId);
        auto result = g_databaseService->userDAO().getById(uid);
        if (result.success) {
            auto& p = result.data.value().profile_data;
            stats.totalPosts = p.value("postsCount", 0);
            stats.totalLikes = p.value("totalLikes", 0);
            stats.totalComments = p.value("totalComments", 0);
            stats.totalViews = p.value("totalViews", 0);
            stats.totalFollowers = p.value("followersCount", 0);
            stats.totalFollowing = p.value("followingCount", 0);
        }

        return stats;
    } catch (const std::exception& e) {
        logger->error("获取用户统计失败: {}", e.what());
        throw;
    }
}

// ==================== 角色管理 ====================

bool UserServiceImpl::updateUserRole(const std::string& userId, const std::string& newRole,
                                    const std::string& adminId) {
    try {
        if (!g_databaseService || !g_databaseService->isInitialized()) {
            throw std::runtime_error("数据库服务未初始化");
        }

        // 验证管理员 — 使用 User.role SMALLINT 字段 (99=admin)
        int64_t aid = std::stoll(adminId);
        auto adminResult = g_databaseService->userDAO().getById(aid);
        if (!adminResult.success || adminResult.data.value().role != 99) {
            throw std::runtime_error("没有管理员权限");
        }

        // 将字符串角色映射为 SMALLINT (1=user, 99=admin)
        int roleValue = 0;
        if (newRole == "user") {
            roleValue = 1;
        } else if (newRole == "admin") {
            roleValue = 99;
        } else {
            throw std::runtime_error("无效的角色，可选值: user, admin");
        }

        int64_t uid = std::stoll(userId);
        auto userResult = g_databaseService->userDAO().getById(uid);
        if (!userResult.success) throw std::runtime_error("用户不存在");

        // 更新 users.role SMALLINT 列
        g_databaseService->userDAO().updateRole(uid, roleValue);

        logger->info("用户角色更新成功: userId={}, newRole={} ({})", userId, newRole, roleValue);
        return true;
    } catch (const std::exception& e) {
        logger->error("更新用户角色失败: {}", e.what());
        return false;
    }
}

bool UserServiceImpl::deactivateUser(const std::string& userId, const std::string& adminId) {
    try {
        if (!g_databaseService || !g_databaseService->isInitialized()) return false;

        int64_t aid = std::stoll(adminId);
        auto adminResult = g_databaseService->userDAO().getById(aid);
        if (!adminResult.success || adminResult.data.value().role != 99) {
            throw std::runtime_error("没有管理员权限");
        }

        g_databaseService->userDAO().delete_(std::stoll(userId));
        logger->info("用户停用成功: userId={}", userId);
        return true;
    } catch (const std::exception& e) {
        logger->error("停用用户失败: {}", e.what());
        return false;
    }
}

bool UserServiceImpl::activateUser(const std::string& userId, const std::string& adminId) {
    try {
        if (!g_databaseService || !g_databaseService->isInitialized()) return false;

        int64_t aid = std::stoll(adminId);
        auto adminResult = g_databaseService->userDAO().getById(aid);
        if (!adminResult.success || adminResult.data.value().role != 99) {
            throw std::runtime_error("没有管理员权限");
        }

        auto conn = Yachiyo::Services::DatabasePool::getInstance().getConnection();
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "UPDATE users SET is_active = TRUE, status = 1, updated_at = NOW() WHERE id = $1",
            std::stoll(userId)
        );
        txn.commit();
        Yachiyo::Services::DatabasePool::getInstance().releaseConnection(conn);

        if (res.affected_rows() <= 0) {
            logger->warn("激活用户未生效，用户可能不存在: userId={}", userId);
            return false;
        }

        logger->info("用户激活成功: userId={}", userId);
        return true;
    } catch (const std::exception& e) {
        logger->error("激活用户失败: {}", e.what());
        return false;
    }
}

// ==================== 通知 (需要 notifications 表) ====================

std::vector<dto::NotificationDTO> UserServiceImpl::getNotifications(const std::string& userId,
                                                                   int page, int pageSize) {
    // TODO: 需要 notifications 表
    return {};
}

bool UserServiceImpl::markNotificationAsRead(const std::string& notificationId,
                                            const std::string& userId) {
    return true;  // TODO
}

bool UserServiceImpl::markAllNotificationsAsRead(const std::string& userId) {
    return true;  // TODO
}

// ==================== 偏好设置 ====================

dto::UserPreferencesDTO UserServiceImpl::getUserPreferences(const std::string& userId) {
    try {
        dto::UserPreferencesDTO prefs;
        prefs.userId = userId;

        if (!g_databaseService || !g_databaseService->isInitialized()) return prefs;

        int64_t uid = std::stoll(userId);
        auto result = g_databaseService->userDAO().getById(uid);
        if (result.success) {
            auto& p = result.data.value().preferences;
            prefs.language = p.value("language", "zh-CN");
            prefs.theme = p.value("theme", "dark");
            prefs.emailNotifications = p.value("emailNotifications", true);
            prefs.pushNotifications = p.value("pushNotifications", true);
            prefs.privacyLevel = p.value("privacyLevel", "friends");
            prefs.contentFilter = p.value("contentFilter", "moderate");
            prefs.autoPlayVideos = p.value("autoPlayVideos", false);
            prefs.showOnlineStatus = p.value("showOnlineStatus", true);
        }

        return prefs;
    } catch (const std::exception& e) {
        logger->error("获取偏好设置失败: {}", e.what());
        throw;
    }
}

bool UserServiceImpl::updateUserPreferences(const std::string& userId,
                                           const dto::UserPreferencesDTO& preferences) {
    try {
        if (!g_databaseService || !g_databaseService->isInitialized()) return false;

        nlohmann::json j;
        j["language"] = preferences.language;
        j["theme"] = preferences.theme;
        j["emailNotifications"] = preferences.emailNotifications;
        j["pushNotifications"] = preferences.pushNotifications;
        j["privacyLevel"] = preferences.privacyLevel;
        j["contentFilter"] = preferences.contentFilter;
        j["autoPlayVideos"] = preferences.autoPlayVideos;
        j["showOnlineStatus"] = preferences.showOnlineStatus;

        g_databaseService->userDAO().updatePreferences(std::stoll(userId), j);
        logger->info("偏好设置更新成功: userId={}", userId);
        return true;
    } catch (const std::exception& e) {
        logger->error("更新偏好设置失败: {}", e.what());
        return false;
    }
}

} // namespace Yachiyo::services
