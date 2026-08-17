#include "gl.h"

#include "hc_mat.h"
#include "hc_vec.h"

#include <Windows.h>

#include <malloc.h>

////////////////////////////////////////////////////////////////////////// SECTION: FUNCTION POINTERS

typedef BOOL WINAPI FType_wglSwapIntervalEXT(int interval);
typedef int WINAPI FType_wglGetSwapIntervalEXT(void);
static FType_wglSwapIntervalEXT *wglSwapIntervalEXT;

////////////////////////////////////////////////////////////////////////// SECTION: CONSTANTS

static B32 g_running = true;

////////////////////////////////////////////////////////////////////////// SECTION: MACROS

#define KILOBYTES_TO_BYTES(count) ((count) * (uint64_t)1024)
#define MEGABYTES_TO_BYTES(count) (KILOBYTES_TO_BYTES(count) * 1024)
#define GIGABYTES_TO_BYTES(count) (MEGABYTES_TO_BYTES(count) * 1024)
#define TERABYTES_TO_BYTES(count) (GIGABYTES_TO_BYTES(count) * 1024)

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

////////////////////////////////////////////////////////////////////////// SECTION: MISC FUNCTIONS

static inline size_t str_trim_trailing_newline(char *str, size_t len) {
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
    return len;
}

static inline void win32_get_last_error(char **out_msg) {
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
        *out_msg = msg;
        LocalFree(msg);
    }
    else { WIN32_ERROR("Failed to print last Win32 error"); }
}

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
            gl_clear_errors();                                                                                                                       \
            gl_operation;                                                                                                                            \
            ASSERT_MSG(!gl_check_errors(), #gl_operation);                                                                                           \
        } while (0)
#endif

