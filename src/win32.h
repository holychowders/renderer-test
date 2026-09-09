#pragma once

#include <Windows.h>

#include "gl.h"

#include "hc_types.h"

typedef struct Win32GL_InitInfo {
    B32 success;
    HWND window_handle;
    HDC window_dc;
    HGLRC glrc_handle;
} Win32GL_InitInfo;

Win32GL_InitInfo win32gl_init(HINSTANCE hInstance, U32 window_width, U32 window_height);
void win32gl_shutdown(Win32GL_InitInfo init_info);
GLuint win32gl_prg_create(void);
