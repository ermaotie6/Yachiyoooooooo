#pragma once

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace yachiyo::models {

/**
 * @brief 基础模型类
 */
class BaseModel {
protected:
    std::string id;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
    bool deleted;
    
public:
    BaseModel() 
        : id(""), 
          createdAt(std::chrono::system_clock::now()),
          updatedAt(std::chrono::system_clock::now()),
          deleted(false) {}
    
    virtual ~BaseModel() = default;
    
    // Getters
    const std::string& getId() const { return id; }
    std::chrono::system_clock::time_point getCreatedAt() const { return createdAt; }
    std::chrono::system_clock::time_point getUpdatedAt() const { return updatedAt; }
    bool isDeleted() const { return deleted; }
    
    // Setters
    void setId(const std::string& newId) { id = newId; }
    void setCreatedAt(std::chrono::system_clock::time_point time) { createdAt = time; }
    void setUpdatedAt(std::chrono::system_clock::time_point time) { updatedAt = time; }
    void setDeleted(bool isDeleted) { deleted = isDeleted; }
    
    // 更新时间戳
    void updateTimestamp() {
        updatedAt = std::chrono::system_clock::now();
    }
    
    // 转换为JSON
    virtual json toJson() const {
        auto timeToStr = [](std::chrono::system_clock::time_point time) -> std::string {
            auto t = std::chrono::system_clock::to_time_t(time);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
            return ss.str();
        };
        
        return {
            {"id", id},
            {"createdAt", timeToStr(createdAt)},
            {"updatedAt", timeToStr(updatedAt)},
            {"deleted", deleted}
        };
    }
    
    // 从JSON解析
    virtual void fromJson(const json& j) {
        id = j.value("id", "");
        
        auto strToTime = [](const std::string& str) -> std::chrono::system_clock::time_point {
            if (str.empty()) return std::chrono::system_clock::now();
            
            std::tm tm = {};
            std::stringstream ss(str);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) return std::chrono::system_clock::now();
            
            return std::chrono::system_clock::from_time_t(std::mktime(&tm));
        };
        
        createdAt = strToTime(j.value("createdAt", ""));
        updatedAt = strToTime(j.value("updatedAt", ""));
        deleted = j.value("deleted", false);
    }
};

} // namespace yachiyo::models