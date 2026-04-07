add_library(utils
    ConfigParser.cpp
    DatabaseUtil.cpp
    EmailService.cpp
    HashUtil.cpp
    HttpServer.cpp
    JsonUtils.cpp
    JwtUtil.cpp
    LogUtils.cpp
    OpencalwClient.cpp
    RedisUtil.cpp
)

target_include_directories(utils
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 查找依赖库
find_package(hiredis REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Crow REQUIRED)
find_package(CURL REQUIRED)

target_link_libraries(utils
    PUBLIC
        hiredis::hiredis
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
        spdlog::spdlog
        yaml-cpp::yaml-cpp
        Crow::Crow
        CURL::libcurl
)
