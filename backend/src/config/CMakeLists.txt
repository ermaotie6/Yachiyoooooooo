add_library(config STATIC
    ConfigManager.cpp
)

target_include_directories(config
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# 链接依赖: yaml-cpp, nlohmann_json
target_link_libraries(config
    PUBLIC
        yaml-cpp::yaml-cpp
)
