cmake_minimum_required(VERSION 3.20)
project(RaytracingDistanceField LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)

# GLFW
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)

# glad
set(GLAD_PROFILE      "compatibility"           CACHE STRING "" FORCE)
set(GLAD_API          "gl=4.6"                  CACHE STRING "" FORCE)
set(GLAD_EXTENSIONS   "GL_ARB_bindless_texture" CACHE STRING "" FORCE)
set(GLAD_REPRODUCIBLE ON                        CACHE BOOL   "" FORCE)
FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
    GIT_SHALLOW    TRUE)

# tinyobjloader
FetchContent_Declare(tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        v1.0.6
    GIT_SHALLOW    TRUE)

# Dear ImGui
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.7
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(glfw glad tinyobjloader imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw ${CMAKE_DL_LIBS})


target_compile_definitions(imgui PRIVATE GLFW_INCLUDE_NONE)

# app
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(rtdf ${APP_SOURCES})
target_link_libraries(rtdf PRIVATE glfw glad tinyobjloader imgui)
target_compile_definitions(rtdf PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/"
    ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
    OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/")

target_include_directories(rtdf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)