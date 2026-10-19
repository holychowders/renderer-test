cmake_minimum_required(VERSION 3.31)
project(renderer LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# BASE APPLICATION CONFIGURATION #
add_executable(renderer src/demo2.c src/renderer_gl.c)
target_include_directories(renderer PRIVATE src/include)
target_compile_definitions(renderer PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(renderer PRIVATE
    -fvisibility=hidden
    -Weverything -pedantic-errors -Werror=vla
    -fno-builtin
    -Wno-unsafe-buffer-usage -Wno-reserved-identifier # disabled for style preferences
    -Wno-declaration-after-statement # disabled for newer standards 
    -Wno-padded -Wno-unused-macros -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable # disable sometimes for QA
)

# PLATFORM-SPECIFIC APP CONFIGURATION #
if (WIN32)
    set_target_properties(renderer PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(renderer PRIVATE src/platform_win32.c src/renderer_gl_win32.c)
    target_link_libraries(renderer PRIVATE)
    target_compile_definitions(renderer PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
elseif(LINUX)
    find_package(X11 REQUIRED)
    target_sources(renderer PRIVATE src/platform_x11.c)
    target_link_libraries(renderer PRIVATE X11::X11 m)
    message(FATAL_ERROR "TODO: Not implemented")
else()
    message(FATAL_ERROR "Platform not supported: ${CMAKE_SYSTEM_NAME}")
endif(WIN32)

# STB DEPENDENCY #
target_include_directories(renderer SYSTEM PRIVATE vendor/stb-src-master-2026-05-04)

# GLEW DEPENDENCY #
find_package(OpenGL REQUIRED)
target_link_libraries(renderer PRIVATE glew OpenGL::GL)
target_compile_definitions(renderer PRIVATE GLEW_STATIC)
target_include_directories(renderer SYSTEM PRIVATE vendor/glew-2.3.1-src-release/include)

add_library(glew STATIC vendor/glew-2.3.1-src-release/src/glew.c)
target_compile_definitions(glew PRIVATE GLEW_STATIC)
target_include_directories(glew PRIVATE vendor/glew-2.3.1-src-release/include)

# IMGUI DEPENDENCY #
#add_library(imgui STATIC vendor/imgui-1.92.7-src-release/imgui.cpp)
#target_include_directories(renderer SYSTEM PRIVATE
#    vendor/imgui-1.92.7-src-release
#    vendor/imgui-1.92.7-src-release/backends
#)

# Keep compile_commands.json in project root in sync.
# This is useful for tools like clangd, clang-tidy, etc.
# ------------------------------------------------------
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(sync_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COMMENT "Syncing compile_commands.json to project root"
        VERBATIM
    )
endif()

