#include <gtest/gtest.h>
#include <iostream>

/**
 * @brief Google Test 主入口点
 * 
 * 该文件包含所有单元测试的入口。
 * 
 * 编译和运行:
 *   mkdir build && cd build
 *   cmake .. && make
 *   make test  # 或 ctest
 * 
 * 或直接运行测试可执行文件:
 *   ./yachiyo_tests
 * 
 * 显示详细输出:
 *   ./yachiyo_tests --gtest_print_time=1
 * 
 * 运行特定测试:
 *   ./yachiyo_tests --gtest_filter=JwtUtilTest.*
 */

int main(int argc, char** argv) {
    // 初始化 Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // 运行所有测试
    int result = RUN_ALL_TESTS();
    
    return result;
}
