#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <memory>

// Stub 全局变量 — 测试环境中数据库/WebSocket 不可用
// 对应 Application.cpp 中的 extern 定义
namespace Yachiyo::Utils { class DatabaseUtil; }
namespace Yachiyo::Services { class DatabaseService; class WebSocketService; }

std::shared_ptr<Yachiyo::Utils::DatabaseUtil> g_databaseUtil = nullptr;
std::shared_ptr<Yachiyo::Services::DatabaseService> g_databaseService = nullptr;
std::shared_ptr<Yachiyo::Services::WebSocketService> g_webSocketService = nullptr;

int main(int argc, char** argv) {
    // 初始化 Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // 设置日志级别 (测试时使用 warn 减少输出干扰)
    spdlog::set_level(spdlog::level::warn);
    
    return RUN_ALL_TESTS();
}
