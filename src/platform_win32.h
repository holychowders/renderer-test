#pragma once

#include <Windows.h>
#include "platform.h"

// TODO: Replace error logging with this where applicable
#define WIN32_DEF_LAST_ERROR_MSG(name)                                                                                                               \
    char(name)[512] = { 0 };                                                                                                                         \
    p_win32_get_last_error((name), sizeof((name)))

HWND p_win32_window_get_handle(P_PlatformWindow *window);

void p_win32_print_last_error(const char *re);
void p_win32_get_last_error(char *out_msg, size_t out_len);
