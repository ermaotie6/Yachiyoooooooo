cmake_minimum_required(VERSION 3.20)
project(YachiyoCPP VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
add_compile_options(-Wall -Wextra -Wpedantic -O2)

# 查找依赖
find_package(Threads REQUIRED)
find_package(GTest QUIET)
find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json 3.2.0 QUIET)

# 如果 nlohmann_json 未安装，使用 header-only 版本
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, using header-only version")
endif()

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${PostgreSQL_INCLUDE_DIRS}
)

# 子目录 - 按照依赖顺序添加
add_subdirectory(src/utils)
add_subdirectory(src/dto)
add_subdirectory(src/models)
add_subdirectory(src/filters)
add_subdirectory(src/mappers)
add_subdirectory(src/config)
add_subdirectory(src/services)
add_subdirectory(src/controllers)

# 测试支持
if(GTest_FOUND)
    enable_testing()
    add_subdirectory(tests)
else()
    message(STATUS "Google Test not found. Skipping tests.")
endif()

# 主可执行文件
add_executable(yachiyo_cpp
    src/main.cpp
    src/Application.cpp
)

# 链接库
target_link_libraries(yachiyo_cpp
    PRIVATE
    controllers
    services
    models
    dto
    utils
    filters
    mappers
    config
    Threads::Threads
    PostgreSQL::PostgreSQL
)

# 如果 nlohmann_json 安装了，链接它
if(nlohmann_json_FOUND)
    target_link_libraries(yachiyo_cpp PRIVATE nlohmann_json::nlohmann_json)
endif()

# 安装目标
install(TARGETS yachiyo_cpp DESTINATION bin)