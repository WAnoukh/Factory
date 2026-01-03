include_directories(${INLUDES_PATH})

add_executable(exe
    src/reloading.c
    src/main.c

    ./src/window/window_input.c
    ./src/window/window.c
)

target_compile_definitions(exe PRIVATE
    GLFW_DLL
)

target_link_libraries(exe
    ${LIBS_PATH}/MinGW/libglfw3dll.a
    glad
    opengl32
    gdi32
    user32
    kernel32

    core
)

target_include_directories(exe
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../shared/
)

set_target_properties(exe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
