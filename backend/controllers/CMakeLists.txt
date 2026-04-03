# Controllers CMakeLists.txt
add_library(controllers
    src/controllers/AuthController.cpp
    src/controllers/AIController.cpp
    src/controllers/UserController.cpp
    src/controllers/PostController.cpp
    src/controllers/HealthController.cpp
    src/controllers/MessageController.cpp
)

target_include_directories(controllers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(controllers
    PUBLIC
        services
        dto
        utils
    PRIVATE
        Threads::Threads
)