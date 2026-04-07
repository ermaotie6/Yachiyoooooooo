#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
    // 初始化 Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // 设置日志级别 (测试时使用 warn 减少输出干扰)
    spdlog::set_level(spdlog::level::warn);
    
    return RUN_ALL_TESTS();
}
