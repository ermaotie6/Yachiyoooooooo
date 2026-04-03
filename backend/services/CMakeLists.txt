# Services CMakeLists.txt
add_library(services
    src/services/AuthServiceImpl.cpp
    src/services/AIServiceImpl.cpp
    src/services/UserServiceImpl.cpp
    src/services/PostServiceImpl.cpp
    src/services/ChatServiceImpl.cpp
    src/services/TTSServiceImpl.cpp
    src/services/Live2DServiceImpl.cpp
    src/services/OpenClawGateway.cpp
    src/services/TranslationService.cpp
    src/services/GPTSoVITSService.cpp
    src/services/Live2DAnimationService.cpp
    src/services/AvatarResponseService.cpp
    src/services/DeepSeekModerationService.cpp
    src/services/DatabaseService.cpp
    src/services/WebSocketService.cpp
)

target_include_directories(services
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(services
    PUBLIC
        models
        mappers
        utils
        Threads::Threads
    PRIVATE
        PostgreSQL::PostgreSQL
)