#include "../../include/services/UserService.hpp"
#include "../../include/utils/LogUtils.hpp"
#include "../../include/utils/JsonUtils.hpp"
#include "../../include/utils/ValidationUtils.hpp"
#include "../../include/utils/HashUtil.hpp"
#include <crow.h>
#include <chrono>
#include <algorithm>
#include <regex>

namespace yachiyo::services {

UserServiceImpl::UserServiceImpl() {
    logger = LogUtils::getLogger("UserServiceImpl");
    logger->info("用户服务初始化完成");
}

UserServiceImpl::~UserServiceImpl() {
    logger->info("用户服务销毁");
}

dto::UserDTO UserServiceImpl::getUserById(const std::string& userId) {
    try {
        logger->debug("获取用户信息: userId={}", userId);
        
        // 这里应该从数据库获取用户信息
        // 暂时返回模拟数据
        
        dto::UserDTO user;
        user.id = userId;
        user.username = "user_" + userId.substr(userId.find_last_of('_') + 1);
        user.email = user.username + "@example.com";
        user.displayName = "用户" + userId.substr(userId.find_last_of('_') + 1);
        user.avatarUrl = "https://example.com/avatars/" + userId + ".jpg";
        user.bio = "这是用户 " + user.displayName + " 的个人简介。";
        user.createdAt = std::chrono::system_clock::now() - std::chrono::days(365);
        user.updatedAt = std::chrono::system_clock::now() - std::chrono::days(30);
        user.isActive = true;
        user.isVerified = true;
        user.role = "user";
        user.followersCount = 150;
        user.followingCount = 80;
        user.postsCount = 25;
        
        // 模拟用户统计
        user.statistics.totalLikes = 420;
        user.statistics.totalComments = 150;
        user.statistics.totalViews = 5000;
        user.statistics.achievements = {"活跃用户", "优质创作者"};
        
        logger->debug("成功获取用户信息: userId={}", userId);
        return user;
        
    } catch (const std::exception& e) {
        logger->error("获取用户信息失败: {}", e.what());
        throw;
    }
}

dto::UserDTO UserServiceImpl::getUserByUsername(const std::string& username) {
    try {
        logger->debug("通过用户名获取用户: username={}", username);
        
        // 这里应该从数据库获取用户信息
        // 暂时返回模拟数据
        
        dto::UserDTO user;
        user.id = "user_001";
        user.username = username;
        user.email = username + "@example.com";
        user.displayName = "用户" + username.substr(username.find_last_of('_') + 1);
        user.avatarUrl = "https://example.com/avatars/" + username + ".jpg";
        user.bio = "这是用户 " + user.displayName + " 的个人简介。";
        user.createdAt = std::chrono::system_clock::now() - std::chrono::days(300);
        user.updatedAt = std::chrono::system_clock::now() - std::chrono::days(15);
        user.isActive = true;
        user.isVerified = true;
        user.role = "user";
        user.followersCount = 200;
        user.followingCount = 100;
        user.postsCount = 30;
        
        logger->debug("成功通过用户名获取用户: username={}", username);
        return user;
        
    } catch (const std::exception& e) {
        logger->error("通过用户名获取用户失败: {}", e.what());
        throw;
    }
}

dto::UserDTO UserServiceImpl::updateUser(const std::string& userId, const dto::UserDTO& userUpdate) {
    try {
        logger->debug("更新用户信息: userId={}", userId);
        
        // 获取现有用户信息
        dto::UserDTO existingUser = getUserById(userId);
        
        // 验证更新数据
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
        
        // 这里应该更新数据库中的用户信息
        // 暂时返回模拟数据
        
        dto::UserDTO updatedUser = existingUser;
        
        // 更新字段
        if (!userUpdate.displayName.empty()) {
            updatedUser.displayName = userUpdate.displayName;
        }
        
        if (!userUpdate.email.empty()) {
            updatedUser.email = userUpdate.email;
        }
        
        if (!userUpdate.bio.empty()) {
            updatedUser.bio = userUpdate.bio;
        }
        
        if (!userUpdate.avatarUrl.empty()) {
            updatedUser.avatarUrl = userUpdate.avatarUrl;
        }
        
        updatedUser.updatedAt = std::chrono::system_clock::now();
        
        logger->info("用户信息更新成功: userId={}", userId);
        return updatedUser;
        
    } catch (const std::exception& e) {
        logger->error("更新用户信息失败: {}", e.what());
        throw;
    }
}

bool UserServiceImpl::updatePassword(const std::string& userId, 
                                    const std::string& oldPassword, 
                                    const std::string& newPassword) {
    try {
        logger->debug("更新用户密码: userId={}", userId);
        
        // 验证旧密码
        if (!validatePassword(userId, oldPassword)) {
            throw std::runtime_error("旧密码不正确");
        }
        
        // 验证新密码强度
        if (!ValidationUtils::validatePasswordStrength(newPassword)) {
            throw std::runtime_error("新密码强度不足");
        }
        
        // 哈希新密码
        std::string hashedPassword = HashUtil::hashPassword(newPassword);
        
        // 这里应该更新数据库中的密码
        // 暂时返回模拟成功
        
        logger->info("密码更新成功: userId={}", userId);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("更新密码失败: {}", e.what());
        return false;
    }
}

bool UserServiceImpl::validatePassword(const std::string& userId, const std::string& password) {
    try {
        logger->debug("验证用户密码: userId={}", userId);
        
        // 这里应该从数据库获取哈希密码并验证
        // 暂时返回模拟验证成功
        
        // 模拟密码验证逻辑
        std::string hashedInput = HashUtil::hashPassword(password);
        
        // 模拟存储的哈希密码（实际应从数据库获取）
        std::string storedHash = HashUtil::hashPassword("correct_password");
        
        bool isValid = (hashedInput == storedHash);
        
        logger->debug("密码验证结果: userId={}, isValid={}", userId, isValid);
        return isValid;
        
    } catch (const std::exception& e) {
        logger->error("验证密码失败: {}", e.what());
        return false;
    }
}

std::vector<dto::UserDTO> UserServiceImpl::searchUsers(const std::string& keyword, 
                                                      int page, int pageSize) {
    try {
        logger->debug("搜索用户: keyword={}, page={}, pageSize={}", 
                     keyword, page, pageSize);
        
        // 这里应该从数据库搜索用户
        // 暂时返回模拟数据
        
        std::vector<dto::UserDTO> users;
        
        if (!keyword.empty()) {
            // 模拟搜索结果
            for (int i = 0; i < pageSize && i < 10; i++) {
                dto::UserDTO user;
                user.id = "search_user_" + std::to_string(i + 1);
                user.username = keyword + "_user_" + std::to_string(i + 1);
                user.email = user.username + "@example.com";
                user.displayName = "包含 '" + keyword + "' 的用户 " + std::to_string(i + 1);
                user.avatarUrl = "https://example.com/avatars/search_" + std::to_string(i + 1) + ".jpg";
                user.bio = "这是搜索到的用户，包含关键词 '" + keyword + "'。";
                user.createdAt = std::chrono::system_clock::now() - std::chrono::days(100 + i);
                user.updatedAt = std::chrono::system_clock::now() - std::chrono::days(i);
                user.isActive = true;
                user.isVerified = (i % 2 == 0);
                user.role = "user";
                user.followersCount = i * 20;
                user.followingCount = i * 10;
                user.postsCount = i * 5;
                
                users.push_back(user);
            }
        }
        
        logger->debug("搜索到 {} 个相关用户", users.size());
        return users;
        
    } catch (const std::exception& e) {
        logger->error("搜索用户失败: {}", e.what());
        throw;
    }
}

bool UserServiceImpl::followUser(const std::string& followerId, const std::string& followingId) {
    try {
        logger->debug("关注用户: followerId={}, followingId={}", 
                     followerId, followingId);
        
        // 检查是否已关注
        if (isFollowing(followerId, followingId)) {
            throw std::runtime_error("已经关注该用户");
        }
        
        // 检查不能关注自己
        if (followerId == followingId) {
            throw std::runtime_error("不能关注自己");
        }
        
        // 这里应该保存关注关系到数据库
        // 暂时返回模拟成功
        
        logger->info("关注成功: followerId={}, followingId={}", 
                    followerId, followingId);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("关注用户失败: {}", e.what());
        return false;
    }
}

bool UserServiceImpl::unfollowUser(const std::string& followerId, const std::string& followingId) {
    try {
        logger->debug("取消关注: followerId={}, followingId={}", 
                     followerId, followingId);
        
        // 检查是否已关注
        if (!isFollowing(followerId, followingId)) {
            throw std::runtime_error("未关注该用户");
        }
        
        // 这里应该从数据库删除关注关系
        // 暂时返回模拟成功
        
        logger->info("取消关注成功: followerId={}, followingId={}", 
                    followerId, followingId);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("取消关注失败: {}", e.what());
        return false;
    }
}

bool UserServiceImpl::isFollowing(const std::string& followerId, const std::string& followingId) {
    try {
        logger->debug("检查关注状态: followerId={}, followingId={}", 
                     followerId, followingId);
        
        // 这里应该从数据库检查关注关系
        // 暂时返回模拟数据
        
        // 模拟关注关系：用户1关注用户2和用户3
        bool isFollowing = false;
        
        if (followerId == "user_001") {
            if (followingId == "user_002" || followingId == "user_003") {
                isFollowing = true;
            }
        } else if (followerId == "user_002") {
            if (followingId == "user_001") {
                isFollowing = true;
            }
        }
        
        logger->debug("关注状态: followerId={}, followingId={}, isFollowing={}", 
                     followerId, followingId, isFollowing);
        return isFollowing;
        
    } catch (const std::exception& e) {
        logger->error("检查关注状态失败: {}", e.what());
        return false;
    }
}

std::vector<dto::UserDTO> UserServiceImpl::getFollowers(const std::string& userId, 
                                                       int page, int pageSize) {
    try {
        logger->debug("获取粉丝列表: userId={}, page={}, pageSize={}", 
                     userId, page, pageSize);
        
        // 这里应该从数据库获取粉丝列表
        // 暂时返回模拟数据
        
        std::vector<dto::UserDTO> followers;
        int startIndex = (page - 1) * pageSize;
        int endIndex = startIndex + pageSize;
        
        // 模拟粉丝数据
        for (int i = startIndex; i < endIndex && i < 20; i++) {
            dto::UserDTO follower;
            follower.id = "follower_" + std::to_string(i + 1);
            follower.username = "follower_user_" + std::to_string(i + 1);
            follower.email = follower.username + "@example.com";
            follower.displayName = "粉丝用户 " + std::to_string(i + 1);
            follower.avatarUrl = "https://example.com/avatars/follower_" + std::to_string(i + 1) + ".jpg";
            follower.bio = "这是 " + userId + " 的粉丝。";
            follower.createdAt = std::chrono::system_clock::now() - std::chrono::days(50 + i);
            follower.updatedAt = std::chrono::system_clock::now() - std::chrono::days(i);
            follower.isActive = true;
            follower.isVerified = (i % 3 == 0);
            follower.role = "user";
            follower.followersCount = i * 5;
            follower.followingCount = i * 3;
            follower.postsCount = i * 2;
            
            followers.push_back(follower);
        }
        
        logger->debug("成功获取 {} 个粉丝", followers.size());
        return followers;
        
    } catch (const std::exception& e) {
        logger->error("获取粉丝列表失败: {}", e.what());
        throw;
    }
}

std::vector<dto::UserDTO> UserServiceImpl::getFollowing(const std::string& userId, 
                                                       int page, int pageSize) {
    try {
        logger->debug("获取关注列表: userId={}, page={}, pageSize={}", 
                     userId, page, pageSize);
        
        // 这里应该从数据库获取关注列表
        // 暂时返回模拟数据
        
        std::vector<dto::UserDTO> following;
        int startIndex = (page - 1) * pageSize;
        int endIndex = startIndex + pageSize;
        
        // 模拟关注数据
        for (int i = startIndex; i < endIndex && i < 15; i++) {
            dto::UserDTO followedUser;
            followedUser.id = "following_" + std::to_string(i + 1);
            followedUser.username = "following_user_" + std::to_string(i + 1);
            followedUser.email = followedUser.username + "@example.com";
            followedUser.displayName = "关注用户 " + std::to_string(i + 1);
            followedUser.avatarUrl = "https://example.com/avatars/following_" + std::to_string(i + 1) + ".jpg";
            followedUser.bio = "这是 " + userId + " 关注的用户。";
            followedUser.createdAt = std::chrono::system_clock::now() - std::chrono::days(80 + i);
            followedUser.updatedAt = std::chrono::system_clock::now() - std::chrono::days(i * 2);
            followedUser.isActive = true;
            followedUser.isVerified = (i % 2 == 0);
            followedUser.role = "user";
            followedUser.followersCount = i * 10;
            followedUser.followingCount = i * 5;
            followedUser.postsCount = i * 3;
            
            following.push_back(followedUser);
        }
        
        logger->debug("成功获取 {} 个关注用户", following.size());
        return following;
        
    } catch (const std::exception& e) {
        logger->error("获取关注列表失败: {}", e.what());
        throw;
    }
}

dto::UserStatisticsDTO UserServiceImpl::getUserStatistics(const std::string& userId) {
    try {
        logger->debug("获取用户统计: userId={}", userId);
        
        // 这里应该从数据库计算用户统计
        // 暂时返回模拟数据
        
        dto::UserStatisticsDTO stats;
        stats.userId = userId;
        stats.totalPosts = 25;
        stats.totalLikes = 420;
        stats.totalComments = 150;
        stats.totalViews = 5000;
        stats.totalFollowers = 150;
        stats.totalFollowing = 80;
        stats.dailyActiveDays = 180;
        stats.weeklyActiveDays = 5;
        stats.monthlyActiveDays = 20;
        
        // 模拟成就
        stats.achievements = {
            "活跃用户",
            "优质创作者",
            "社区贡献者",
            "月度之星"
        };
        
        // 模拟活动趋势
        stats.activityTrend = {
            {"2024-01", 120},
            {"2024-02", 150},
            {"2024-03", 180},
            {"2024-04", 200},
            {"2024-05", 220}
        };
        
        logger->debug("成功获取用户统计: userId={}", userId);
        return stats;
        
    } catch (const std::exception& e) {
        logger->error("获取用户统计失败: {}", e.what());
        throw;
    }
}

bool UserServiceImpl::updateUserRole(const std::string& userId, const std::string& newRole, 
                                    const std::string& adminId) {
    try {
        logger->debug("更新用户角色: userId={}, newRole={}, adminId={}", 
                     userId, newRole, adminId);
        
        // 验证管理员权限
        dto::UserDTO adminUser = getUserById(adminId);
        if (adminUser.role != "admin") {
            throw std::runtime_error("没有管理员权限");
        }
        
        // 验证角色有效性
        std::vector<std::string> validRoles = {"user", "moderator", "admin"};
        if (std::find(validRoles.begin(), validRoles.end(), newRole) == validRoles.end()) {
            throw std::runtime_error("无效的角色");
        }
        
        // 这里应该更新数据库中的用户角色
        // 暂时返回模拟成功
        
        logger->info("用户角色更新成功: userId={}, newRole={}", userId, newRole);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("更新用户角色失败: {}", e.what());
        return false;
    }
}

bool UserServiceImpl::deactivateUser(const std::string& userId, const std::string& adminId) {
    try {
        logger->debug("停用用户: userId={}, adminId={}", userId, adminId);
        
        // 验证管理员权限
        dto::UserDTO adminUser = getUserById(adminId);
        if (adminUser.role != "admin") {
            throw std::runtime_error("没有管理员权限");
        }
        
        // 这里应该更新数据库中的用户状态
        // 暂时返回模拟成功
        
        logger->info("用户停用成功: userId={}", userId);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("停用用户失败: {}", e.what());
        return false;
    }
}

bool UserServiceImpl::activateUser(const std::string& userId, const std::string& adminId) {
    try {
        logger->debug("激活用户: userId={}, adminId={}", userId, adminId);
        
        // 验证管理员权限
        dto::UserDTO adminUser = getUserById(adminId);
        if (adminUser.role != "admin") {
            throw std::runtime_error("没有管理员权限");
        }
        
        // 这里应该更新数据库中的用户状态
        // 暂时返回模拟成功
        
        logger->info("用户激活成功: userId={}", userId);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("激活用户失败: {}", e.what());
        return false;
    }
}

std::vector<dto::NotificationDTO> UserServiceImpl::getNotifications(const std::string& userId, 
                                                                   int page, int pageSize) {
    try {
        logger->debug("获取用户通知: userId={}, page={}, pageSize={}", 
                     userId, page, pageSize);
        
        // 这里应该从数据库获取用户通知
        // 暂时返回模拟数据
        
        std::vector<dto::NotificationDTO> notifications;
        int startIndex = (page - 1) * pageSize;
        int endIndex = startIndex + pageSize;
        
        // 模拟通知数据
        std::vector<std::string> notificationTypes = {
            "like", "comment", "follow", "mention", "system"
        };
        
        std::vector<std::string> notificationMessages = {
            "点赞了你的帖子",
            "评论了你的帖子",
            "关注了你",
            "在评论中提到了你",
            "系统通知：欢迎使用新功能"
        };
        
        for (int i = startIndex; i < endIndex && i < 10; i++) {
            dto::NotificationDTO notification;
            notification.id = "notification_" + std::to_string(i + 1);
            notification.userId = userId;
            notification.type = notificationTypes[i % notificationTypes.size()];
            notification.message = "用户" + std::to_string(i % 5 + 1) + notificationMessages[i % notificationMessages.size()];
            notification.isRead = (i < 3); // 前3条已读
            notification.createdAt = std::chrono::system_clock::now() - std::chrono::minutes(i * 30);
            notification.metadata = {
                {"postId", "post_" + std::to_string(i + 1)},
                {"actorId", "user_" + std::to_string(i % 5 + 1)}
            };
            
            notifications.push_back(notification);
        }
        
        logger->debug("成功获取 {} 条通知", notifications.size());
        return notifications;
        
    } catch (const std::exception& e) {
        logger->error("获取用户通知失败: {}", e.what());
        throw;
    }
}

bool UserServiceImpl::markNotificationAsRead(const std::string& notificationId, 
                                            const std::string& userId) {
    try {
        logger->debug("标记通知为已读: notificationId={}, userId={}", 
                     notificationId, userId);
        
        // 这里应该更新数据库中的通知状态
        // 暂时返回模拟成功
        
        logger->info("通知标记为已读成功: notificationId={}", notificationId);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("标记通知为已读失败: {}", e.what());
        return false;
    }
}

bool UserServiceImpl::markAllNotificationsAsRead(const std::string& userId) {
    try {
        logger->debug("标记所有通知为已读: userId={}", userId);
        
        // 这里应该批量更新数据库中的通知状态
        // 暂时返回模拟成功
        
        logger->info("所有通知标记为已读成功: userId={}", userId);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("标记所有通知为已读失败: {}", e.what());
        return false;
    }
}

dto::UserPreferencesDTO UserServiceImpl::getUserPreferences(const std::string& userId) {
    try {
        logger->debug("获取用户偏好设置: userId={}", userId);
        
        // 这里应该从数据库获取用户偏好设置
        // 暂时返回模拟数据
        
        dto::UserPreferencesDTO preferences;
        preferences.userId = userId;
        preferences.language = "zh-CN";
        preferences.theme = "dark";
        preferences.emailNotifications = true;
        preferences.pushNotifications = true;
        preferences.privacyLevel = "friends";
        preferences.contentFilter = "moderate";
        preferences.autoPlayVideos = false;
        preferences.showOnlineStatus = true;
        preferences.createdAt = std::chrono::system_clock::now() - std::chrono::days(100);
        preferences.updatedAt = std::chrono::system_clock::now() - std::chrono::days(7);
        
        logger->debug("成功获取用户偏好设置: userId={}", userId);
        return preferences;
        
    } catch (const std::exception& e) {
        logger->error("获取用户偏好设置失败: {}", e.what());
        throw;
    }
}

bool UserServiceImpl::updateUserPreferences(const std::string& userId, 
                                           const dto::UserPreferencesDTO& preferences) {
    try {
        logger->debug("更新用户偏好设置: userId={}", userId);
        
        // 这里应该更新数据库中的用户偏好设置
        // 暂时返回模拟成功
        
        logger->info("用户偏好设置更新成功: userId={}", userId);
        return true;
        
    } catch (const std::exception& e) {
        logger->error("更新用户偏好设置失败: {}", e.what());
        return false;
    }
}

} // namespace yachiyo::services