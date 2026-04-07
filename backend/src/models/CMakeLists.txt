add_library(models INTERFACE)

target_include_directories(models
    INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)
