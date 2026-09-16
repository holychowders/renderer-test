#include "platform.h"
#include "platform_win32.h"
#include <malloc.h>

#include "hc_basic.h"
#include "hc_types.h"
#include "hc_log.h"

////////////////////////////////////////////////////////////////////////// SECTION: MISC

//#define WIN32_INFO(msg) info_re("Win32", (msg))
//#define WIN32_WARN(msg) warn_re("Win32", (msg))
//#define WIN32_ERROR(msg) error_re("Win32", (msg))
//#define WIN32_ERROR_PLATFORM(re, msg, code) ferror_re("Win32", "%s (code %lu)", (msg), (code))
//#define WIN32_ERROR_DETAILED(msg) ERROR_RE_DETAILED("Win32", (msg))

static const char g_win32_window_class_name[] = "Renderer-Window-Class";
static HINSTANCE g_win32_hinstance = { 0 };

struct P_PlatformWindow {
    HWND handle;
};

#if 0
    #ifdef __clang__
        #define dll_export __declspec(dllexport)
    #elif __GNUC__
        #define dll_export __attribute__((visibility("default"))
    #else
    #endif
#endif

////////////////////////////////////////////////////////////////////////// SECTION: WIN32

static inline void win32_get_last_error(char *out_msg, size_t out_len) {
    char *fmt_msg = { 0 };
    DWORD code = GetLastError();
    DWORD fmt_len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // clang-format off
                                   NULL, code, 0, (LPSTR)&fmt_msg, 0, NULL); // clang-format on
    if (fmt_len == 0) {
        error_re("Win32", "Failed to get last Win32 error");
        return;
    }

    str_trim_trailing_newline(fmt_msg, fmt_len);
    snprintf(out_msg, out_len, "%s", fmt_msg);
    LocalFree(fmt_msg);
}

