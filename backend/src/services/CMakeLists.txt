add_library(services
    AuthService.cpp
    AuthServiceImpl.cpp
    ChatService.cpp
    PostService.cpp
    UserService.cpp
)

target_include_directories(services
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(services
    PUBLIC
        utils
        dto
        models
)
