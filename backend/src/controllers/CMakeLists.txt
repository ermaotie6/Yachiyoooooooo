add_library(controllers
    AuthController.cpp
    BaseController.cpp
    HealthController.cpp
    MessageController.cpp
    WebSocketController.cpp
    routes.cpp
)

target_include_directories(controllers
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Crow REQUIRED)
find_package(spdlog REQUIRED)

# uuid (from util-linux-libs, no cmake config, use pkg-config)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UUID REQUIRED uuid)

target_link_libraries(controllers
    PUBLIC
        services
        utils
    PRIVATE
        Crow::Crow
        spdlog::spdlog
        ${UUID_LIBRARIES}
)
target_include_directories(controllers PRIVATE ${UUID_INCLUDE_DIRS})
