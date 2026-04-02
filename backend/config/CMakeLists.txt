# Config CMakeLists.txt
add_library(config
    src/config/ConfigManager.cpp
    src/config/ConfigParser.cpp
)

target_include_directories(config
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 查找YAML库
find_package(yaml-cpp REQUIRED)

target_link_libraries(config
    PUBLIC
        utils
        yaml-cpp
)