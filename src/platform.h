#pragma once

#include "hc_types.h"
#include "hc_keys.h"

typedef struct PlatformData PlatformData;

B32 platform_init(Str8 window_title, U32 window_width, U32 window_height);
B32 platform_shutdown(PlatformData *platform_data);
B32 platform_poll_events(void);
