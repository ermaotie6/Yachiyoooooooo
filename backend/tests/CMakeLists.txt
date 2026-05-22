cmake_minimum_required(VERSION 3.20)

# ==================== Google Test ====================
find_package(GTest REQUIRED)
include(GoogleTest)

# ==================== 公共依赖 ====================

set(TEST_COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
    ${CMAKE_SOURCE_DIR}/third_party
)

set(TEST_COMMON_LIBS
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    services
    controllers
    utils
    dto
    models
    Threads::Threads
    spdlog::spdlog
)

# ==================== 单元测试可执行文件 ====================

add_executable(yachiyo_unit_tests
    # 测试入口
    main_test.cpp
    
    # 工具类单元测试
    unit/HashUtilTest.cpp
    unit/JwtUtilTest.cpp
    unit/JsonUtilsTest.cpp
    unit/ResultTest.cpp
    
    # DTO 单元测试
    unit/OpenClawDTOTest.cpp
    unit/ModerationDTOTest.cpp
    unit/TTSServiceDTOTest.cpp
    unit/ChatMessageDTOTest.cpp
    
    # 服务层单元测试
    unit/OpenClawGatewayTest.cpp
    unit/ChatServiceTest.cpp
)

target_include_directories(yachiyo_unit_tests PRIVATE ${TEST_COMMON_INCLUDES})
target_link_libraries(yachiyo_unit_tests PRIVATE ${TEST_COMMON_LIBS})
gtest_discover_tests(yachiyo_unit_tests)

# ==================== 集成测试可执行文件 ====================

add_executable(yachiyo_integration_tests
    main_test.cpp
    
    # 集成测试
    integration/AuthFlowTest.cpp
    integration/MessageFlowTest.cpp
    integration/HealthControllerTest.cpp
)

target_include_directories(yachiyo_integration_tests PRIVATE ${TEST_COMMON_INCLUDES})
target_link_libraries(yachiyo_integration_tests PRIVATE ${TEST_COMMON_LIBS})
gtest_discover_tests(yachiyo_integration_tests)

# ==================== 自定义目标: 分别运行 ====================

add_custom_target(run_unit_tests
    COMMAND yachiyo_unit_tests --gtest_color=yes
    DEPENDS yachiyo_unit_tests
    COMMENT "运行单元测试"
)

add_custom_target(run_integration_tests
    COMMAND yachiyo_integration_tests --gtest_color=yes
    DEPENDS yachiyo_integration_tests
    COMMENT "运行集成测试"
)

add_custom_target(run_all_tests
    COMMAND yachiyo_unit_tests --gtest_color=yes
    COMMAND yachiyo_integration_tests --gtest_color=yes
    DEPENDS yachiyo_unit_tests yachiyo_integration_tests
    COMMENT "运行所有测试"
)
