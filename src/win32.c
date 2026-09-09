#include "win32.h"
#include <malloc.h>

////////////////////////////////////////////////////////////////////////// SECTION: MISC

#define KILOBYTES_TO_BYTES(count) ((count) * (uint64_t)1024)
#define MEGABYTES_TO_BYTES(count) (KILOBYTES_TO_BYTES(count) * 1024)
#define GIGABYTES_TO_BYTES(count) (MEGABYTES_TO_BYTES(count) * 1024)
#define TERABYTES_TO_BYTES(count) (GIGABYTES_TO_BYTES(count) * 1024)

#define RET_IF_EQ(expr, eq)                                                                                                                          \
    do {                                                                                                                                             \
        if ((expr) == (eq)) return eq;                                                                                                               \
    } while (0)
#define RET_IF_0(expr) RET_IF_EQ((expr), 0)

#if defined(__MSC_VER)
    #define INTRIN_ALLOCA(size) _alloca(size)
#elif defined(__clang__) || defined(__GNUC__)
    #define INTRIN_ALLOCA(size) __builtin_alloca(size)
#else
    #error "Compiler intrinsic for alloca unavailable."
#endif

#if 0
    #ifdef __clang__
        #define dll_export __declspec(dllexport)
    #elif __GNUC__
        #define dll_export __attribute__((visibility("default"))
    #else
    #endif
#endif

/// Returns new length of passed string
static inline size_t str_trim_trailing_newline(char *str, size_t len) {
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
    return len;
}

////////////////////////////////////////////////////////////////////////// SECTION: WIN32

#define WIN32_INFO(msg) info_re("Win32", (msg))
#define WIN32_WARN(msg) warn_re("Win32", (msg))
#define WIN32_ERROR(msg) error_re("Win32", (msg))
#define WIN32_ERROR_PLATFORM(re, msg, code) ferror_re("Win32", "%s (code %lu)", (msg), (code))
#define WIN32_ERROR_DETAILED(msg) ERROR_RE_DETAILED("Win32", (msg))

static inline void win32_get_last_error(char *out_msg, size_t out_len) {
    char *fmt_msg = { 0 };
    DWORD code = GetLastError();
    DWORD fmt_len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // clang-format off
                               NULL, code, 0, (LPSTR)&fmt_msg, 0, NULL); // clang-format on
    if (fmt_len == 0) {
        WIN32_ERROR("Failed to get last Win32 error");
        return;
    }

    str_trim_trailing_newline(fmt_msg, fmt_len);
    snprintf(out_msg, out_len, "%s", fmt_msg);
    LocalFree(fmt_msg);
}

static inline void win32_print_last_error(const char *re) {
    char *msg = { 0 };
    DWORD code = GetLastError();
    DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               code,
                               0,
                               (LPSTR)&msg,
                               0,
                               NULL);
    if (len) {
        len = (DWORD)str_trim_trailing_newline(msg, len);
        ferror_re(re, "%s (code %lu)", msg, code);
        LocalFree(msg);
    }
    else { WIN32_ERROR("Failed to print last Win32 error"); }
}

/// NOTE: Caller must free the returned shader source pointer
static inline char *win32_prg_src_load(const char *fpath) {
    char *shader_source = { 0 };

    // Create File Handle
    // ------------------
    HANDLE file_handle = CreateFileA(fpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        char error_message[512] = { 0 };
        win32_get_last_error(error_message, sizeof(error_message));
        ferror_re("Win32", "Failed to create handle to shader source file %s: %s", fpath, error_message);
    }

    // Get File Size and Initialize Shader Source Buffer
    // -------------------------------------------------
    size_t file_size_bytes = { 0 };

    LARGE_INTEGER _file_size_bytes_li = { 0 };
    BOOL get_size_success = GetFileSizeEx(file_handle, &_file_size_bytes_li);
    if (get_size_success) { file_size_bytes = (size_t)_file_size_bytes_li.QuadPart; }
    else {
        file_size_bytes = KILOBYTES_TO_BYTES(64); // Fallback size, since actual size could not be determined
        char error_message[512] = { 0 };
        win32_get_last_error(error_message, sizeof(error_message));
        fwarn_re("Win32",
                 "Failed to get the size of shader source file %s: %s\n      Will use fallback size\n      Does the file exist?",
                 fpath,
                 error_message);
    }
    shader_source = (char *)malloc(file_size_bytes + 1);
    if (shader_source) {
        // Read File and Set Null Terminator at End
        // ----------------------------------------
        DWORD bytes_read = { 0 };
        BOOL read_success = ReadFile(file_handle, shader_source, (DWORD)file_size_bytes, &bytes_read, NULL);
        if (read_success) {
            if (!bytes_read) { fwarn_re("Win32", "No bytes were read from shader source file %s. Is the file empty?", fpath); }
            shader_source[bytes_read] = '\0';
        }
        else {
            char error_message[512] = { 0 };
            win32_get_last_error(error_message, sizeof(error_message));
            ferror_re("Win32", "Failed to read shader source file %s: %s", fpath, error_message);
        }
    }
    else { ferror_re("Win32", "Failed to allocate %zu bytes for shader source (%s)\n     Will not read shader source", file_size_bytes + 1, fpath); }

    // Close File
    // ----------
    BOOL close_success = CloseHandle(file_handle);
    if (!close_success) {
        char error_message[512] = { 0 };
        win32_get_last_error(error_message, sizeof(error_message));
        ferror_re("Win32", "Failed to close handle to shader source file %s: %s", fpath, error_message);
    }

    return shader_source;
}

