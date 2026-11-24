#include "renderer_gl_platform.h"
#include "platform_win32.h"
#include <windows.h>
#include <wingdi.h>
#include <GL/glew.h>
#include <GL/wglew.h>

#include "hc_log.h"
#include "hc_assert.h"

#define R_Win32GL_ERROR(msg) ERROR_RE_DETAILED("Renderer: Win32/GL", (msg))

typedef struct {
    HWND window_handle; // borrowed from platform
    HDC window_dc; // owned
    HGLRC glrc_handle; // owned
} R_Win32GL_State;

static R_Win32GL_State g_win32gl_info;

//// WGL function type definitions
//typedef HGLRC FType_wglCreateContextAttribsARB(HDC hDC, HGLRC hshareContext, const int *attribList);
//typedef BOOL WINAPI FType_wglSwapIntervalEXT(int interval);
//typedef int WINAPI FType_wglGetSwapIntervalEXT(void);
//
//// WGL function pointer definitions
//static FType_wglCreateContextAttribsARB *wglCreateContextAttribsARB;
//static FType_wglSwapIntervalEXT *wglSwapIntervalEXT;

static inline void gl_print_version_info(void) {
    finfo_re("GL", "GLEW version   : %s", (const char *)glewGetString(GLEW_VERSION));
    finfo_re("GL", "OpenGL version : %s", glGetString(GL_VERSION));
    finfo_re("GL", "GLSL version   : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    finfo_re("GL", "OpenGL vendor  : %s", glGetString(GL_VENDOR));
    finfo_re("GL", "OpenGL renderer: %s", glGetString(GL_RENDERER));
}

static inline bool glew_init(void) {
    glewExperimental = GL_TRUE;

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        ferror_re("GL", "Failed to initialize GLEW: %s", (const char *)glewGetErrorString(err));
        return false;
    }

    return true;
}

// TODO: Keep WGL-equivalent pixel format functions in mind for more control in the future (eg, wglChoosePixelFormatARB)
// TODO: Allow the renderer to provide a different pixel format
static inline bool win32gl_set_pixel_format(HDC window_dc) {
    // Specify Desired Pixel Format
    // ----------------------------
    PIXELFORMATDESCRIPTOR requested = { 0 };
    requested.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    requested.nVersion = 1;
    requested.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

    // Color
    requested.iPixelType = PFD_TYPE_RGBA;
    requested.cColorBits = 24; // TODO: The docs say "excluding the alpha bitplanes", but then it gives 32 bits anyway?
    requested.cAlphaBits = 8;

    // Color: RGB fields are ignored by Windows, but used by us for validation later
    requested.cRedBits = 8;
    requested.cGreenBits = 8;
    requested.cBlueBits = 8;

    // Depth
    requested.cDepthBits = 24;
    requested.cStencilBits = 8;

    // Receive Pixel Format
    // --------------------
    int received_idx = ChoosePixelFormat(window_dc, &requested); // index of supported format that best matches request
    if (!received_idx) {
        WIN32_DEF_LAST_ERROR_MSG(err_msg);
        ferror_re("Renderer: Win32/GL", "Failed to obtain valid pixel format index for window: %s", err_msg);
        return false;
    }
    PIXELFORMATDESCRIPTOR received = { 0 };
    if (!DescribePixelFormat(window_dc, received_idx, sizeof(PIXELFORMATDESCRIPTOR), &received)) {
        WIN32_DEF_LAST_ERROR_MSG(err_msg);
        ferror_re("Renderer: Win32/GL", "Failed to obtain valid window pixel format: %s", err_msg);
        return false;
    }

    // Validate Received Pixel Format Details
    // --------------------------------------
    // TODO: More robust validation, inluding removing asserts and enumerating available pixel formats

    // Color format
    ASSERT(received.iPixelType == requested.iPixelType);
    ASSERT(received.cRedBits == requested.cRedBits);
    ASSERT(received.cGreenBits == requested.cGreenBits);
    ASSERT(received.cBlueBits == requested.cBlueBits);
    ASSERT(received.cAlphaBits == requested.cAlphaBits);

    // Depth
    ASSERT(received.cDepthBits >= requested.cDepthBits);
    ASSERT(received.cStencilBits == requested.cStencilBits);

    // Flags
    ASSERT((received.dwFlags & requested.dwFlags) == requested.dwFlags);

    // Set Pixel Format
    // ----------------
    if (!SetPixelFormat(window_dc, received_idx, &received)) {
        WIN32_DEF_LAST_ERROR_MSG(err_msg);
        ferror_re("Renderer: Win32/GL", "Failed set window pixel format: %s", err_msg);
        return false;
    }

    //
    return true;
}

// destroy_wgl_context
static inline void win32gl_context_destroy(R_Win32GL_State state) {
    wglMakeCurrent(NULL, NULL);
    if (state.glrc_handle) {
        if (wglDeleteContext(state.glrc_handle)) { state.glrc_handle = NULL; }
        else { warn_re("Win32/GL", "Failed to delete the OpenGL rendering context"); }
    }
    if (state.window_handle && state.window_dc) {
        if (ReleaseDC(state.window_handle, state.window_dc)) {
            state.window_handle = NULL;
            state.window_dc = NULL;
        }
        else { warn_re("Win32", "Failed to release the window device context"); }
    }
}

