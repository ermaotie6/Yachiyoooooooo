#pragma once

#include <crow.h>
#include <memory>
#include <vector>
#include <string>

namespace Yachiyo {
namespace controllers {

// 前向声明
class BaseController;

/**
 * @brief 路由管理器
 * 
 * 负责管理所有控制器的路由注册
 */
class RouteManager {
public:
    /**
     * @brief 获取路由管理器单例
     * @return 路由管理器实例
     */
    static RouteManager& getInstance();
    
    /**
     * @brief 注册控制器
     * @param controller 控制器指针
     * @param basePath 基础路径
     */
    void registerController(std::shared_ptr<BaseController> controller, const std::string& basePath = "");
    
    /**
     * @brief 注册所有路由到应用
     * @param app Crow应用实例
     */
    void registerAllRoutes(crow::SimpleApp& app);
    
    /**
     * @brief 获取所有控制器
     * @return 控制器列表
     */
    std::vector<std::shared_ptr<BaseController>> getControllers() const;
    
    /**
     * @brief 获取控制器数量
     * @return 控制器数量
     */
    size_t getControllerCount() const;
    
    /**
     * @brief 清空所有控制器
     */
    void clearControllers();
    
    /**
     * @brief 根据路径查找控制器
     * @param path 路径
     * @return 控制器指针，如果未找到返回nullptr
     */
    std::shared_ptr<BaseController> findControllerByPath(const std::string& path) const;

private:
    RouteManager() = default;
    ~RouteManager() = default;
    
    // 禁止拷贝
    RouteManager(const RouteManager&) = delete;
    RouteManager& operator=(const RouteManager&) = delete;
    
    struct ControllerEntry {
        std::shared_ptr<BaseController> controller;
        std::string basePath;
        std::string description;
    };
    
    std::vector<ControllerEntry> controllers;
};

/**
 * @brief 路由注册辅助类
 */
class RouteRegistrar {
public:
    /**
     * @brief 构造函数
     * @param app Crow应用实例
     */
    explicit RouteRegistrar(crow::SimpleApp& app);
    
    /**
     * @brief 注册GET路由
     * @param rule 路由规则
     * @param handler 处理函数
     * @return 路由注册器引用
     */
    RouteRegistrar& get(const std::string& rule, std::function<void(const crow::request&, crow::response&)> handler);
    
    /**
     * @brief 注册POST路由
     * @param rule 路由规则
     * @param handler 处理函数
     * @return 路由注册器引用
     */
    RouteRegistrar& post(const std::string& rule, std::function<void(const crow::request&, crow::response&)> handler);
    
    /**
     * @brief 注册PUT路由
     * @param rule 路由规则
     * @param handler 处理函数
     * @return 路由注册器引用
     */
    RouteRegistrar& put(const std::string& rule, std::function<void(const crow::request&, crow::response&)> handler);
    
    /**
     * @brief 注册DELETE路由
     * @param rule 路由规则
     * @param handler 处理函数
     * @return 路由注册器引用
     */
    RouteRegistrar& delete_(const std::string& rule, std::function<void(const crow::request&, crow::response&)> handler);
    
    /**
     * @brief 注册PATCH路由
     * @param rule 路由规则
     * @param handler 处理函数
     * @return 路由注册器引用
     */
    RouteRegistrar& patch(const std::string& rule, std::function<void(const crow::request&, crow::response&)> handler);
    
    /**
     * @brief 注册OPTIONS路由
     * @param rule 路由规则
     * @param handler 处理函数
     * @return 路由注册器引用
     */
    RouteRegistrar& options(const std::string& rule, std::function<void(const crow::request&, crow::response&)> handler);
    
    /**
     * @brief 注册HEAD路由
     * @param rule 路由规则
     * @param handler 处理函数
     * @return 路由注册器引用
     */
    RouteRegistrar& head(const std::string& rule, std::function<void(const crow::request&, crow::response&)> handler);
    
    /**
     * @brief 注册支持所有HTTP方法的路由
     * @param rule 路由规则
     * @param handler 处理函数
     * @return 路由注册器引用
     */
    RouteRegistrar& all(const std::string& rule, std::function<void(const crow::request&, crow::response&)> handler);
    
    /**
     * @brief 设置路由描述
     * @param description 描述文本
     * @return 路由注册器引用
     */
    RouteRegistrar& description(const std::string& description);
    
    /**
     * @brief 设置路由标签
     * @param tags 标签列表
     * @return 路由注册器引用
     */
    RouteRegistrar& tags(const std::vector<std::string>& tags);
    
    /**
     * @brief 设置路由名称
     * @param name 路由名称
     * @return 路由注册器引用
     */
    RouteRegistrar& name(const std::string& name);

private:
    crow::SimpleApp& app;
    std::string currentRule;
    std::string currentDescription;
    std::vector<std::string> currentTags;
    std::string currentName;
};

/**
 * @brief 路由工具函数
 */
namespace RouteUtils {
    
    /**
     * @brief 标准化路径
     * @param path 原始路径
     * @return 标准化后的路径
     */
    std::string normalizePath(const std::string& path);
    
    /**
     * @brief 提取路径参数
     * @param path 路径
     * @return 参数列表
     */
    std::vector<std::string> extractPathParams(const std::string& path);
    
    /**
     * @brief 构建完整路径
     * @param basePath 基础路径
     * @param relativePath 相对路径
     * @return 完整路径
     */
    std::string buildFullPath(const std::string& basePath, const std::string& relativePath);
    
    /**
     * @brief 验证路径是否有效
     * @param path 路径
     * @return 是否有效
     */
    bool isValidPath(const std::string& path);
    
    /**
     * @brief 获取路径的HTTP方法
     * @param path 路径
     * @return HTTP方法列表
     */
    std::vector<std::string> getPathMethods(const std::string& path);
    
    /**
     * @brief 生成路由文档
     * @param basePath 基础路径
     * @param routes 路由列表
     * @return 文档字符串
     */
    std::string generateRouteDocumentation(const std::string& basePath, 
                                          const std::vector<std::string>& routes);
}

} // namespace controllers
} // namespace Yachiyo