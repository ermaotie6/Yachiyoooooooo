# DTO CMakeLists.txt
add_library(dto
    src/dto/AuthDTO.cpp
    src/dto/UserDTO.cpp
    src/dto/AIDTO.cpp
    src/dto/PostDTO.cpp
    src/dto/CommonDTO.cpp
)

target_include_directories(dto
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)