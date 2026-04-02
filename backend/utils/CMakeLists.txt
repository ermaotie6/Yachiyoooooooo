# Utils CMakeLists.txt
add_library(utils
    src/utils/HttpServer.cpp
    src/utils/CrowHttpServer.cpp
    src/utils/JsonUtils.cpp
    src/utils/ConfigUtils.cpp
    src/utils/JWTUtils.cpp
    src/utils/PasswordUtils.cpp
    src/utils/LogUtils.cpp
    src/utils/TimeUtils.cpp
    src/utils/StringUtils.cpp
    src/utils/FileUtils.cpp
    src/utils/NetworkUtils.cpp
    src/utils/AIUtils.cpp
    src/utils/TTSUtils.cpp
)

target_include_directories(utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 查找依赖库
find_package(Crow REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(utils
    PUBLIC
        Crow::Crow
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)