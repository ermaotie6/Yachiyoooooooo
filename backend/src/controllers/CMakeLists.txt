add_library(controllers
    AuthController.cpp
    BaseController.cpp
    HealthController.cpp
    MessageController.cpp
    WebSocketController.cpp
    routes.cpp
    UserController.cpp
)

target_include_directories(controllers
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Crow REQUIRED)
find_package(spdlog REQUIRED)

target_link_libraries(controllers
    PUBLIC
        services
        utils
    PRIVATE
        Crow::Crow
        spdlog::spdlog
        uuid
)
