cmake_minimum_required(VERSION 3.20)

# ==================== Google Test 配置 ====================

# 方法1: 使用已安装的 GTest
find_package(GTest CONFIG QUIET)

# 方法2: FetchContent 下载 GTest (如果未安装)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
endif()

# ==================== 包含路径 ====================

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# ==================== 测试可执行文件 ====================

# AuthService 单元测试
add_executable(AuthServiceTest AuthServiceTest.cpp)
target_link_libraries(AuthServiceTest
    PRIVATE
    gtest
    gtest_main
    pthread
)
add_test(NAME AuthServiceTest COMMAND AuthServiceTest)

# MessageService 单元测试
add_executable(MessageServiceTest MessageServiceTest.cpp)
target_link_libraries(MessageServiceTest
    PRIVATE
    gtest
    gtest_main
    pthread
)
add_test(NAME MessageServiceTest COMMAND MessageServiceTest)

# IntegrationTest 集成测试
add_executable(IntegrationTest IntegrationTest.cpp)
target_link_libraries(IntegrationTest
    PRIVATE
    gtest
    gtest_main
    pthread
)
add_test(NAME IntegrationTest COMMAND IntegrationTest)

# PerformanceTest 性能测试
add_executable(PerformanceTest PerformanceTest.cpp)
target_link_libraries(PerformanceTest
    PRIVATE
    gtest
    gtest_main
    pthread
)
add_test(NAME PerformanceTest COMMAND PerformanceTest)

# ==================== 自定义测试目标 ====================

# 运行所有单元测试
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^(AuthServiceTest|MessageServiceTest)$" --output-on-failure
    DEPENDS AuthServiceTest MessageServiceTest
    COMMENT "Running unit tests..."
)

# 运行所有集成测试
add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^IntegrationTest$" --output-on-failure
    DEPENDS IntegrationTest
    COMMENT "Running integration tests..."
)

# 运行所有性能测试
add_custom_target(run_performance_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^PerformanceTest$" --output-on-failure
    DEPENDS PerformanceTest
    COMMENT "Running performance tests..."
)

# 运行所有测试
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -V
    DEPENDS AuthServiceTest MessageServiceTest IntegrationTest PerformanceTest
    COMMENT "Running all tests..."
)

# ==================== 测试配置 ====================

# 启用测试
enable_testing()

# 并行运行测试 (最多使用 4 个处理器)
set(CTEST_PARALLEL_LEVEL 4)

# 设置测试标签
set_tests_properties(AuthServiceTest PROPERTIES LABELS "unit")
set_tests_properties(MessageServiceTest PROPERTIES LABELS "unit")
set_tests_properties(IntegrationTest PROPERTIES LABELS "integration")
set_tests_properties(PerformanceTest PROPERTIES LABELS "performance")

# 添加自定义测试目标用于运行所有测试
add_custom_target(
    run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS AuthServiceTest MessageServiceTest
    COMMENT "Running all unit tests..."
)

# ==================== 编译选项 ====================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# ==================== 测试属性 ====================

set_property(TEST AuthServiceTest PROPERTY LABELS "unit")
set_property(TEST MessageServiceTest PROPERTY LABELS "unit")
