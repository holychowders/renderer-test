#pragma once

#include "hc_types.h"

typedef struct P_PlatformWindow P_PlatformWindow;

int app_main(void);

B32 p_platform_init(void);
void p_platform_shutdown(void);

P_PlatformWindow *p_platform_window_create(U32 width, U32 height);
void p_platform_window_destroy(P_PlatformWindow *window);

B32 p_platform_process_events(S32 *exit_code);

// TODO: Make this a generic file reading function
char *p_platform_load_shader_source(const char *path);
