cmake_minimum_required(VERSION 3.31)
project(Project C CXX)

set(CMAKE_USE_RELATIVE_PATHS ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall")

if(EXISTS "${CMAKE_SOURCE_DIR}/CMakeLocal.cmake")
    include("${CMAKE_SOURCE_DIR}/CMakeLocal.cmake")
endif()

add_subdirectory(core/)
add_subdirectory(host/exe/)
add_subdirectory(host/hot/)

add_subdirectory(engine/)
add_subdirectory(game/)

############
### Glad ###
############

find_package(OpenGL REQUIRED)

add_library(glad SHARED)

target_sources(glad
    PUBLIC
    ${INCLUDES_PATH}/glad/glad.h
    PRIVATE
    ${INCLUDES_PATH}/glad/glad.c
)

target_compile_definitions(glad PRIVATE 
    GLAD_GLAPI_EXPORT=1 
    GLAD_GLAPI_EXPORT_BUILD=1
)

target_include_directories(glad PUBLIC 
    ${INCLUDES_PATH}
)

target_link_libraries(glad PRIVATE 
    opengl32
)

set_target_properties(glad 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

#############
### ImGui ###
#############
include_directories(${INCLUDES_PATH})

add_library(cimgui STATIC
    ${CIMGUI_PATH}/cimgui.cpp
    ${CIMGUI_PATH}/cimgui_impl.cpp
    ${CIMGUI_PATH}/imgui/imgui.cpp
    ${CIMGUI_PATH}/imgui/imgui_draw.cpp
    ${CIMGUI_PATH}/imgui/imgui_tables.cpp
    ${CIMGUI_PATH}/imgui/imgui_widgets.cpp
    # optional:
    ${CIMGUI_PATH}/imgui/imgui_demo.cpp
)

set_target_properties(cimgui PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(cimgui 
    PUBLIC
    ${CIMGUI_PATH}
    ${CIMGUI_PATH}/imgui
)

target_compile_definitions(cimgui PUBLIC 
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    GLFW_DLL
    "-DIMGUI_IMPL_API=extern \"C\" __declspec(dllexport)"
)

target_link_libraries(cimgui PRIVATE
    ${LIBS_PATH}/MinGW/libglfw3dll.a
    opengl32
    gdi32
    user32
    kernel32
)
