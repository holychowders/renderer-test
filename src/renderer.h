#pragma once

#include "hc_types.h"

//////////////////////////////////////////////////////////////////////////

#ifdef __clang__
    #define dll_export __declspec(dllexport)
#elif __GNUC__
    #define dll_export __attribute__((visibility("default"))
#else
#endif

////////////////////////////////////////////////////////////////////////// SECTION: DATA STRUCTURES

typedef struct Window Window;
typedef struct Image Image;

struct Image {
    U32 width;
    U32 height;
    U32 channels;
    U32 bytes_per_pixel;
    void *pixels;
};

//////////////////////////////////////////////////////////////////////////

B32 init(Str8 window_title, U32 window_width, U32 window_height);
B32 poll_events(void);

Image image_load(const char *path);
void image_free(Image *image);
