#include "backend.h"

#include <GL/glew.h>

#include "hc_log.h"
#include "hc_assert.h"

////////////////////////////////////////////////////////////////////////// SECTION(PRIVATE): MACROS

#define GL_INFO(msg) info_re("Renderer/GL", (msg))
#define GL_WARN(msg) warn_re("Renderer/GL", (msg))
#define GL_ERROR(msg) error_re("Renderer/GL", (msg))
#define GL_ERROR_DETAILED(msg) ERROR_RE_DETAILED("Renderer/GL", (msg))

#define gl(gl_operation)                                                                                                                             \
    clear_gl_errors();                                                                                                                               \
    gl_operation;                                                                                                                                    \
    ASSERT(!check_gl_errors() && #gl_operation)

////////////////////////////////////////////////////////////////////////// SECTION(PRIVATE): DEFINITIONS

static inline B32 glew_init(void) {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        error_re("Renderer/GL/GLEW", (const char *)glewGetErrorString(err));
        return false;
    }
    finfo_re("Renderer/GL/GLEW", "Version %s", (const char *)glewGetString(GLEW_VERSION));
    return true;
}

static inline void clear_gl_errors(void) {
    while (glGetError() != GL_NO_ERROR) {}
}

static inline B32 check_gl_errors(void) {
    B32 has_error = false;
    GLenum gl_error = { 0 };
    while ((gl_error = glGetError()) != GL_NO_ERROR) {
        has_error = true;
        const char *msg = "";
        switch (gl_error) {
            case GL_INVALID_ENUM: msg = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: msg = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: msg = "GL_INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: msg = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY: msg = "GL_OUT_OF_MEMORY"; break;
            case GL_STACK_UNDERFLOW: msg = "GL_STACK_UNDERFLOW"; break;
            case GL_STACK_OVERFLOW: msg = "GL_STACK_OVERFLOW"; break;
            default: {
                char fmsg[128];
                snprintf(fmsg, sizeof(fmsg), "Unknown error: 0x%x", gl_error);
                msg = fmsg;
            } break;
        }
        GL_ERROR(msg);
    }
    return has_error;
}

// Returns true/false on success/fail
B32 backend_init(void) {
    //if (!window) { /*return -1;*/ }

    if (!glew_init()) { return false; }

    gl(glEnable(GL_DEPTH_TEST));
    gl(glEnable(GL_BLEND));
    gl(glDepthFunc(GL_LESS));
    gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    GL_INFO("Initialized");

    return true;
}

void backend_clear_background(F32 r, F32 g, F32 b, F32 a) {
    //gl(glViewport(0, 0, window_width, window_height)); // set the viewport
    gl(glClearColor(r, g, b, a));
    gl(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}
