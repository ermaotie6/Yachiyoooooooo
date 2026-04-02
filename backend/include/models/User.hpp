#pragma once

#include "BaseModel.hpp"
#include <string>
#include <ctime>

namespace yachiyo::models {

/**
 * @brief 用户角色枚举 (v2.0 - 虚拟主播直播平台)
 * - 1: 普通用户
 * - 99: 管理员
 */
enum class UserRole {
    USER = 1,       // 普通用户
    ADMIN = 99      // 管理员
};

/**
 * @brief 用户状态枚举
 * - 1: 活跃
 * - 2: 禁用
 * - 3: 封禁
 */
enum class UserStatus {
    ACTIVE = 1,     // 活跃
    DISABLED = 2,   // 禁用
    BANNED = 3      // 封禁
};

/**
 * @brief 用户模型 (v2.0 - 虚拟主播直播平台)
 */
class User : public BaseModel {
private:
    std::string username;
    std::string email;
    std::string passwordHash;
    std::string salt;
    std::string nickname;
    std::string avatar;
    std::string bio;
    UserRole role = UserRole::USER;
    UserStatus status = UserStatus::ACTIVE;
    
    // 消息统计
    int64_t messagesSent = 0;
    int64_t messagesApproved = 0;
    int64_t messagesRejected = 0;
    int16_t warningsCount = 0;
    
    // 封禁信息
    bool isBanned = false;
    std::string banReason;
    time_t banUntil = 0;
    
    std::string lastLoginIp;
    std::chrono::system_clock::time_point lastLoginTime;
    
public:
    User() 
        : username(""), email(""), passwordHash(""), salt(""), nickname(""),
          avatar(""), bio(""), role(UserRole::USER), status(UserStatus::ACTIVE),
          messagesSent(0), messagesApproved(0), messagesRejected(0), warningsCount(0),
          isBanned(false), banReason(""), banUntil(0),
          lastLoginIp(""), lastLoginTime(std::chrono::system_clock::now()) {}
    
    // ==================== Getters ====================
    const std::string& getUsername() const { return username; }
    const std::string& getEmail() const { return email; }
    const std::string& getPasswordHash() const { return passwordHash; }
    const std::string& getSalt() const { return salt; }
    const std::string& getNickname() const { return nickname; }
    const std::string& getAvatar() const { return avatar; }
    const std::string& getBio() const { return bio; }
    UserRole getRole() const { return role; }
    UserStatus getStatus() const { return status; }
    
    int64_t getMessagesSent() const { return messagesSent; }
    int64_t getMessagesApproved() const { return messagesApproved; }
    int64_t getMessagesRejected() const { return messagesRejected; }
    int16_t getWarningsCount() const { return warningsCount; }
    bool getIsBanned() const { return isBanned; }
    const std::string& getBanReason() const { return banReason; }
    time_t getBanUntil() const { return banUntil; }
    
    const std::string& getLastLoginIp() const { return lastLoginIp; }
    std::chrono::system_clock::time_point getLastLoginTime() const { return lastLoginTime; }
    
    // ==================== Setters ====================
    void setUsername(const std::string& newUsername) { username = newUsername; }
    void setEmail(const std::string& newEmail) { email = newEmail; }
    void setPasswordHash(const std::string& newPasswordHash) { passwordHash = newPasswordHash; }
    void setSalt(const std::string& newSalt) { salt = newSalt; }
    void setNickname(const std::string& newNickname) { nickname = newNickname; }
    void setAvatar(const std::string& newAvatar) { avatar = newAvatar; }
    void setBio(const std::string& newBio) { bio = newBio; }
    void setRole(UserRole newRole) { role = newRole; }
    void setStatus(UserStatus newStatus) { status = newStatus; }
    
    void setMessagesSent(int64_t count) { messagesSent = count; }
    void setMessagesApproved(int64_t count) { messagesApproved = count; }
    void setMessagesRejected(int64_t count) { messagesRejected = count; }
    void setWarningsCount(int16_t count) { warningsCount = count; }
    void setBanned(bool banned, const std::string& reason = "", time_t until = 0) {
        isBanned = banned;
        banReason = reason;
        banUntil = until;
    }
    
    void setLastLoginIp(const std::string& ip) { lastLoginIp = ip; }
    void setLastLoginTime(std::chrono::system_clock::time_point time) { lastLoginTime = time; }
    
    // ==================== 辅助方法 ====================
    bool isAdmin() const { return role == UserRole::ADMIN; }
    bool isActive() const { return status == UserStatus::ACTIVE; }
    bool canSendMessages() const { return !isBanned && status == UserStatus::ACTIVE; }
    
    double getApprovalRate() const {
        if (messagesSent == 0) return 0.0;
        return static_cast<double>(messagesApproved) / messagesSent * 100.0;
    }
    
    // ==================== JSON 序列化 ====================
    json toJson() const override {
        json j = BaseModel::toJson();
        
        std::string roleStr = (role == UserRole::ADMIN) ? "admin" : "user";
        std::string statusStr;
        switch (status) {
            case UserStatus::ACTIVE: statusStr = "active"; break;
            case UserStatus::DISABLED: statusStr = "disabled"; break;
            case UserStatus::BANNED: statusStr = "banned"; break;
        }
        
        j["username"] = username;
        j["email"] = email;
        j["nickname"] = nickname;
        j["avatar"] = avatar;
        j["bio"] = bio;
        j["role"] = roleStr;
        j["status"] = statusStr;
        j["messagesSent"] = messagesSent;
        j["messagesApproved"] = messagesApproved;
        j["messagesRejected"] = messagesRejected;
        j["warningsCount"] = warningsCount;
        j["isBanned"] = isBanned;
        j["banReason"] = banReason;
        j["approvalRate"] = getApprovalRate();
        
        return j;
    }
    
    // ==================== JSON 反序列化 ====================
    void fromJson(const json& j) override {
        BaseModel::fromJson(j);
        
        username = j.value("username", "");
        email = j.value("email", "");
        nickname = j.value("nickname", "");
        avatar = j.value("avatar", "");
        bio = j.value("bio", "");
        
        std::string roleStr = j.value("role", "user");
        role = (roleStr == "admin") ? UserRole::ADMIN : UserRole::USER;
        
        std::string statusStr = j.value("status", "active");
        if (statusStr == "disabled") status = UserStatus::DISABLED;
        else if (statusStr == "banned") status = UserStatus::BANNED;
        else status = UserStatus::ACTIVE;
        
        if (j.contains("messagesSent")) messagesSent = j["messagesSent"].get<int64_t>();
        if (j.contains("messagesApproved")) messagesApproved = j["messagesApproved"].get<int64_t>();
        if (j.contains("messagesRejected")) messagesRejected = j["messagesRejected"].get<int64_t>();
        if (j.contains("warningsCount")) warningsCount = j["warningsCount"].get<int16_t>();
        if (j.contains("isBanned")) isBanned = j["isBanned"].get<bool>();
        if (j.contains("banReason")) banReason = j["banReason"].get<std::string>();
    }
};

} // namespace yachiyo::models
