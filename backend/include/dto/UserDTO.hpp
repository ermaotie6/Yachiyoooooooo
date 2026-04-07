#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Yachiyo {
namespace DTO {

/**
 * @brief 用户统计信息内嵌结构
 */
struct UserStatistics {
    int totalLikes = 0;
    int totalComments = 0;
    int totalViews = 0;
};

/**
 * @brief 用户数据传输对象
 */
struct UserDTO {
    std::string id;              // 字符串形式的用户ID
    int64_t userId = 0;          // 数值形式的用户ID
    std::string username;
    std::string email;
    std::string nickname;
    std::string displayName;     // 显示名称
    std::string avatar;
    std::string avatarUrl;       // 头像URL (别名)
    std::string bio;
    std::string role;
    std::string status;
    std::string createdAt;
    bool isActive = true;
    bool isVerified = false;
    int followersCount = 0;
    int followingCount = 0;
    int postsCount = 0;
    UserStatistics statistics;
    
    json toJson() const {
        return {
            {"id", id},
            {"userId", userId},
            {"username", username},
            {"email", email},
            {"nickname", nickname},
            {"displayName", displayName},
            {"avatar", avatar},
            {"avatarUrl", avatarUrl},
            {"bio", bio},
            {"role", role},
            {"status", status},
            {"createdAt", createdAt},
            {"isActive", isActive},
            {"isVerified", isVerified},
            {"followersCount", followersCount},
            {"followingCount", followingCount},
            {"postsCount", postsCount}
        };
    }
    
    static UserDTO fromJson(const json& j) {
        UserDTO dto;
        dto.id = j.value("id", "");
        dto.userId = j.value("userId", int64_t(0));
        dto.username = j.value("username", "");
        dto.email = j.value("email", "");
        dto.nickname = j.value("nickname", "");
        dto.displayName = j.value("displayName", "");
        dto.avatar = j.value("avatar", "");
        dto.avatarUrl = j.value("avatarUrl", "");
        dto.bio = j.value("bio", "");
        dto.role = j.value("role", "user");
        dto.status = j.value("status", "active");
        dto.createdAt = j.value("createdAt", "");
        dto.isActive = j.value("isActive", true);
        dto.isVerified = j.value("isVerified", false);
        return dto;
    }
};

/**
 * @brief 用户更新数据传输对象
 */
struct UserUpdateDTO {
    std::string nickname;
    std::string avatar;
    std::string bio;
    
    json toJson() const {
        return {
            {"nickname", nickname},
            {"avatar", avatar},
            {"bio", bio}
        };
    }
    
    static UserUpdateDTO fromJson(const json& j) {
        UserUpdateDTO dto;
        dto.nickname = j.value("nickname", "");
        dto.avatar = j.value("avatar", "");
        dto.bio = j.value("bio", "");
        return dto;
    }
};

/**
 * @brief 用户统计数据传输对象
 */
struct UserStatsDTO {
    int64_t userId = 0;
    int64_t messagesSent = 0;
    int64_t messagesApproved = 0;
    int64_t messagesRejected = 0;
    int16_t warningsCount = 0;
    
    json toJson() const {
        return {
            {"userId", userId},
            {"messagesSent", messagesSent},
            {"messagesApproved", messagesApproved},
            {"messagesRejected", messagesRejected},
            {"warningsCount", warningsCount}
        };
    }
};

/**
 * @brief 用户详细统计数据传输对象 (UserService.cpp 使用)
 */
struct UserStatisticsDTO {
    std::string userId;
    int totalPosts = 0;
    int totalLikes = 0;
    int totalComments = 0;
    int totalViews = 0;
    int totalFollowers = 0;
    int totalFollowing = 0;
    
    json toJson() const {
        return {
            {"userId", userId},
            {"totalPosts", totalPosts},
            {"totalLikes", totalLikes},
            {"totalComments", totalComments},
            {"totalViews", totalViews},
            {"totalFollowers", totalFollowers},
            {"totalFollowing", totalFollowing}
        };
    }
};

/**
 * @brief 通知数据传输对象
 */
struct NotificationDTO {
    std::string id;
    std::string userId;
    std::string type;       // "follow", "like", "comment", "system"
    std::string title;
    std::string content;
    std::string fromUserId;
    bool isRead = false;
    std::string createdAt;
    
    json toJson() const {
        return {
            {"id", id},
            {"userId", userId},
            {"type", type},
            {"title", title},
            {"content", content},
            {"fromUserId", fromUserId},
            {"isRead", isRead},
            {"createdAt", createdAt}
        };
    }
};

/**
 * @brief 用户偏好设置数据传输对象
 */
struct UserPreferencesDTO {
    std::string userId;
    std::string language = "zh-CN";
    std::string theme = "dark";
    bool emailNotifications = true;
    bool pushNotifications = true;
    std::string privacyLevel = "friends";
    std::string contentFilter = "moderate";
    bool autoPlayVideos = false;
    bool showOnlineStatus = true;
    
    json toJson() const {
        return {
            {"userId", userId},
            {"language", language},
            {"theme", theme},
            {"emailNotifications", emailNotifications},
            {"pushNotifications", pushNotifications},
            {"privacyLevel", privacyLevel},
            {"contentFilter", contentFilter},
            {"autoPlayVideos", autoPlayVideos},
            {"showOnlineStatus", showOnlineStatus}
        };
    }
};

} // namespace DTO
} // namespace Yachiyo

// 后向兼容别名
namespace Yachiyo::dto {
    using Yachiyo::DTO::UserDTO;
    using Yachiyo::DTO::UserUpdateDTO;
    using Yachiyo::DTO::UserStatsDTO;
    using Yachiyo::DTO::UserStatisticsDTO;
    using Yachiyo::DTO::NotificationDTO;
    using Yachiyo::DTO::UserPreferencesDTO;
    using Yachiyo::DTO::UserStatistics;
}

namespace yachiyo::dto {
    using Yachiyo::DTO::UserDTO;
    using Yachiyo::DTO::UserUpdateDTO;
    using Yachiyo::DTO::UserStatsDTO;
    using Yachiyo::DTO::UserStatisticsDTO;
    using Yachiyo::DTO::NotificationDTO;
    using Yachiyo::DTO::UserPreferencesDTO;
    using Yachiyo::DTO::UserStatistics;
}

