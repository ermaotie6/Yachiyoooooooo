# Handlers 模块 CMakeLists.txt
# WebSocketMessageHandler — WebSocket 消息处理逻辑

add_library(handlers STATIC
    WebSocketMessageHandler.cpp
)

target_include_directories(handlers PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(handlers
    PUBLIC
    core
    controllers
    services
    models
    dto
    utils
    config
    spdlog::spdlog
)
