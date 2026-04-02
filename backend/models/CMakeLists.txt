# Models CMakeLists.txt
add_library(models
    src/models/User.cpp
    src/models/Post.cpp
    src/models/ChatSession.cpp
    src/models/ChatMessage.cpp
    src/models/Link.cpp
    src/models/Like.cpp
    src/models/Bookmark.cpp
)

target_include_directories(models
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)