static inline LRESULT CALLBACK win32_window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = { 0 };
    switch (uMsg) {
        case WM_KEYDOWN: {
            WPARAM vkcode = wParam;
            if (vkcode == 'Q' || vkcode == VK_ESCAPE) { PostMessageA(hWnd, WM_CLOSE, 0, 0); }
        } break;

        case WM_CLOSE: { // Request to close a window
            // TODO: Maybe ask for confirmation
            DestroyWindow(hWnd);
        } break;

        case WM_DESTROY: {
            PostQuitMessage(0); // Post WM_QUIT message to quit this thread's message queue
        } break;

        default: {
            result = DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }
    }
    return result;
}

static inline HWND win32_create_window(HINSTANCE hInstance, U32 width, U32 height) {
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = win32_window_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Renderer-Class";
    RegisterClassExA(&wc);

    // clang-format off
    HWND window_handle = CreateWindowExA(0, // extended style
                                         wc.lpszClassName, "Renderer", // names
                                         WS_OVERLAPPEDWINDOW, // style
                                         CW_USEDEFAULT, CW_USEDEFAULT, // x, y pos
                                         //CW_USEDEFAULT, CW_USEDEFAULT, // width, height
                                         (S32)width, (S32)height, // width, height
                                         NULL, NULL, hInstance, NULL // misc
    ); // clang-format on

    return window_handle;
}

////////////////////////////////////////////////////////////////////////// SECTION: WIN32-GL

// WGL function pointers
typedef BOOL WINAPI FType_wglSwapIntervalEXT(int interval);
typedef int WINAPI FType_wglGetSwapIntervalEXT(void);
static FType_wglSwapIntervalEXT *wglSwapIntervalEXT;

static inline B32 glew_init(void) {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        error_re("GL/GLEW", (const char *)glewGetErrorString(err));
        return false;
    }
    finfo_re("Renderer/GL/GLEW", "Version %s", (const char *)glewGetString(GLEW_VERSION));
    return true;
}

static inline B32 win32gl_set_pixel_format(HDC window_dc) {
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

    B32 success = SetPixelFormat(window_dc, px_format_idx_given, &px_format_given);

    return success;
}

Win32GL_InitInfo win32gl_init(HINSTANCE hInstance, U32 window_width, U32 window_height) {
    Win32GL_InitInfo init_info = { 0 };

    // Create window
    // -------------
    HWND window_handle = win32_create_window(hInstance, window_width, window_height);
    if (!window_handle) {
        WIN32_ERROR_DETAILED("Failed to create window");
        return init_info;
    }

    // Get window device context
    // ---------------------
    HDC window_dc = GetDC(window_handle);
    if (!window_dc) {
        WIN32_ERROR_DETAILED("Failed to get window device context");
        return init_info;
    }

    // Set pixel format
    // ----------------
    if (!win32gl_set_pixel_format(window_dc)) {
        WIN32_ERROR_DETAILED("Failed to set pixel format for OpenGL context");
        return init_info;
    }

    // Create OpenGL context
    // ---------------------
    HGLRC glrc_handle = wglCreateContext(window_dc);
    if (!glrc_handle) {
        WIN32_ERROR_DETAILED("Failed to create OpenGL rendering context");
        return init_info;
    }

    // Set OpenGL context to current window
    // ---------------------
    if (!wglMakeCurrent(window_dc, glrc_handle)) {
        WIN32_ERROR_DETAILED("Failed to set OpenGL rendering context to current window device context");
        return init_info;
    }

    // -----------
    ShowWindow(window_handle, SW_SHOWNORMAL);

    // Initialize OpenGL
    // -----------------
    if (!glew_init()) { return init_info; }

    //const GLubyte *extensions = glGetString(GL_EXTENSIONS);
    //const GLubyte *extensions = glGetStringi(count, GL_EXTENSIONS);
    //const char* term = "WGL_EXT_swap_control";

    // Load OpenGL extensions
    // ----------------------
    wglSwapIntervalEXT = (FType_wglSwapIntervalEXT *)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT) {
        if (!wglSwapIntervalEXT(1)) { warn_re("Win32/GL", "Failed to enable VSync"); }
    }
    else { win32_print_last_error("Win32/wglGetProcAddress(\"wglSwapIntervalEXT\")"); }

    init_info.success = true;
    init_info.window_handle = window_handle;
    init_info.window_dc = window_dc;
    init_info.glrc_handle = glrc_handle;
    return init_info;
}

void win32gl_shutdown(Win32GL_InitInfo init_info) {
    wglMakeCurrent(NULL, NULL);
    if (init_info.glrc_handle) {
        if (wglDeleteContext(init_info.glrc_handle)) { init_info.glrc_handle = NULL; }
        else { warn_re("Win32/GL", "Failed to delete the OpenGL rendering context"); }
    }
    if (init_info.window_handle) {
        if (init_info.window_dc) {
            if (ReleaseDC(init_info.window_handle, init_info.window_dc)) { init_info.window_dc = NULL; }
            else { warn_re("Win32", "Failed to release the window device context"); }
        }
        if (DestroyWindow(init_info.window_handle)) { init_info.window_handle = NULL; }
        else { warn_re("Win32", "Failed to destroy window"); }
    }
}

GLuint win32gl_prg_create(void) {
    char *vs_src = win32_prg_src_load("assets/shaders/vertex.glsl");
    char *fs_src = win32_prg_src_load("assets/shaders/fragment.glsl");
    if (!vs_src || !fs_src) { return (GLuint){ 0 }; }
    GLuint prg = gl_prg_create(vs_src, fs_src); // TODO: Pass a vector of shader sources to support different shader types?
    free(vs_src);
    free(fs_src);
    return prg;
}