bool r_gl_platform_context_create(P_PlatformWindow *window) {
    HWND window_handle = { 0 };
    HDC window_dc = { 0 };
    HGLRC fake_glrc_handle = { 0 };
    HGLRC modern_glrc_handle = { 0 };

    // Get Window Handle
    // -----------------
    window_handle = p_win32_window_get_handle(window);
    if (!window_handle) {
        R_Win32GL_ERROR("Failed to get valid window handle");
        goto fail;
    }

    // Get Window Device Context
    // -------------------------
    window_dc = GetDC(window_handle);
    if (!window_dc) {
        R_Win32GL_ERROR("Failed to get window device context");
        goto fail;
    }

    // Set Pixel Format
    // ----------------
    if (!win32gl_set_pixel_format(window_dc)) {
        R_Win32GL_ERROR("Failed to set window pixel format");
        goto fail;
    }

    // Create OpenGL Context
    // ---------------------
    fake_glrc_handle = wglCreateContext(window_dc);
    if (!fake_glrc_handle) {
        R_Win32GL_ERROR("Failed to obtain a legacy OpenGL rendering context for bootstrapping");
        goto fail;
    }

    // Set OpenGL Context to Current Window
    // ------------------------------------
    if (!wglMakeCurrent(window_dc, fake_glrc_handle)) {
        R_Win32GL_ERROR("Failed to set legacy OpenGL rendering context to current window");
        goto fail;
    }

    // Load OpenGL Symbols and Extensions
    // ----------------------------------
    {
        if (!glew_init()) { goto fail; }
        if (!WGLEW_ARB_create_context || !WGLEW_ARB_create_context_profile) {
            ferror_re("GL", "Extension for requesting specific modern OpenGL context is not supported.");
            goto fail;
        }

        //wglCreateContextAttribsARB = (FType_wglCreateContextAttribsARB *)wglGetProcAddress("wglCreateContextAttribsARB");
        HGLRC share_context = NULL; // for sharing data across contexts in the future
        int attribs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 3, // clang-format off
                          WGL_CONTEXT_MINOR_VERSION_ARB, 2,
                          WGL_CONTEXT_PROFILE_MASK_ARB,
                          WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0 }; // clang-format on
        //if (!wglCreateContextAttribsARB) {
        //R_Win32GL_ERROR("Failed to get procedure wglCreateContextAttribsARB for creating modern OpenGL rendering context");
        //goto fail;
        //}
        modern_glrc_handle = wglCreateContextAttribsARB(window_dc, share_context, attribs);
        if (!modern_glrc_handle) {
            R_Win32GL_ERROR("Failed to obtain a modern OpenGL rendering context");
            goto fail;
        }
        if (!wglMakeCurrent(window_dc, modern_glrc_handle)) {
            R_Win32GL_ERROR("Failed to set modern OpenGL rendering context to current window");
            goto fail;
        }
        //finfo("Reinitializing GLEW for modern context");
        if (!glew_init()) { goto fail; } // initialize GLEW with the modern context
        gl_print_version_info();

        //wglSwapIntervalEXT = (FType_wglSwapIntervalEXT *)wglGetProcAddress("wglSwapIntervalEXT");
        if (wglSwapIntervalEXT) {
            if (!wglSwapIntervalEXT(1)) { warn_re("Renderer: Win32/GL", "Failed to enable VSync"); }
        }
        else {
            WIN32_DEF_LAST_ERROR_MSG(err_msg);
            ferror_re("Renderer: Win32/GL", "Failed to load WGL function \"wglSwapIntervalEXT\": %s", err_msg);
        }
    }

    // Cleanup
    // -------
    wglDeleteContext(fake_glrc_handle);

    // Populate Init Info
    // ------------------
    g_win32gl_info = (R_Win32GL_State){
        .window_handle = window_handle,
        .window_dc = window_dc,
        .glrc_handle = modern_glrc_handle,
    };

    return true;

fail:
    wglMakeCurrent(window_dc, NULL);
    wglDeleteContext(fake_glrc_handle);
    wglDeleteContext(modern_glrc_handle);
    ReleaseDC(window_handle, window_dc);
    // NOTE: Do not release the window handle. It is owned by the platform. We borrow it.
    return false;
}

/////////////////////////////////////////////////////////////////////////// SECTION: RENDERER API

//void p_platform_shutdown(void) {
//win32gl_destroy_wgl_context(g_win32gl_info.glrc_handle);
//win32_shutdown(g_win32gl_info.window_handle, g_win32gl_info.window_dc);
//}

void r_gl_platform_present(void) { SwapBuffers(g_win32gl_info.window_dc); }

void r_gl_platform_context_destroy(void) { win32gl_context_destroy(g_win32gl_info); }
