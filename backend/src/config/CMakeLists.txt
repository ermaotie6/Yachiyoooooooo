add_library(config
    # 配置管理实现
)

target_include_directories(config
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(config
    PUBLIC
        utils
)