static inline char *win32_prg_src_load(const char *fpath) {
    char *shader_source = { 0 };

    // Create File Handle
    // ------------------
    HANDLE file_handle = CreateFileA(fpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (!file_handle) {
        char *error_message = { 0 };
        win32_get_last_error(&error_message);
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
        char *error_message = { 0 };
        win32_get_last_error(&error_message);
        fwarn_re("Win32", "Failed to get the size of shader source file %s: %s\n      Will use fallback size", fpath, error_message);
    }
    shader_source = (char *)malloc(file_size_bytes + 1);
    if (!shader_source) {
        // TODO: How to properly handle?
        ferror_re("Win32", "Failed to allocate %zu bytes for shader source (%s)\n     CONTINUE WITH CAUTION", file_size_bytes + 1, fpath);
        //DEBUG_BREAK();
    }

    // Read File and Set Null Terminator at End
    // ----------------------------------------
    DWORD bytes_read = { 0 };
    BOOL read_success = ReadFile(file_handle, shader_source, (DWORD)file_size_bytes, &bytes_read, NULL);
    if (read_success) {
        if (!bytes_read) { fwarn_re("Win32", "No bytes were read from shader source file %s. Is the file empty?", fpath); }
        shader_source[bytes_read] = '\0';
    }
    else {
        char *error_message = { 0 };
        win32_get_last_error(&error_message);
        ferror_re("Win32", "Failed to read shader source file %s: %s", fpath, error_message);
    }

    // Close File
    // ----------
    BOOL close_success = CloseHandle(file_handle);
    if (!close_success) {
        char *error_message = { 0 };
        win32_get_last_error(&error_message);
        ferror_re("Win32", "Failed to close handle to shader source file %s: %s", fpath, error_message);
    }

    return shader_source;
}

////////////////////////////////////////////////////////////////////////// SECTION: WIN32 FUNCTIONS

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

static inline HWND win32_create_window(HINSTANCE hInstance) {
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = win32_window_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "App-Class";
    RegisterClassExA(&wc);

    // clang-format off
    HWND window_handle = CreateWindowExA(0, // extended style
                                         wc.lpszClassName, "App", // names
                                         WS_OVERLAPPEDWINDOW, // style
                                         CW_USEDEFAULT, CW_USEDEFAULT, // x, y pos
                                         //CW_USEDEFAULT, CW_USEDEFAULT, // width, height
                                         720, 720, // width, height
                                         NULL, NULL, hInstance, NULL // misc
    ); // clang-format on

    return window_handle;
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

typedef struct Win32GL_InitInfo {
    B32 success;
    HWND window_handle;
    HDC window_dc;
    HGLRC glrc_handle;
} Win32GL_InitInfo;

static inline Win32GL_InitInfo win32gl_init(HINSTANCE hInstance) {
    Win32GL_InitInfo init_info = { 0 };

    // Create window
    // -------------
    HWND window_handle = win32_create_window(hInstance);
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
    if (!gl_glew_init()) { return init_info; }
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_BLEND));
    GL(glDepthFunc(GL_LESS));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

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

static inline void win32gl_shutdown(Win32GL_InitInfo init_info) {
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

static inline GLuint win32gl_prg_create(void) {
    char *vs_src = win32_prg_src_load("assets/shaders/vertex.glsl");
    char *fs_src = win32_prg_src_load("assets/shaders/fragment.glsl");
    GLuint prg = gl_prg_create(vs_src, fs_src); // TODO: Pass a vector of shader sources?
    free(vs_src);
    free(fs_src);
    return prg;
}

//static Mat4F32 calculate_mvp(const Transform &transform, const Mat4F32 &view, const Mat4F32 &projection) {
//    Mat4F32 model = Mat4F32(1.0F);
//    model = glm::translate(model, transform.pos);
//
//    model = glm::rotate(model, transform.ori.x, glm::vec3(1.0F, 0.F, 0.0F));
//    model = glm::rotate(model, transform.ori.y, glm::vec3(0.0F, 1.F, 0.0F));
//    model = glm::rotate(model, transform.ori.z, glm::vec3(0.0F, 0.F, 1.0F));
//
//    model = glm::scale(model, transform.scale);
//
//    return projection * view * model;
//}

typedef struct Transform {
    Vec3F32 pos;
    Vec3F32 scale;
    Vec3F32 ori;
    Vec3F32 angvel;
} Transform;

static inline Mat4F32 calculate_mvp(const Transform transform, const Mat4F32 view, const Mat4F32 projection) {
    Mat4F32 model = mat4f32_identity();
    model = mat4f32_trans(model, transform.pos);
    model = mat4f32_rot_xyz(model, transform.ori);
    model = mat4f32_scale(model, transform.scale);
    return mat4f32_mul(mat4f32_mul(model, view), projection);
}

static inline void test_mat(void) {
    // Original Matrix
    // ---------------
    // [ 1 2 ]
    // [ 3 4 ]
    Mat2F32 mat1 = { .e = { 1, 2, 3, 4 } }; // store it internally as either row or column major, but init is the same

    // Multiply by Identity Matrix and Verify
    // --------------------------------------
    // [ 1 0 ]
    // [ 0 1 ]
    Mat2F32 res1 = mat2f32_mul(mat1, mat2f32_identity());
    ASSERT(mat2f32_eq(res1, mat1));

    // Multiply by Another Matrix and Verify
    // -------------------------------------
    // [ 5 6 ]
    // [ 7 8 ]
    Mat2F32 res2 = mat2f32_mul(mat1, (Mat2F32){ 5, 6, 7, 8 });
    ASSERT(mat2f32_eq(res2, (Mat2F32){ 19, 22, 43, 50 }));

    // Alternative Forms of Comparison to Consider
    // -------------------------------------------
    //                    row1    row2
    //ASSERT(res2.e == { 19, 22, 43, 50 });
    //ASSERT(mat2f32_eq(res2, res2_expected));
    //ASSERT(arreq(res2.e, res2_expected.e));
    //ASSERT(MAT_EQ(res2, res2_expected));
    //ASSERT(mat_eq(res2, res2_expected));
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    (void)nShowCmd, (void)lpCmdLine, (void)hPrevInstance;
    Win32GL_InitInfo init_info = win32gl_init(hInstance);
    if (!init_info.success) {
        win32gl_shutdown(init_info);
        return -1;
    }

    test_mat();

    // Sample Geometry and Transform
    // -----------------------------
    const F32 vertex_buffer[] = {
        /*pos*/ -0.5F, -0.5F, 0.0F, /*tex*/ 0.0F, 0.0F, // 0 bottom-left
        /*pos*/ +0.5F, -0.5F, 0.0F, /*tex*/ 1.0F, 0.0F, // 1 bottom-right
        /*pos*/ +0.5F, +0.5F, 0.0F, /*tex*/ 1.0F, 1.0F, // 2 top-right
        /*pos*/ -0.5F, +0.5F, 0.0F, /*tex*/ 0.0F, 1.0F, // 3 top-left
    };
    const U32 index_buffer[] = { 0, 1, 2, 0, 2, 3 };
    GL_VAOInfo vao_info = gl_vao_create(vertex_buffer, index_buffer, sizeof(vertex_buffer), sizeof(index_buffer));

    //Mat4F32 model_transform = {};

    // Textures
    // --------
    GLuint texture = gl_texture_from_image("assets/images/Faces for a Dying Land/creep12.png");
    GL(glBindTextureUnit(0, texture));

    // Shaders
    // -------
    GLuint shader_program = win32gl_prg_create();
    gl_prg_bind(shader_program);

    // Shared Transforms
    // -----------------

    // Shader Uniforms
    // ---------------
    // u_color
    F32 u_color[] = { 1.0F, 0.25F, 0.25F, 1.0F };

    // u_texunit
    GLint u_texunit1_loc = gl_prg_get_uloc(shader_program, "u_texunit1");
    GL(glUniform1i(u_texunit1_loc, 0));

    // u_mvp
    //Mat4F32 u_mvp = calculate_mvp(transform, view_matrix, proj_matrix);
    GLint u_mvp_loc = gl_prg_get_uloc(shader_program, "u_mvp");
    //GL(glUniformMatrix4fv(u_mvp_loc, 1, GL_FALSE, &u_mvp[0][0]));

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

        gl_clear_background(0.1F, 0.1F, 0.1F, 1);
        gl_vao_draw(vao_info, shader_program, u_color);

        SwapBuffers(init_info.window_dc);
    }

    gl_vao_delete(&vao_info);

    win32gl_shutdown(init_info);

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

#if 0
typedef struct Image {
    U32 width;
    U32 height;
    U32 channels;
    U32 bytes_per_pixel;
    void *pixels;
} Image;

static inline Image image_load(const char *fpath) {
    Image result = { 0 };

    S32 width = { 0 }, height = { 0 }, channels = { 0 };
    stbi_uc *image = stbi_load(fpath, &width, &height, &channels, 4);

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
#endif
