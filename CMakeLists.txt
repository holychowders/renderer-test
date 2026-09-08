cmake_minimum_required(VERSION 3.20)
project(renderer-test LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# EXECUTABLES
add_executable(renderer-test WIN32 src/renderer_win32_gl.c src/gl.c)

# SOURCE LIBRARIES
add_library(glew STATIC vendor/glew-2.3.1-src-release/src/glew.c)
add_library(imgui STATIC vendor/imgui-1.92.7-src-release/imgui.cpp)

# LINK LIBRARIES
target_link_libraries(renderer-test PRIVATE glew opengl32)

# COMPILE OPTIONS
target_compile_options(renderer-test PRIVATE
    -g -O0 -fvisibility=hidden # output
    -pedantic # strict standard conformance
    -Wall -Wextra -Weverything # enable most diagnostics
    -Wvla -Werror=vla # disallow VLAs
    -Wno-padded # performance/Layout
    #-Wno-gnu-anonymous-struct -Wno-nested-anon-types # extensions
    -Wno-declaration-after-statement # C89 compatibility
    -Wno-missing-braces -Wno-covered-switch-default -Wno-reserved-identifier # misc
    -Wno-unused-parameter -Wno-unsafe-buffer-usage -Wno-alloca -Wno-unused-macros # misc
)

# COMPILE DEFINITIONS
target_compile_definitions(glew PRIVATE GLEW_STATIC)
target_compile_definitions(renderer-test PRIVATE GLEW_STATIC _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

# INCLUDE DIRECTORIES
target_include_directories(renderer-test PRIVATE src/include/)
target_include_directories(renderer-test SYSTEM PRIVATE
    vendor/stb-src-master-2026-05-04/
    vendor/glew-2.3.1-src-release/include/
    vendor/glm-1.0.3-src-release/
    vendor/imgui-1.92.7-src-release/
    vendor/imgui-1.92.7-src-release/backends/
)
target_include_directories(glew PRIVATE vendor/glew-2.3.1-src-release/include/)
