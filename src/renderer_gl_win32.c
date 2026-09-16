#include "renderer_gl_platform.h"
#include "platform_win32.h"
#include <Windows.h>
#include <wingdi.h>

#include "hc_log.h"
#include "hc_assert.h"

#define R_Win32GL_ERROR(msg) ERROR_RE_DETAILED("Renderer: Win32/GL", (msg))

typedef struct {
    HWND window_handle; // borrowed from platform
    HDC window_dc;      // owned
    HGLRC glrc_handle;  // owned
} R_Win32GL_State;

static R_Win32GL_State g_win32gl_info = { 0 };

// WGL function pointers
typedef BOOL WINAPI FType_wglSwapIntervalEXT(int interval);
typedef int WINAPI FType_wglGetSwapIntervalEXT(void);
static FType_wglSwapIntervalEXT *wglSwapIntervalEXT;

// TODO: Keep WGL-equivalent pixel format functions in mind for more control in the future (eg, wglChoosePixelFormatARB)
// TODO: Allow the renderer to provide a different pixel format
static inline B32 win32gl_set_pixel_format(HDC window_dc) {
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

B32 r_gl_platform_context_create(P_PlatformWindow *window) {
    R_Win32GL_State init_info = { 0 };

    // Get Window Handle
    // -----------------
    HWND window_handle = p_win32_window_get_handle(window);
    if (!window_handle) {
        R_Win32GL_ERROR("Failed to get valid window handle");
        return false;
    }

    // Get Window Device Context
    // -------------------------
    HDC window_dc = GetDC(window_handle);
    if (!window_dc) {
        R_Win32GL_ERROR("Failed to get window device context");
        ReleaseDC(window_handle, window_dc);
        return false;
    }

    // Set Pixel Format
    // ----------------
    if (!win32gl_set_pixel_format(window_dc)) {
        R_Win32GL_ERROR("Failed to set window pixel format");
        ReleaseDC(window_handle, window_dc);
        return false;
    }

    // Create OpenGL Context
    // ---------------------
    HGLRC glrc_handle = wglCreateContext(window_dc);
    if (!glrc_handle) {
        R_Win32GL_ERROR("Failed to create OpenGL rendering context");
        ReleaseDC(window_handle, window_dc);
        return false;
    }

    // Set OpenGL Context to Current Window
    // ------------------------------------
    if (!wglMakeCurrent(window_dc, glrc_handle)) {
        wglDeleteContext(glrc_handle);
        ReleaseDC(window_handle, window_dc);
        R_Win32GL_ERROR("Failed to set OpenGL rendering context to current window");
        return false;
    }

    //const GLubyte *extensions = glGetString(GL_EXTENSIONS);
    //const GLubyte *extensions = glGetStringi(count, GL_EXTENSIONS);
    //const char* term = "WGL_EXT_swap_control";

    // Load OpenGL Extensions
    // ----------------------
    wglSwapIntervalEXT = (FType_wglSwapIntervalEXT *)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT) {
        if (!wglSwapIntervalEXT(1)) { warn_re("Renderer: Win32/GL", "Failed to enable VSync"); }
    }
    else {
        WIN32_DEF_LAST_ERROR_MSG(err_msg);
        ferror_re("Renderer: Win32/GL", "Failed to load WGL function \"wglSwapIntervalEXT\": %s", err_msg);
    }

    init_info.window_handle = window_handle;
    init_info.window_dc = window_dc;
    init_info.glrc_handle = glrc_handle;
    g_win32gl_info = init_info;

    return true;
}

/////////////////////////////////////////////////////////////////////////// SECTION: RENDERER API

//void p_platform_shutdown(void) {
//win32gl_destroy_wgl_context(g_win32gl_info.glrc_handle);
//win32_shutdown(g_win32gl_info.window_handle, g_win32gl_info.window_dc);
//}

void r_gl_platform_present(void) { SwapBuffers(g_win32gl_info.window_dc); }

void r_gl_platform_context_destroy(void) { win32gl_context_destroy(g_win32gl_info); }
