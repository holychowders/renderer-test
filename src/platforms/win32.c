#include "platform.h"

#include "hc_log.h"
#include "hc_assert.h"

#include <Windows.h>

////////////////////////////////////////////////////////////////////////// SECTION: MACROS

#define WIN32_INFO(msg) info_re("Renderer/Win32", (msg))
#define WIN32_WARN(msg) warn_re("Renderer/Win32", (msg))
#define WIN32_ERROR(msg) error_re("Renderer/Win32", (msg))
#define WIN32_ERROR_INTERNAL(msg, code) ferror_re("Renderer/Win32/Internal", "%s (code %lu)", (msg), (code))
#define WIN32_ERROR_DETAILED(msg) ERROR_RE_DETAILED("Renderer/Win32", (msg))

#define RET_IF_EQ(expr, eq)                                                                                                                          \
    do {                                                                                                                                             \
        if ((expr) == eq) return eq;                                                                                                                 \
    } while (0)
#define RET_IF_0(expr) RET_IF_EQ((expr), 0)

#if defined(__MSC_VER)
    #define INTRIN_ALLOCA(size) _alloca(size)
#elif defined(__clang__) || defined(__GNUC__)
    #define INTRIN_ALLOCA(size) __builtin_alloca(size)
#else
    #error "Compiler intrinsic for alloca unavailable."
#endif

////////////////////////////////////////////////////////////////////////// SECTION: DATA STRUCTURES

struct PlatformData {
    HWND window_handle;
    HGLRC glrc_handle;
};

////////////////////////////////////////////////////////////////////////// SECTION: MISC FUNCTIONS

static void print_last_win32_error(void) {
    char *msg = { 0 };
    DWORD code = GetLastError();
    char* fmt_args = "%1.%0"; // no trailing newline
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING, NULL, code, 0, (char *)&msg, 0, &fmt_args);
    WIN32_ERROR_INTERNAL(msg, code);
    LocalFree(msg);
}

////////////////////////////////////////////////////////////////////////// SECTION: FUNCTIONS

static inline LRESULT CALLBACK window_procedure_callback(HWND window_handle, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN: {
            WPARAM vkcode = wParam;
            if (vkcode == 'Q' || vkcode == VK_ESCAPE) { PostQuitMessage(0); }
        } break;
        case WM_QUIT:
        case WM_CLOSE: {
            PostQuitMessage(0);
        } break;
        default: { DefWindowProcA(window_handle, uMsg, wParam, lParam); }
    }
}

B32 platform_init(Str8 window_title, U32 window_width, U32 window_height) {
    // Window title string to wide
    int window_title_w_size = MultiByteToWideChar(CP_UTF8, 0, window_title.str, (int)window_title.size_bytes, NULL, 0);
    PWSTR window_title_w = (PWSTR)INTRIN_ALLOCA((size_t)window_title_w_size);
    MultiByteToWideChar(CP_UTF8, 0, window_title.str, (int)window_title.size_bytes, window_title_w, window_title_w_size);

    // Register and create window
    HINSTANCE hinstance = GetModuleHandle(NULL);
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = window_procedure_callback;
    wc.hInstance = hinstance;
    wc.lpszClassName = window_title_w;
    RegisterClassEx(&wc);
    // clang-format off
    HWND window_handle = CreateWindowEx(0, window_title_w, window_title_w, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, (int)window_width, (int)window_height, NULL, NULL, hinstance, NULL); // clang-format on
    if (!window_handle) {
        WIN32_ERROR_DETAILED("Failed to create window");
        return false;
    }

    // Create OpenGL context
    // FIXME: Handle DirectX too
    HDC window_dc = GetDC(window_handle);

    // Requested pixel format
    PIXELFORMATDESCRIPTOR px_format_requested = { 0 };
    px_format_requested.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    px_format_requested.nVersion = 1;
    px_format_requested.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    px_format_requested.iPixelType = PFD_TYPE_RGBA;
    px_format_requested.cColorBits = 32; // TODO: The docs say "excluding the alpha bitplanes", but then it gives 32 bits anyway?
    px_format_requested.cAlphaBits = 8;
    px_format_requested.iLayerType = PFD_MAIN_PLANE;

    // Given pixel format
    // Get an index into the list of supported formats, to a format that best matches the requested format
    int px_format_idx_given = ChoosePixelFormat(window_dc, &px_format_requested);

    // Get a description of the pixel format given
    PIXELFORMATDESCRIPTOR px_format_given = { 0 };
    DescribePixelFormat(window_dc, px_format_idx_given, sizeof(PIXELFORMATDESCRIPTOR), &px_format_given);

    if (!SetPixelFormat(window_dc, px_format_idx_given, &px_format_given)) {
        WIN32_ERROR_DETAILED("Failed to set pixel format for OpenGL context");
        return false;
    }

    HGLRC glrc_handle = wglCreateContext(window_dc);
    if (!glrc_handle) {
        WIN32_ERROR_DETAILED("Failed to create OpenGL rendering context");
        return false;
    }

    if (!wglMakeCurrent(window_dc, glrc_handle)) {
        WIN32_ERROR_DETAILED("Failed to set OpenGL rendering context to current window device context");
        return false;
    }

    if (!ReleaseDC(window_handle, window_dc)) { WIN32_WARN("Failed to release window handle and device context"); }

    ShowWindow(window_handle, SW_SHOWNORMAL);

    WIN32_INFO("Initialized");

    return true;
}

B32 platform_shutdown(PlatformData *platform_data) {
    ASSERT(wglMakeCurrent(NULL, NULL));
    // TODO: check if this fails with an invalid glrc_handle so we know whether we need to check if glrc_handle is null before we call delete context
    ASSERT(wglDeleteContext(NULL));
    return (B32)wglDeleteContext(platform_data->glrc_handle);
}

B32 platform_poll_events(void) {
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}
