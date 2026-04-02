add_library(controllers
    AIController.cpp
    AuthController.cpp
    BaseController.cpp
    HealthController.cpp
    PostController.cpp
    routes.cpp
    UserController.cpp
)

target_include_directories(controllers
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(controllers
    PUBLIC
        services
        utils
)
