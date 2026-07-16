#include "renderer.h"

#include "hc_log.h"
#include "hc_types.h"
#include "hc_assert.h"

int main(void) {
    ASSERT(init(str8("Renderer Test"), 1920, 1080));
    //Window window = window_open("Renderer Test");

    Image image = image_load("assets/images/Faces For A Dying Land/creep32.png");

    //for (;;) {
    //image_render(image);
    //}

    image_free(&image);
}
