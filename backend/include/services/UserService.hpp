#pragma once

#include "../models/User.hpp"
#include "../dto/UserDTO.hpp"
#include "../utils/Result.hpp"
#include "../utils/LogUtils.hpp"
#include "../utils/HashUtil.hpp"
#include <string>
#include <vector>
#include <memory>
#include <spdlog/spdlog.h>

namespace Yachiyo {
namespace services {

using Utils::Result;
using dto::UserDTO;
using dto::UserUpdateDTO;
using dto::UserStatsDTO;
using dto::UserStatisticsDTO;
using dto::NotificationDTO;
using dto::UserPreferencesDTO;

class UserService {
public:
    virtual ~UserService() = default;
    
    // 获取用户信息
    virtual Result<UserDTO> getUserInfo(const std::string& userId) = 0;
    
    // 更新用户信息
    virtual Result<UserDTO> updateUserInfo(const std::string& userId, const UserUpdateDTO& updateData) = 0;
    
    // 获取用户列表（支持分页）
    virtual Result<std::vector<UserDTO>> getUserList(int page, int pageSize) = 0;
    
    // 更新用户角色
    virtual Result<void> updateUserRole(const std::string& userId, const std::string& newRole) = 0;
    
    // 删除用户
    virtual Result<void> deleteUser(const std::string& userId) = 0;
    
    // 搜索用户
    virtual Result<std::vector<UserDTO>> searchUsers(const std::string& keyword, int page, int pageSize) = 0;
    
    // 获取用户统计信息
    virtual Result<UserStatsDTO> getUserStats(const std::string& userId) = 0;
};

/**
 * @brief 用户服务实现类
 * 
 * 提供用户管理功能：查询、更新、搜索、关注、角色管理、通知、偏好设置等
 */
class UserServiceImpl {
public:
    UserServiceImpl();
    ~UserServiceImpl();

    // ==================== 用户查询 ====================
    dto::UserDTO getUserById(const std::string& userId);
    dto::UserDTO getUserByUsername(const std::string& username);
    
    // ==================== 用户更新 ====================
    dto::UserDTO updateUser(const std::string& userId, const dto::UserDTO& userUpdate);
    bool updatePassword(const std::string& userId,
                       const std::string& oldPassword,
                       const std::string& newPassword);
    bool validatePassword(const std::string& userId, const std::string& password);
    
    // ==================== 搜索 ====================
    std::vector<dto::UserDTO> searchUsers(const std::string& keyword,
                                          int page = 1, int pageSize = 20);
    
    // ==================== 关注 ====================
    bool followUser(const std::string& followerId, const std::string& followingId);
    bool unfollowUser(const std::string& followerId, const std::string& followingId);
    bool isFollowing(const std::string& followerId, const std::string& followingId);
    std::vector<dto::UserDTO> getFollowers(const std::string& userId, int page = 1, int pageSize = 20);
    std::vector<dto::UserDTO> getFollowing(const std::string& userId, int page = 1, int pageSize = 20);
    
    // ==================== 统计 ====================
    dto::UserStatisticsDTO getUserStatistics(const std::string& userId);
    
    // ==================== 角色管理 ====================
    bool updateUserRole(const std::string& userId, const std::string& newRole,
                       const std::string& adminId);
    bool deactivateUser(const std::string& userId, const std::string& adminId);
    bool activateUser(const std::string& userId, const std::string& adminId);
    
    // ==================== 通知 ====================
    std::vector<dto::NotificationDTO> getNotifications(const std::string& userId,
                                                       int page = 1, int pageSize = 20);
    bool markNotificationAsRead(const std::string& notificationId, const std::string& userId);
    bool markAllNotificationsAsRead(const std::string& userId);
    
    // ==================== 偏好设置 ====================
    dto::UserPreferencesDTO getUserPreferences(const std::string& userId);
    bool updateUserPreferences(const std::string& userId,
                              const dto::UserPreferencesDTO& preferences);

private:
    std::shared_ptr<spdlog::logger> logger;
};

} // namespace services
} // namespace Yachiyo

// 向后兼容别名
namespace yachiyo::services {
    using UserService = Yachiyo::services::UserService;
    using UserServiceImpl = Yachiyo::services::UserServiceImpl;
    namespace dto = yachiyo::dto;
    using LogUtils = yachiyo::utils::LogUtils;
    using HashUtil = Yachiyo::Utils::HashUtil;
}