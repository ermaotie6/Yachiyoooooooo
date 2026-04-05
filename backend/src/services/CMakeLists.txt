add_library(services
    AIServiceImpl.cpp
    AuthService.cpp
    AuthServiceImpl.cpp
    AvatarResponseService.cpp
    ChatService.cpp
    DatabaseService.cpp
    DeepSeekModerationService.cpp
    GPTSoVITSService.cpp
    Live2DAnimationService.cpp
    MessageServiceImpl.cpp
    OpenClawGateway.cpp
    TranslationService.cpp
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
find_package(PostgreSQL REQUIRED)

target_link_libraries(services
    PUBLIC
        utils
        dto
        models
    PRIVATE
        Crow::Crow
        PostgreSQL::PostgreSQL
)
