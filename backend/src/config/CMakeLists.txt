add_library(config INTERFACE)

target_include_directories(config
    INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)

# config 是 header-only, 使用 INTERFACE 链接
# target_link_libraries(config INTERFACE utils)