static void win32_print_last_error(const char *re) {
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
    else { error_re("Win32", "Failed to print last Win32 error"); }
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
            //DestroyWindow(hWnd);
            PostQuitMessage(0);
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

// TODO: Multiple window classes
static inline B32 win32_window_class_register(void) {
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = win32_window_proc;
    wc.hInstance = g_win32_hinstance;
    wc.lpszClassName = g_win32_window_class_name;
    if (!RegisterClassExA(&wc)) {
        WIN32_DEF_LAST_ERROR_MSG(err_msg);
        ferror_re("Win32", "Failed to register window class: %s", err_msg);
        return false;
    }
    return true;
}

static inline HWND win32_window_create(HINSTANCE hInstance, U32 width, U32 height) {
    // clang-format off
    HWND handle = CreateWindowExA(0, // extended style
                                  g_win32_window_class_name, "Renderer", // names
                                  WS_OVERLAPPEDWINDOW, // style
                                  CW_USEDEFAULT, CW_USEDEFAULT, // x, y pos
                                  (S32)width, (S32)height, // width, height
                                  NULL, NULL, hInstance, NULL // misc
    ); // clang-format on
    if (!handle) {
        WIN32_DEF_LAST_ERROR_MSG(err_msg);
        ferror_re("Win32", "Failed to create window: %s", err_msg);
    }
    return handle;
}
static inline B32 win32_window_destroy(HWND window_handle) {
    if (!window_handle) {
        warn_re("Win32", "Failed to destroy window: Null window handle provided");
        return false;
    }
    if (!DestroyWindow(window_handle)) {
        warn_re("Win32", "Failed to destroy window");
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////// SECTION: WIN32-GL

//static inline GLuint win32gl_prg_create(const char *fpath_vertex, const char *fpath_fragment) {
//    char *vs_src = win32_prg_src_load(fpath_vertex);
//    char *fs_src = win32_prg_src_load(fpath_fragment);
//    if (!vs_src || !fs_src) { return (GLuint){ 0 }; }
//    GLuint prg = gl_prg_create(vs_src, fs_src); // TODO: Pass a vector of shader sources to support different shader types?
//    free(vs_src);
//    free(fs_src);
//    return prg;
//}

/////////////////////////////////////////////////////////////////////////// SECTION: PLATFORM API

int app_main(void);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    (void)nShowCmd, (void)lpCmdLine, (void)hPrevInstance;

    //Win32GL_InitInfo init_info = win32gl_init(hInstance, g_current_window_width, g_current_window_height);
    //if (!init_info.success) {
    //win32gl_shutdown(init_info);
    //return -1;
    //}

    g_win32_hinstance = hInstance;
    return app_main();
}

B32 p_platform_init(void) { return win32_window_class_register(); }
void p_platform_shutdown(void) {
    if (!UnregisterClassA(g_win32_window_class_name, g_win32_hinstance)) {
        win32_print_last_error("UnregisterClassA"); // TODO: proper error
    }
}

/// Note: Destroy the returned window with p_platform_window_destroy
P_PlatformWindow *p_platform_window_create(U32 width, U32 height) {
    P_PlatformWindow *window = (P_PlatformWindow *)malloc(sizeof(P_PlatformWindow));
    if (!window) {
        ferror_re("Win32", "Failed to allocate %zu bytes for platform window specification", sizeof(P_PlatformWindow));
        return NULL;
    }
    window->handle = win32_window_create(g_win32_hinstance, width, height);
    if (!window->handle) { return NULL; }
    ShowWindow(window->handle, SW_SHOWNORMAL);
    return window;
}
void p_platform_window_destroy(P_PlatformWindow *window) {
    if (!window) { return; } // TODO: proper error
    if (!win32_window_destroy(window->handle)) { return; }
    free(window);
}

B32 p_platform_process_events(S32 *exit_code) {
    MSG msg = { 0 };
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            *exit_code = (S32)msg.wParam;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

// TODO: Make this a generic file reading function
/// NOTE: Caller must free the returned shader source pointer
char *p_platform_load_shader_source(const char *path) {
    char *shader_source = { 0 };

    // Create File Handle
    // ------------------
    HANDLE file_handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        char error_message[512] = { 0 };
        win32_get_last_error(error_message, sizeof(error_message));
        ferror_re("Win32", "Failed to create handle to shader source file %s: %s", path, error_message);
        return shader_source;
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
                 path,
                 error_message);
    }
    shader_source = (char *)malloc(file_size_bytes + 1);
    if (shader_source) {
        // Read File and Set Null Terminator at End
        // ----------------------------------------
        DWORD bytes_read = { 0 };
        BOOL read_success = ReadFile(file_handle, shader_source, (DWORD)file_size_bytes, &bytes_read, NULL);
        if (read_success) {
            if (!bytes_read) { fwarn_re("Win32", "No bytes were read from shader source file %s. Is the file empty?", path); }
            shader_source[bytes_read] = '\0';
        }
        else {
            char error_message[512] = { 0 };
            win32_get_last_error(error_message, sizeof(error_message));
            ferror_re("Win32", "Failed to read shader source file %s: %s", path, error_message);
        }
    }
    else { ferror_re("Win32", "Failed to allocate %zu bytes for shader source (%s)\n     Will not read shader source", file_size_bytes + 1, path); }

    // Close File
    // ----------
    BOOL close_success = CloseHandle(file_handle);
    if (!close_success) {
        char error_message[512] = { 0 };
        win32_get_last_error(error_message, sizeof(error_message));
        ferror_re("Win32", "Failed to close handle to shader source file %s: %s", path, error_message);
    }

    return shader_source;
}

HWND p_win32_window_get_handle(P_PlatformWindow *window) {
    if (!window) {
        warn_re("Win32", "Failed to access window handle from provided null platform window object");
        return NULL;
    }
    return window->handle;
}

void p_win32_print_last_error(const char *re) { win32_print_last_error(re); }
void p_win32_get_last_error(char *out_msg, size_t out_len) { win32_get_last_error(out_msg, out_len); }
