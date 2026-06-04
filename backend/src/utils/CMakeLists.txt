add_library(utils
    ConfigParser.cpp
    DatabaseUtil.cpp
    HashUtil.cpp
    HttpServer.cpp
    JwtUtil.cpp
    Logger.cpp
    LogUtils.cpp
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

# crypt (for bcrypt in HashUtil)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CRYPT QUIET libcrypt)
if(NOT CRYPT_FOUND)
    set(CRYPT_LIBRARIES crypt)
endif()

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
        ${CRYPT_LIBRARIES}
)
