#pragma once

#include "platform.h"

#include "hc_types.h"

bool r_gl_platform_context_create(P_PlatformWindow *window);
void r_gl_platform_context_destroy(void);
void r_gl_platform_present(void);
