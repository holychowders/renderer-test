#include "gl.h"
#include "win32.h"

#include "hc_mat.h"

////////////////////////////////////////////////////////////////////////// SECTION: GLOBALS

static B32 g_running = true;
static U32 g_current_window_width = 1920;
static U32 g_current_window_height = 1080;

////////////////////////////////////////////////////////////////////////// SECTION: RENDERER

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
    return mat4f32_mul(projection, mat4f32_mul(view, model));
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

static inline void gl_init(void) {
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_BLEND));
    GL(glDepthFunc(GL_LESS));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
}

//typedef struct {
//    const char* name;
//    const char* fpath;
//    const char* type;
//} ShaderSource;

static inline GL_Program gl_prg_make(const char *vs_path, const char *fs_path, const char **u_names, U32 u_count) {
    GL_Program prg = { 0 };
    prg.handle = win32gl_prg_create(vs_path, fs_path);

    // TODO: Decide how to handle this. Either make the buffer really large or dynamic.
    ASSERT_MSG(u_count <= ARRAY_COUNT(prg.u_locs), "Too many uniforms to store");

    for (U32 u_idx = 0; u_idx < u_count; u_idx++) {
        finfo("Caching uniform location: %s", u_names[u_idx]);
        prg.u_locs[u_idx] = (GL_Uniform){ .name = u_names[u_idx], .loc = gl_prg_get_u_loc(prg.handle, u_names[u_idx]) };
    }
    prg.u_count = u_count;

    return prg;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    (void)nShowCmd, (void)lpCmdLine, (void)hPrevInstance;
    Win32GL_InitInfo init_info = win32gl_init(hInstance, g_current_window_width, g_current_window_height);
    if (!init_info.success) {
        win32gl_shutdown(init_info);
        return -1;
    }
    gl_init();

    test_mat();

    // Sample Geometry and Transform
    // -----------------------------
    const F32 sample_vb[] = {
        /*pos*/ -0.5F, -0.5F, 0.0F, /*tex*/ 0.0F, 0.0F, // 0 bottom-left
        /*pos*/ +0.5F, -0.5F, 0.0F, /*tex*/ 1.0F, 0.0F, // 1 bottom-right
        /*pos*/ +0.5F, +0.5F, 0.0F, /*tex*/ 1.0F, 1.0F, // 2 top-right
        /*pos*/ -0.5F, +0.5F, 0.0F, /*tex*/ 0.0F, 1.0F, // 3 top-left
    };
    const U32 sample_ib[] = { 0, 1, 2, 0, 2, 3 };
    GL_VAOInfo sample_vao_info = gl_vao_create(sample_vb, sample_ib, sizeof(sample_vb), sizeof(sample_ib));

    Transform sample_transform = { .pos = (Vec3F32){ 0, 0, 0 },
                                   .scale = (Vec3F32){ 1, 1, 1 },
                                   .ori = (Vec3F32){ 0, 0, 0 },
                                   .angvel = (Vec3F32){ 0, 0, 0 } };

    // Textures
    // --------
    GLuint sample_texture = gl_texture_from_image("assets/textures/Faces for a Dying Land/creep13.png");
    GL(glBindTextureUnit(0, sample_texture));

    // Shaders
    // -------
    const char *u_names[] = { "u_mvp", "u_color", "u_texunit1", "u_light_ambient_color", "u_light_ambient_intensity" };
    GL_Program prg = gl_prg_make("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl", u_names, ARRAY_COUNT(u_names));
    gl_prg_bind(prg.handle);

    // Get Hot Shader Uniforms
    // -----------------------
    GLint u_mvp_loc = gl_prg_get_u_loc_cached(prg, "u_mvp");
    GLint u_light_ambient_intensity_loc = gl_prg_get_u_loc_cached(prg, "u_light_ambient_intensity");

    // Shared Transforms
    // -----------------
    //Mat4F32 view_matrix = mat4f32_trans(mat4f32_identity(), (Vec3F32){ 0, 0, -3 });
    Mat4F32 view_matrix = mat4f32_identity();
    F32 fov_y_rad = deg_to_rad(45);
    F32 aspect = (F32)g_current_window_width / (F32)g_current_window_height;
    Mat4F32 proj_matrix = mat4f32_perspective(fov_y_rad, aspect, 0.1F, 100.0F);

    // Set Shader Uniforms
    // -------------------
    F32 u_color[] = { 1.0F, 0.25F, 1.25F, 1.0F };
    gl_prg_set_1i(prg, "u_texunit1", 0);
    gl_prg_set_3f(prg, "u_light_ambient_color", 1.0F, 1.0F, 1.0F);

    // Create Instance Transforms
    // --------------------------
    Vec3F32 instance_transforms[1000] = { 0 };

    U32 instance_count = ARRAY_COUNT(instance_transforms);
    U32 column_count = instance_count / 30;
    U32 row_count = instance_count / column_count;
    F32 spacing = 0.75F;

    for (U32 row = 0; row < row_count; row++) {
        for (U32 column = 0; column < column_count; column++) {
            U32 index = (row * column_count) + column;
            instance_transforms[index].x = ((F32)column - ((F32)(column_count - 1) * 0.5F)) * spacing;
            instance_transforms[index].y = ((F32)row - ((F32)(row_count - 1) * 0.5F)) * spacing;
            instance_transforms[index].z = 10.0F * sinf32((F32)index);
        }
    }

    // Create Instance Buffer
    // ----------------------
    GL(glBindVertexArray(sample_vao_info.vao)); // make sure the instance buffer will be attached to our VAO

    U32 instance_buffer = { 0 };
    GL(glGenBuffers(1, &instance_buffer));
    GL(glBindBuffer(GL_ARRAY_BUFFER, instance_buffer));
    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(instance_transforms), instance_transforms, GL_STATIC_DRAW));

    GL(glEnableVertexAttribArray(GL_ATTR_LOC_INSTANCE_POSITION)); // NOTE: This assumes there were only two attribute arrays enabled before it
    GL(glVertexAttribPointer(GL_ATTR_LOC_INSTANCE_POSITION,
                             3,
                             GL_FLOAT,
                             GL_FALSE,
                             sizeof(instance_transforms[0]),
                             (void *)(0)));                      // NOLINT(modernize-use-nullptr)
    GL(glVertexAttribDivisor(GL_ATTR_LOC_INSTANCE_POSITION, 1)); // attribute 2 advances onces per instance, not vertex

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

        // Timer
        // -----
        static F32 timer = 0.0F;
        timer -= 0.0066F;

        // MVP
        // ---
        static F32 orbit_radius = 15.0F;
        F32 camx = sinf32(timer) * orbit_radius;
        F32 camz = cosf32(timer) * orbit_radius;

        ASSERT(mat4f32_look_at(&view_matrix, (Vec3F32){ camx, 0, camz }, (Vec3F32){ 0 }, (Vec3F32){ 0, 1, 0 }));
        Mat4F32 u_mvp = calculate_mvp(sample_transform, view_matrix, proj_matrix);
        gl_prg_set_mat4fv_loc(u_mvp_loc, 1, GL_TRUE, &u_mvp);

        // Lighting
        // --------
        gl_prg_set_1f_loc(u_light_ambient_intensity_loc, absf32(sinf32(timer)));

        // Drawing
        // -------
        gl_clear_background(0.1F, 0.1F, 0.1F, 1);
        //gl_vao_draw(sample_vao_info, shader_program, u_color);
        GL(glDrawElementsInstanced(GL_TRIANGLES, sample_vao_info.index_count, GL_UNSIGNED_INT, (void *)(0), instance_count));

        // End of Frame
        // ------------
        SwapBuffers(init_info.window_dc);
    }

    gl_vao_delete(&sample_vao_info);
    win32gl_shutdown(init_info);

    return exit_code;
}
