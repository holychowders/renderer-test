#include "hc_log.h"
#include "hc_assert.h"
#include "hc_types.h"

#include <Windows.h>

#include <GL/glew.h>
//#include <GL/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

////////////////////////////////////////////////////////////////////////// SECTION: FUNCTION POINTERS

typedef BOOL WINAPI FType_wglSwapIntervalEXT(int interval);
typedef int WINAPI FType_wglGetSwapIntervalEXT(void);
static FType_wglSwapIntervalEXT *wglSwapIntervalEXT;

////////////////////////////////////////////////////////////////////////// SECTION: CONSTANTS

static B32 g_running = true;

////////////////////////////////////////////////////////////////////////// SECTION: MACROS

#define WIN32_INFO(msg) info_re("Win32", (msg))
#define WIN32_WARN(msg) warn_re("Win32", (msg))
#define WIN32_ERROR(msg) error_re("Win32", (msg))
#define WIN32_ERROR_PLATFORM(re, msg, code) ferror_re("Win32", "%s (code %lu)", (msg), (code))
#define WIN32_ERROR_DETAILED(msg) ERROR_RE_DETAILED("Win32", (msg))

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

////////////////////////////////////////////////////////////////////////// SECTION: PLATFORM-SPECIFIC APP FUNCTION IMPLEMENTATIONS

////////////////////////////////////////////////////////////////////////// SECTION: GL FUNCTIONS

#if 0
    #define GL_INFO(msg) info_re("Renderer/GL", (msg))
    #define GL_WARN(msg) warn_re("Renderer/GL", (msg))
    #define GL_ERROR(msg) error_re("Renderer/GL", (msg))
    #define GL_ERROR_DETAILED(msg) ERROR_RE_DETAILED("Renderer/GL", (msg))
#endif

#if 0
    #define GL(gl_operation)                                                                                                                         \
        do {                                                                                                                                         \
            win32gl_clear_errors();                                                                                                                  \
            gl_operation;                                                                                                                            \
            ASSERT_MSG(!win32gl_check_errors(), #gl_operation);                                                                                      \
        } while (0)
#endif

#define GL(gl_operation)                                                                                                                             \
    win32gl_clear_errors();                                                                                                                          \
    gl_operation;                                                                                                                                    \
    ASSERT_MSG(!win32gl_check_errors(), #gl_operation)

static inline B32 win32gl_check_errors(void) {
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
        error_re("GL", msg);
    }
    return has_error;
}

static inline void win32gl_clear_errors(void) {
    while (glGetError() != GL_NO_ERROR) {}
}

static inline void win32gl_clear_background(F32 r, F32 g, F32 b, F32 a) {
    //GL(glViewport(0, 0, window_width, window_height));
    GL(glClearColor(r, g, b, a));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

static inline B32 win32gl_glew_init(void) {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        error_re("GL/GLEW", (const char *)glewGetErrorString(err));
        return false;
    }
    finfo_re("Renderer/GL/GLEW", "Version %s", (const char *)glewGetString(GLEW_VERSION));
    return true;
}

////////////////////////////////////////////////////////////////////////// SECTION: OPENGL FUNCTIONS (SHADERS)

static B32 shader_source_verify(GLuint shader, GLenum shader_type) {
    GLint compile_success = GL_FALSE;
    GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_success));
    if (!compile_success) {
        GLint log_len = { 0 };
        GL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len));

        char log_message[2048];
        GL(glGetShaderInfoLog(shader, log_len, &log_len, log_message));

        const char *shader_type_str = "vertex";
        if (shader_type == GL_VERTEX_SHADER) { shader_type_str = "vertex"; }
        else if (shader_type == GL_FRAGMENT_SHADER) { shader_type_str = "fragment"; }

        char fmsg[sizeof(log_message) + 128];
        snprintf(fmsg, sizeof(fmsg), "Failed to compile %s shader\n%s", shader_type_str, log_message);
        error_re("GL", fmsg);
        return false;
    }
    return true;
}

static B32 shader_program_bind(GLuint prg) {
    if (!prg) {
        error_re("GL", "Failed to bind shader program (null shader program provided)");
        return false;
    }
    GL(glUseProgram(prg));
    return true;
}

static void shader_program_unbind() {
    GL(glUseProgram(0));
}

static GLint shader_program_get_uniform_location(GLuint shader_program, const char *name) {
    GL(GLint location = glGetUniformLocation(shader_program, name));
    if (location == -1) { fwarn(NULL, "Failed to get uniform location: %s", name); }
    return location;
}

static B32 shader_program_verify(GLuint prg) {
    // Check Link Status
    GLint link_success = GL_FALSE;
    GL(glGetProgramiv(prg, GL_LINK_STATUS, &link_success));
    if (!link_success) {
        char log_message[2048] = { 0 };
        GL(glGetProgramInfoLog(prg, sizeof(log_message), NULL, log_message));
        if (log_message[0]) { ferror_re("GL", "Failed to link shader program: %s", log_message); }
        else { error_re("GL", "Failed to link shader program"); }
    }

    GLint validate_success = GL_FALSE;
    if (link_success) {
        // Check Validation Status
        GL(glValidateProgram(prg));
        GL(glGetProgramiv(prg, GL_VALIDATE_STATUS, &validate_success));
        if (!validate_success) {
            char log_message[2048] = { 0 };
            GL(glGetProgramInfoLog(prg, sizeof(log_message), NULL, log_message));
            if (log_message[0]) { ferror_re("GL", "Failed to validate shader program: %s", log_message); }
            else { error_re("GL", "Failed to validate shader program"); }
        }
    }

    return (validate_success && link_success);
}

