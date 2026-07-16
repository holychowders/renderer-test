cmake_minimum_required(VERSION 3.20)
project(renderer-test LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(RENDERER "OpenGL" CACHE STRING "Graphics backend")
set_property(CACHE RENDERER PROPERTY STRINGS OpenGL DirectX12)

# Test Application #
add_executable(renderer-test src/main.c)
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

# Platform-Renderer Dependencies
if (WIN32)
    if (RENDERER STREQUAL OPENGL)
        target_sources(renderer-test PRIVATE src/backends/gl.c)
        target_link_libraries(renderer-test PRIVATE opengl32)
    elseif(RENDERER STREQUAL DIRECTX12)
    endif()
elseif(UNIX)
    if (RENDERER STREQUAL OPENGL)
    endif()
endif()

# Platform-Only Dependencies #
if(WIN32)
    target_sources(renderer-test PRIVATE src/platforms/win32.c)
    target_compile_definitions(renderer-test PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
elseif(UNIX)
    target_sources(renderer-test PRIVATE src/platforms/x11.c)
endif()

# Renderer-Only Dependencies #
if(RENDERER STREQUAL OPENGL)
    target_compile_definitions(renderer-test PRIVATE GLEW_STATIC)
    target_link_libraries(renderer-test PRIVATE glew)
    target_include_directories(renderer-test SYSTEM PRIVATE
        vendor/glew-2.3.1-src-release/include/
        vendor/glm-1.0.3-src-release/
    )
    add_library(glew STATIC vendor/glew-2.3.1-src-release/src/glew.c)
    target_compile_definitions(glew PRIVATE GLEW_STATIC)
    target_include_directories(glew PRIVATE vendor/glew-2.3.1-src-release/include/)
elseif(renderer-test STREQUAL DIRECTX12)
    # TODO
endif()

# STB #
target_include_directories(renderer-test SYSTEM PRIVATE
    vendor/stb-src-master-2026-05-04/
)

# IMGui #
add_library(imgui STATIC vendor/imgui-1.92.7-src-release/imgui.cpp)
target_include_directories(renderer-test SYSTEM PRIVATE
    vendor/imgui-1.92.7-src-release/
    vendor/imgui-1.92.7-src-release/backends/
)
