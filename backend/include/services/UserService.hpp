#pragma once

#include "models/User.hpp"
#include "dto/UserDTO.hpp"
#include "utils/Result.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Yachiyo {
namespace services {

class UserService {
public:
    virtual ~UserService() = default;
    
    // 获取用户信息
    virtual Utils::Result<dto::UserDTO> getUserInfo(const std::string& userId) = 0;
    
    // 更新用户信息
    virtual Utils::Result<dto::UserDTO> updateUserInfo(const std::string& userId, const dto::UserUpdateDTO& updateData) = 0;
    
    // 获取用户列表（支持分页）
    virtual Utils::Result<std::vector<dto::UserDTO>> getUserList(int page, int pageSize) = 0;
    
    // 更新用户角色
    virtual Utils::Result<void> updateUserRole(const std::string& userId, const std::string& newRole) = 0;
    
    // 删除用户
    virtual Utils::Result<void> deleteUser(const std::string& userId) = 0;
    
    // 搜索用户
    virtual Utils::Result<std::vector<dto::UserDTO>> searchUsers(const std::string& keyword, int page, int pageSize) = 0;
    
    // 获取用户统计信息
    virtual Utils::Result<dto::UserStatsDTO> getUserStats(const std::string& userId) = 0;
};

} // namespace services
} // namespace Yachiyo