/// Returns created shader program object. Returns 0 on failure.
static GLuint shader_program_create(const char *vs_src, const char *fs_src) {
    ASSERT(vs_src);
    ASSERT(fs_src);

    // Vertex Shader
    // -------------
    GL(GLuint vs = glCreateShader(GL_VERTEX_SHADER));
    GL(glShaderSource(vs, 1, &vs_src, NULL));
    GL(glCompileShader(vs));
    B32 vs_ok = shader_source_verify(vs, GL_VERTEX_SHADER);

    // Fragment Shader
    // ---------------
    GL(GLuint fs = glCreateShader(GL_FRAGMENT_SHADER));
    GL(glShaderSource(fs, 1, &fs_src, NULL));

    GL(glCompileShader(fs));
    B32 fs_ok = shader_source_verify(fs, GL_FRAGMENT_SHADER);

    // Program
    // -------
    GLuint prg = 0;
    if (vs_ok && fs_ok) {
        GL(prg = glCreateProgram());
        GL(glAttachShader(prg, vs));
        GL(glAttachShader(prg, fs));
        GL(glLinkProgram(prg));
        B32 prg_ok = shader_program_verify(prg);
        if (!prg_ok) {
            error_re("GL", "Failed to create shader program");
            GL(glDeleteProgram(prg));
        }
    }

    // Delete Intermediate Shader Objects
    GL(glDeleteShader(vs));
    GL(glDeleteShader(fs));
    vs = 0;
    fs = 0;

    return prg;
}

////////////////////////////////////////////////////////////////////////// SECTION: WIN32 FUNCTIONS

static inline size_t str_trim_trailing_newline(char *str, size_t len) {
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
    return len;
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
    else { WIN32_ERROR("Failed to print last Win32 error"); }
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    (void)nShowCmd, (void)lpCmdLine, (void)hPrevInstance;

    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = win32_window_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "App-Class";
    RegisterClassExA(&wc);

    HWND window_handle = CreateWindowExA(0, // extended style
                                         wc.lpszClassName,
                                         "App",               // names
                                         WS_OVERLAPPEDWINDOW, // style
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT, // x, y pos
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT, // width, height
                                         NULL,
                                         NULL,
                                         hInstance,
                                         NULL // misc
    );

    if (!window_handle) {
        WIN32_ERROR_DETAILED("Failed to create window");
        return -1;
    }

    // Create OpenGL context
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
        return -1;
    }

    HGLRC glrc_handle = wglCreateContext(window_dc);
    if (!glrc_handle) {
        WIN32_ERROR_DETAILED("Failed to create OpenGL rendering context");
        return -1;
    }

    if (!wglMakeCurrent(window_dc, glrc_handle)) {
        WIN32_ERROR_DETAILED("Failed to set OpenGL rendering context to current window device context");
        return -1;
    }

    if (!ReleaseDC(window_handle, window_dc)) { WIN32_WARN("Failed to release window handle and device context"); }

    ShowWindow(window_handle, SW_SHOWNORMAL);

    // OpenGL Init
    if (!win32gl_glew_init()) { return false; }
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_BLEND));
    GL(glDepthFunc(GL_LESS));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    //const GLubyte *extensions = glGetString(GL_EXTENSIONS);
    //const GLubyte *extensions = glGetStringi(count, GL_EXTENSIONS);
    //const char* term = "WGL_EXT_swap_control";

    wglSwapIntervalEXT = (FType_wglSwapIntervalEXT *)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1); // TODO: Verify that VSync is actually used
    }
    else { win32_print_last_error("Win32/wglGetProcAddress(\"wglSwapIntervalEXT\")"); }

    //shader_program_create();

    S32 exit_code = 0;
    while (g_running) {
        MSG msg = { 0 };
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_running = false;
                exit_code = (S32)msg.wParam;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!g_running) { break; }
        SwapBuffers(window_dc);
        win32gl_clear_background(0.1F, 0.1F, 0.1F, 1);
    }

    // Shutdown
    ASSERT(wglMakeCurrent(NULL, NULL));
    ASSERT(wglDeleteContext(glrc_handle));

    return exit_code;
}

#if 0
    #ifdef __clang__
        #define dll_export __declspec(dllexport)
    #elif __GNUC__
        #define dll_export __attribute__((visibility("default"))
    #else
    #endif
#endif

typedef struct Image {
    U32 width;
    U32 height;
    U32 channels;
    U32 bytes_per_pixel;
    void *pixels;
} Image;

static inline Image image_load(const char *path) {
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
static inline void image_free(Image *image) {
    stbi_image_free(image->pixels);
    *image = (Image){ 0 };
}
