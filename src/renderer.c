#include "renderer.h"

#include "platform.h"
#include "backend.h"

#include "hc_log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

////////////////////////////////////////////////////////////////////////// SECTION: MACROS

#define RENDERER_INFO(msg) info_re("Renderer", (msg))
#define RENDERER_WARN(msg) warn_re("Renderer", (msg))
#define RENDERER_ERROR(msg) error_re("Renderer", (msg))
#define RENDERER_ERROR_DETAILED(msg) ERROR_RE_DETAILED("Renderer", (msg))

////////////////////////////////////////////////////////////////////////// SECTION: DATA STRUCTURES

////////////////////////////////////////////////////////////////////////// SECTION: PLATFORM UTILITIES

B32 init(Str8 window_title, U32 window_width, U32 window_height) {
    if (!platform_init(window_title, window_width, window_height)) {
        RENDERER_ERROR("Failed to initialize platform");
        return false;
    }

    if (!backend_init()) {
        RENDERER_ERROR("Failed to initialize platform");
        return false;
    }

    backend_clear_background(1, 0, 1, 1);
    // platform_swap_buffers

    RENDERER_INFO("Initialized");

    return true;
}

B32 poll_events(void) {
    return platform_poll_events();
}

////////////////////////////////////////////////////////////////////////// SECTION: IMAGE

Image image_load(const char *path) {
    Image result = { 0 };

    S32 width = { 0 }, height = { 0 }, channels = { 0 };
    stbi_uc *image = stbi_load(path, &width, &height, &channels, 4);

    result.width = (U32)width;
    result.height = (U32)height;
    result.channels = (U32)channels;
    //result.bytes_per_pixel = 8; TODO
    result.pixels = image;

    //stbi_image_free(image);

    return result;
}
void image_free(Image *image) {
    stbi_image_free(image->pixels);
    *image = (Image){ 0 };
}
