add_library(models
    # 模型类通常只有头文件或最小的实现
)

target_include_directories(models
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)
