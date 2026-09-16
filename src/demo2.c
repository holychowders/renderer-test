#include "renderer.h"

#include "hc_mat.h"

static U32 g_current_window_width = 1920;
static U32 g_current_window_height = 1080;

static P_PlatformWindow *init_app(void) {
    if (!p_platform_init()) { return NULL; } // (register window class (win32))
    // (create and show window, return opaque platform-specific window data)
    P_PlatformWindow *window = p_platform_window_create(g_current_window_width, g_current_window_height);
    if (!r_renderer_init(window)) { return NULL; } // (initialize wgl render context (win32+opengl))
    return window;
}

int app_main(void) {
    P_PlatformWindow *window = init_app();
    if (!window) { return -1; }

    // Sample Geometry and Transform
    // -----------------------------
    const F32 sample_vb[] = {
        /*pos*/ -0.5F, -0.5F, 0.0F, /*tex*/ 0.0F, 0.0F, // 0 bottom-left
        /*pos*/ +0.5F, -0.5F, 0.0F, /*tex*/ 1.0F, 0.0F, // 1 bottom-right
        /*pos*/ +0.5F, +0.5F, 0.0F, /*tex*/ 1.0F, 1.0F, // 2 top-right
        /*pos*/ -0.5F, +0.5F, 0.0F, /*tex*/ 0.0F, 1.0F, // 3 top-left
    };
    const U32 sample_ib[] = { 0, 1, 2, 0, 2, 3 };
    //R_Mesh sample_mesh = gl_vao_create(sample_vb, sample_ib, sizeof(sample_vb), sizeof(sample_ib));

    // Shaders
    // -------
    F32 u_color[] = { 1.0F, 0.25F, 1.25F, 1.0F };
    R_ShaderProgram prg = r_shader_program_create("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl", NULL, 0);
    r_shader_program_bind(prg);

    S32 exit_code = 0;
    while (p_platform_process_events(&exit_code)) {
        // Timer
        // -----
        static F32 timer = 0.0F;
        timer -= 0.016F;

        // Drawing
        // -------
        r_frame_begin();

        F32 r = absf32(sinf32(timer));
        r_clear_background(r, 0.1F, 0.1F, 1);
        //gl_vao_draw(sample_vao_info, prg.handle, u_color);

        r_frame_present();
    }

    //gl_vao_delete(&sample_vao_info);
    r_renderer_shutdown();
    p_platform_window_destroy(window);
    p_platform_shutdown();

    return exit_code;
}
