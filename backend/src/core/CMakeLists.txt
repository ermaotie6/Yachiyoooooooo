# Core 模块 CMakeLists.txt
# ServiceRegistry — 管线服务的创建和组装

add_library(core STATIC
    ServiceRegistry.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(core
    PUBLIC
    controllers
    services
    models
    dto
    utils
    config
    spdlog::spdlog
)
