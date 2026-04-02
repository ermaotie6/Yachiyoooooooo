# Services CMakeLists.txt
add_library(services
    src/services/AuthServiceImpl.cpp
    src/services/AIServiceImpl.cpp
    src/services/UserServiceImpl.cpp
    src/services/PostServiceImpl.cpp
    src/services/ChatServiceImpl.cpp
    src/services/TTSServiceImpl.cpp
    src/services/Live2DServiceImpl.cpp
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
)