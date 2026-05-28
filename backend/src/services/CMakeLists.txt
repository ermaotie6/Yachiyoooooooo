add_library(services
    AuthService.cpp
    AuthServiceImpl.cpp
    AvatarResponseService.cpp
    ChatService.cpp
    DatabaseService.cpp
    GPTSoVITSService.cpp
    Live2DAnimationService.cpp
    MessageServiceImpl.cpp
    OpenClawGateway.cpp
    UserService.cpp
    WebSocketService.cpp
)

target_include_directories(services
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Crow REQUIRED)
find_package(spdlog REQUIRED)
find_package(CURL REQUIRED)

# pqxx (libpqxx) - PostgreSQL C++ library
find_package(libpqxx QUIET)

target_link_libraries(services
    PUBLIC
        utils
        dto
        models
    PRIVATE
        Crow::Crow
        spdlog::spdlog
        CURL::libcurl
)

# Link pqxx if found
if(libpqxx_FOUND)
    target_link_libraries(services PRIVATE pqxx)
else()
    # Try pkg-config as fallback
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PQXX QUIET libpqxx)
        if(PQXX_FOUND)
            target_include_directories(services PRIVATE ${PQXX_INCLUDE_DIRS})
            target_link_libraries(services PRIVATE ${PQXX_LIBRARIES})
        endif()
    endif()
endif()
