add_library(dto
    AuthDTO.cpp
    CommonDTO.cpp
)

target_include_directories(dto
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dto
    PUBLIC
        utils
)
