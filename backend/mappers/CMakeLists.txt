# Mappers CMakeLists.txt
add_library(mappers
    src/mappers/UserMapper.cpp
    src/mappers/PostMapper.cpp
    src/mappers/ChatMapper.cpp
    src/mappers/LinkMapper.cpp
)

target_include_directories(mappers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 查找MySQL连接器
find_package(MySQL REQUIRED)

target_link_libraries(mappers
    PUBLIC
        models
        utils
        MySQL::MySQL
)