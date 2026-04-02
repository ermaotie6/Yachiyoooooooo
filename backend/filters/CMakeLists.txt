# Filters CMakeLists.txt
add_library(filters
    src/filters/AuthFilter.cpp
    src/filters/LogFilter.cpp
    src/filters/RateLimitFilter.cpp
    src/filters/CORSFilter.cpp
    src/filters/CompressionFilter.cpp
)

target_include_directories(filters
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(filters
    PUBLIC
        utils
)