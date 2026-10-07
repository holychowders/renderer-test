cmake_minimum_required(VERSION 3.31)
project(renderer LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# EXECUTABLES
add_executable(renderer WIN32 src/demo2.c src/renderer_gl.c src/platform_win32.c src/renderer_gl_win32.c)

# SOURCE LIBRARIES
add_library(glew STATIC vendor/glew-2.3.1-src-release/src/glew.c)
add_library(imgui STATIC vendor/imgui-1.92.7-src-release/imgui.cpp)

# LINK LIBRARIES
target_link_libraries(renderer PRIVATE glew opengl32)

# COMPILE OPTIONS
target_compile_options(renderer PRIVATE
    -fvisibility=hidden

    -Weverything          # enable all diagnostics (includes those from -Wall, -Wextra, -Wpedantic, -Wvla, + all others)
    -pedantic-errors      # treat non-standard language extensions as errors
    -Werror=vla           # treat VLA use as error
    -Wno-declaration-after-statement # do not warn about C89 incompatibility
    #-fno-builtin # consider in the future

    -Wno-padded # performance/Layout
    -Wno-missing-braces -Wno-covered-switch-default -Wno-reserved-identifier # misc
    -Wno-unused-parameter -Wno-unsafe-buffer-usage -Wno-alloca -Wno-unused-macros # misc
)

# COMPILE DEFINITIONS
target_compile_definitions(glew PRIVATE GLEW_STATIC)
target_compile_definitions(renderer PRIVATE GLEW_STATIC _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

# INCLUDE DIRECTORIES
target_include_directories(renderer PRIVATE src/include/)
target_include_directories(renderer SYSTEM PRIVATE
    vendor/stb-src-master-2026-05-04/
    vendor/glew-2.3.1-src-release/include/
    vendor/imgui-1.92.7-src-release/
    vendor/imgui-1.92.7-src-release/backends/
)
target_include_directories(glew PRIVATE vendor/glew-2.3.1-src-release/include/)
