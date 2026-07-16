
#if 0

    #define STB_IMAGE_IMPLEMENTATION
    #include <stb_image.h>

    #include <GL/glew.h>
    #include <GLFW/glfw3.h>
//#include <glm/vec2.hpp>
    #include <glm/gtc/matrix_transform.hpp>

    #include <imgui.h>
    #include <imgui_impl_glfw.h>
    #include <imgui_impl_opengl3.h>

    #include <stdio.h>
    #include <unordered_map>
    #include <string>

    #include "log.hpp"

////////////////////////////////////////////////////////////////////////// Section: TODO

/*
   TODO:
   Renderer:
     - C only
     - Support OpenGL and DirectX
     - Support Windows and Linux (not really; just create separate platform layers for Win32 and Linux, and leave Linux empty for future implementation)
     - Shader asset manager and hotloading
     - Multithreading
     - SIMD
     - Profiling

   Renderer (to consider or low priority):
     - Static + dynamic linking of renderer for hotloading renderer
     - Custom OpenGL function loader?
     - Replace `gl` macro with modern OpenGL logging: glEnable(GL_DEBUG_OUTPUT); glDebugMessageCallback(...); (OpenGL 4.3 in KHR_debug extension)
     - Replace GLM

   Renderer Test Application:
     - Asset manager
     - Level editor
*/

////////////////////////////////////////////////////////////////////////// SECTION: CONSTANTS

static constexpr F32 WINDOW_WIDTH = (int)(2560.0F * 0.75F);
static constexpr F32 WINDOW_HEIGHT = (int)(1440.0F * 0.75F);

static constexpr S32 TARGET_FPS = 120;

static constexpr F32 IMGUI_FONT_SIZE = 30 * 0.60F;

////////////////////////////////////////////////////////////////////////// SECTION: GLOBALS

////////////////////////////////////////////////////////////////////////// SECTION: DATA STRUCTURES

namespace {}

////////////////////////////////////////////////////////////////////////// SECTION: DECLARATIONS

////////////////////////////////////////////////////////////////////////// SECTION: DEFINITIONS

////////////////////////////////////////////////////////////////////////// Section: MACROS






////////////////////////////////////////////////////////////////////////// Section: Data Structures

namespace { // Anonymous namespace to prevent ODR violations and improve LTO (in theory)

    #if 0
struct V3F32 {
    union {
        struct { F32 x, y, z; };
        F32 e[3];
    };
};
M4F32 translate(M4F32 mat, V3F32& tvec);
    #endif

enum class VertexFormat : U8 { xyz_uv_rgba, xyz_n_uv /* pos: xyz, normals: xyz, texcoords: uv */, xyz };

// TODO: Look into whether or not we should separate the pure geometry data (vb, vb_size, etc), and render data (vao, vbo, ibo, etc).
//       Consider a struct Model with Mesh and RenderData.
struct RenderMesh {
    // GL render data
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    GLsizei index_count;

    // Geometry data
    F32 *vb;
    size_t vb_size;

    // Other
    VertexFormat vertex_format;
    //S32 texunit; // Which texture unit to use when rendering
};

struct Transform {
    glm::vec3 pos;
    glm::vec3 scale;
    glm::vec3 ori;
    glm::vec3 angvel;
};

//struct MeshRenderData {
//    RenderMesh &mesh;
//    Transform &transform;
//};

struct ShaderData {
    GLuint prg;
    std::unordered_map<std::string, GLint> ulocs;
};

struct FrameContext {
    GLFWwindow *window;
    double &dt_s;
    //ImGuiIO &imgui_io;
    ShaderData &shader_xyz_uv_rgba;
    ShaderData &shader_xyz_n_uv;
    glm::mat4 view_matrix;
    glm::mat4 proj_matrix;
};

}

////////////////////////////////////////////////////////////////////////// SECTION: ERROR CHECKING

static void clear_gl_errors() {
    while (glGetError() != GL_NO_ERROR) {}
}

static bool check_gl_errors() {
    bool has_error = false;
    GLenum gl_error = {};
    while ((gl_error = glGetError()) != GL_NO_ERROR) {
        has_error = true;
        const char *message = "";
        switch (gl_error) {
            case GL_INVALID_ENUM: message = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: message = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: message = "GL_INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: message = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY: message = "GL_OUT_OF_MEMORY"; break;
            case GL_STACK_UNDERFLOW: message = "GL_STACK_UNDERFLOW"; break;
            case GL_STACK_OVERFLOW: message = "GL_STACK_OVERFLOW"; break;
            default: {
                char fmsg[128];
                snprintf(fmsg, sizeof(fmsg), "Unknown error: 0x%x", gl_error);
                message = fmsg;
            } break;
        }
        error("OpenGL", message);
    }
    return has_error;
}

////////////////////////////////////////////////////////////////////////// Section: GLEW

static bool glew_init() {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        error("GLEW", (const char *)glewGetErrorString(err));
        return false;
    }
    finfo("GLEW", "Using GLEW version %s", (const char *)glewGetString(GLEW_VERSION));
    return true;
}

////////////////////////////////////////////////////////////////////////// Section: GLFW

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) { // NOLINT(misc-unused-parameters)
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window, true); }
    }
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_EQUAL) {}
        else if (key == GLFW_KEY_MINUS) {}
    }
}

static void glfw_error_callback(int error_code, const char *description) {
    char re[32];
    snprintf(re, sizeof(re), "GLFW: code %d", error_code);
    error(re, description);
}

static GLFWwindow *glfw_init(int window_width, int window_height, const char *window_title) {
    // Init
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) { return nullptr; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    // Create window
    GLFWwindow *window = glfwCreateWindow(window_width, window_height, window_title, nullptr, nullptr);
    if (window) {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // vsync
        glfwSetKeyCallback(window, glfw_key_callback);
    }
    else {
        error("GLFW", "Failed to initialize\n");
        glfwTerminate();
    }

    return window;
}

static void glfw_update(GLFWwindow *window) {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

////////////////////////////////////////////////////////////////////////// Section: Misc

static void clear_background(F32 r, F32 g, F32 b, F32 a) {
    gl(glClearColor(r, g, b, a));
    gl(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

static void update_vertex_buffer(GLuint vbo, F32 *vb, size_t vb_size) {
    gl(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    gl(glBufferSubData(GL_ARRAY_BUFFER, 0, vb_size, vb));
}

static glm::mat4 calculate_mvp(const Transform &transform, const glm::mat4 &view, const glm::mat4 &projection) {
    glm::mat4 model = glm::mat4(1.0F);
    model = glm::translate(model, transform.pos);

    model = glm::rotate(model, transform.ori.x, glm::vec3(1.0F, 0.F, 0.0F));
    model = glm::rotate(model, transform.ori.y, glm::vec3(0.0F, 1.F, 0.0F));
    model = glm::rotate(model, transform.ori.z, glm::vec3(0.0F, 0.F, 1.0F));

    model = glm::scale(model, transform.scale);

    return projection * view * model;
}

////////////////////////////////////////////////////////////////////////// Section: Shaders

//#define strfmt(str, fmt) ({ \
//                           \
//})

//static char* strfmt(

static bool shader_bind(GLuint prg) {
    if (!prg) {
        error("Failed to bind shader program (null shader program provided)", __FILE__, __LINE__);
        return false;
    }
    gl(glUseProgram(prg));
    return true;
}

static void shader_unbind() {
    gl(glUseProgram(0));
}

static GLint shader_get_uniform_location(GLuint shader_program, const char *name) {
    gl(GLint location = glGetUniformLocation(shader_program, name));
    if (location == -1) { fwarn(nullptr, "Failed to get uniform location: %s", name); }
    return location;
}

static bool shader_verify(GLuint shader, GLenum shader_type) {
    GLint compile_success = GL_FALSE;
    gl(glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_success));
    if (!compile_success) {
        GLint log_len{};
        gl(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len));

        char log_message[2048];
        gl(glGetShaderInfoLog(shader, log_len, &log_len, log_message));

        const char *shader_type_str = "vertex";
        if (shader_type == GL_VERTEX_SHADER) { shader_type_str = "vertex"; }
        else if (shader_type == GL_FRAGMENT_SHADER) { shader_type_str = "fragment"; }

        char fmsg[sizeof(log_message) + 128];
        snprintf(fmsg, sizeof(fmsg), "Failed to compile %s shader\n%s", shader_type_str, log_message);
        error(fmsg);
        return false;
    }
    return true;
}

static bool shader_program_verify(GLuint prg) {
    // Check Link Status
    GLint link_success = GL_FALSE;
    gl(glGetProgramiv(prg, GL_LINK_STATUS, &link_success));
    if (!link_success) {
        char log_message[2048]{};
        gl(glGetProgramInfoLog(prg, sizeof(log_message), nullptr, log_message));
        if (log_message[0]) { error("shader program link", log_message); }
        else { error("Failed to link shader program"); }
    }

    GLint validate_success = GL_FALSE;
    if (link_success) {
        // Check Validation Status
        gl(glValidateProgram(prg));
        gl(glGetProgramiv(prg, GL_VALIDATE_STATUS, &validate_success));
        if (!validate_success) {
            char log_message[2048]{};
            gl(glGetProgramInfoLog(prg, sizeof(log_message), nullptr, log_message));
            if (log_message[0]) { error("shader program validation", log_message); }
            else { error("Failed to validate shader program"); }
        }
    }

    return (validate_success && link_success);
}

// TODO: Verify we're binding/unbinding the program properly

/// Returns created shader program object. Returns 0 on failure.
static GLuint shader_program_create(const char *vs_src, const char *fs_src) {
    assert(vs_src);
    assert(fs_src);

    // Vertex Shader
    gl(GLuint vs = glCreateShader(GL_VERTEX_SHADER));
    gl(glShaderSource(vs, 1, &vs_src, nullptr));
    gl(glCompileShader(vs));
    bool vs_ok = shader_verify(vs, GL_VERTEX_SHADER);

    // Fragment Shader
    gl(GLuint fs = glCreateShader(GL_FRAGMENT_SHADER));
    gl(glShaderSource(fs, 1, &fs_src, nullptr));
    gl(glCompileShader(fs));
    bool fs_ok = shader_verify(fs, GL_FRAGMENT_SHADER);

    // Program
    GLuint prg = 0;
    if (vs_ok && fs_ok) {
        gl(prg = glCreateProgram());
        gl(glAttachShader(prg, vs));
        gl(glAttachShader(prg, fs));
        gl(glLinkProgram(prg));
        bool prg_ok = shader_program_verify(prg);
        if (!prg_ok) {
            error("Failed to create shader program");
            gl(glDeleteProgram(prg));
        }
    }

    // Delete Intermediate Shader Objects
    gl(glDeleteShader(vs));
    gl(glDeleteShader(fs));
    vs = 0;
    fs = 0;

    return prg;
}

////////////////////////////////////////////////////////////////////////// Section: Texture

static GLuint texture_create_and_upload_from_rgba(U32 texture_unit_index, const U32 rgba) {
    U8 color[] = { (U8)((rgba >> 24) & 0xFF), (U8)((rgba >> 16) & 0xFF), (U8)((rgba >> 8) & 0xFF), (U8)((rgba >> 0) & 0xFF) };

    GLuint texture{};
    gl(glCreateTextures(GL_TEXTURE_2D, 1, &texture));

    gl(glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR)); // Linearly resample on minification (will not snap to pixel)
    gl(glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR)); // Linearly resample on magnification (stretch to fill)

    gl(glTextureStorage2D(texture, 1, GL_RGBA8, 1, 1));
    gl(glTextureSubImage2D(texture, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color));

    gl(glBindTextureUnit(texture_unit_index, texture));

    return texture;
}

// TODO: Image format arg?
static GLuint texture_create_and_upload_from_image(U32 texture_unit_index, const char *fpath) {
    // Load image
    stbi_set_flip_vertically_on_load(true);
    int width{}, height{}, channels{};
    unsigned char *idata = stbi_load(fpath, &width, &height, &channels, 4);
    if (!idata) {
        error("stbi_load", stbi_failure_reason());
        return 0;
    }

    GLuint texture{};
    gl(glCreateTextures(GL_TEXTURE_2D, 1, &texture));

    gl(glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    gl(glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    gl(glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    gl(glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    gl(glTextureStorage2D(texture, 1, GL_RGBA8, width, height));
    gl(glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, idata));

    gl(glBindTextureUnit(texture_unit_index, texture));

    if (idata) { stbi_image_free(idata); }

    return texture;
}

// Takes PNG/JPG-encoded bytes, not raw image RGBA bytes
static GLuint texture_create_and_upload_from_image(U32 texture_unit_index, const size_t image_size, const UChar *image_bytes) {
    // Load image
    //stbi_set_flip_vertically_on_load(true);
    int width{}, height{}, channels{};
    unsigned char *idata = stbi_load_from_memory(image_bytes, (S32)image_size, &width, &height, &channels, 4);
    if (!idata) {
        error("stbi_load_from_memory", stbi_failure_reason());
        return 0;
    }

    // Create texture object
    GLuint texture{};
    gl(glCreateTextures(GL_TEXTURE_2D, 1, &texture));

    // Allocate and upload image data to texture object
    GLsizei mipmap_levels = 1;                                                                  // 1 + (GLsizei)floor(log2(fmax(width, height)));
    gl(glTextureStorage2D(texture, mipmap_levels, GL_RGBA8, width, height));                    // Allocate
    gl(glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, idata)); // Upload
    //gl(glGenerateTextureMipmap(texture));

    // Set texture parameters
    gl(glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR));    // Linearly resample on minification (will not snap to pixel)
    gl(glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR));    // Linearly resample on magnification (stretch to fill)
    gl(glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)); // Horizonal wrap behavior: clamp, don't wrap
    gl(glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)); // Vertical wrap behavior: clamp, don't wrap

    // Bind texture object and select it into texture unit
    gl(glBindTextureUnit(texture_unit_index, texture));

    // Cleanup
    if (idata) { stbi_image_free(idata); }

    return texture;
}

    #if 0
static void texture_bind(GLint uloc_texture_unit_index, GLuint texture_unit_index, GLuint texture_object) {
    gl(glBindTextureUnit(texture_unit_index, texture_object));
    gl(glUniform1i(uloc_texture_unit_index, texture_unit_index));
}
    #endif

////////////////////////////////////////////////////////////////////////// Section: Mesh

static bool rendermesh_bind(RenderMesh &rmesh) {
    if (rmesh.vao && rmesh.vbo && rmesh.ibo) {
        gl(glBindVertexArray(rmesh.vao));
        gl(glBindBuffer(GL_ARRAY_BUFFER, rmesh.vbo));
        gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rmesh.ibo));
        return true;
    }
    else {
        char regarding[256];
        char message[256];
        snprintf(regarding, sizeof(regarding), "%s:%d", __FILE__, __LINE__);
        snprintf(message, sizeof(message), "Failed to bind render mesh (vao:%u, vbo:%u, ibo:%u)", rmesh.vao, rmesh.vbo, rmesh.ibo);
        error(regarding, message);
        return false;
    }
}

static void rendermesh_draw(FrameContext &fctx, ShaderData &shader, RenderMesh &rmesh, Transform &transform) {
    if (!shader_bind(shader.prg)) { return; }
    glm::mat4 u_mvp = calculate_mvp(transform, fctx.view_matrix, fctx.proj_matrix);
    if (shader.ulocs.contains("u_texunit")) {
        gl(glUniform1i(shader.ulocs["u_texunit"], TEXTURE_UNIT_CAT_BASE_COLOR));
    } // FIXME: use the appropriate texture unit for each mesh
    gl(glUniformMatrix4fv(shader.ulocs["u_mvp"], 1, GL_FALSE, &u_mvp[0][0]));
    rendermesh_bind(rmesh);
    gl(glDrawElements(GL_LINES, rmesh.index_count, GL_UNSIGNED_INT, nullptr));
}

static RenderMesh rendermesh_create(VertexFormat vfmt, F32 *vb, U32 *ib, size_t vb_size, size_t ib_size) {
    GLuint vao{};
    gl(glGenVertexArrays(1, &vao));
    gl(glBindVertexArray(vao));

    GLuint vbo{};
    gl(glGenBuffers(1, &vbo));
    gl(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    gl(glBufferData(GL_ARRAY_BUFFER, vb_size, vb, GL_STATIC_DRAW));

    switch (vfmt) {
        case VertexFormat::xyz_uv_rgba: {
            size_t stride = 9;
            gl(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)0)); // NOLINT(modernize-use-nullptr)
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            gl(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat))));
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            gl(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(5 * sizeof(GLfloat))));

            gl(glEnableVertexAttribArray(0));
            gl(glEnableVertexAttribArray(1));
            gl(glEnableVertexAttribArray(2));
        } break;
        case VertexFormat::xyz: {
            size_t stride = 3;
            gl(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)0)); // NOLINT(modernize-use-nullptr)
            gl(glEnableVertexAttribArray(0));
        } break;
        case VertexFormat::xyz_n_uv: {
            size_t stride = 8;
            gl(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)0)); // NOLINT(modernize-use-nullptr)
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            gl(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat))));
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            gl(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(6 * sizeof(GLfloat))));

            gl(glEnableVertexAttribArray(0));
            gl(glEnableVertexAttribArray(1));
            gl(glEnableVertexAttribArray(2));
        } break;
        default: {
            error(__FUNCTION__, "Passed an unhandled vertex format");
        } break;
    }

    GLuint ibo{};
    gl(glGenBuffers(1, &ibo));
    gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    gl(glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib_size, ib, GL_STATIC_DRAW)); // init index buffer data

    // Clean Up (Unbind)
    gl(glBindVertexArray(0)); // unbind this global VAO (only one VAO is active at a time)

    RenderMesh rmesh{ vao, vbo, ibo, 0, vb, vb_size, vfmt };
    size_t index_count = ib_size / sizeof(ib[0]);
    assert(index_count <= (size_t)INT_MAX);
    rmesh.index_count = (GLsizei)index_count;

    return rmesh;
}

////////////////////////////////////////////////////////////////////////// Section: ImGui

static ImGuiIO &imgui_init(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::GetStyle().FontSizeBase = IMGUI_FONT_SIZE;
    ImGui::GetStyle().ScaleAllSizes(1);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();

    return io;
}

/// Create window and begin frame
static void imgui_start(const char *title) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(title);
}

static void imgui_end() {
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static void imgui_framerate(ImGuiIO &imgui_io) {
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0 / (double)imgui_io.Framerate, (double)imgui_io.Framerate);
}

static void imgui_section(const char *name) {
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::Text("%s", name);
}

static void imgui_render(ImGuiIO &imgui_io, Transform &cube_transform) {
    imgui_start("Debug Menu");
    imgui_framerate(imgui_io);
    {
        imgui_section("Cube");
        ImGui::DragFloat3("Translation##cube", &cube_transform.pos.x, 1);
        ImGui::DragFloat3("Angular Velocity##cube", &cube_transform.angvel.x, 0.005F);
        ImGui::DragFloat3("Orientation##cube", &cube_transform.ori.x, 0.005F);
    }
    imgui_end();
}

////////////////////////////////////////////////////////////////////////// Section: Main

static std::vector<RenderMesh> load_glb_and_create_rmeshes(const char *glb_path) { }

static void update(FrameContext &fctx, Transform &asset_tform, Transform &tbg, Transform &tcube) {
    float dt_s = (F32)fctx.dt_s;
    asset_tform.ori += dt_s * asset_tform.angvel;
    tbg.ori += dt_s * tbg.angvel;
    tcube.ori += dt_s * tcube.angvel;
}

static void render(FrameContext &fctx,
                   std::vector<RenderMesh> &cat_model,
                   RenderMesh &mbg,
                   RenderMesh &mcube,
                   Transform &tasset,
                   Transform &tbg,
                   Transform &tcube) {
    clear_background(0.1F, 0.1F, 0.1F, 0.1F);
    //rendermesh_draw(fctx, fctx.shader_xyz_uv_rgba, mbg, tbg);

    if (g_cat_model_mesh_idx == SIZE_MAX) { g_cat_model_mesh_idx = 0; }
    if (g_cat_model_mesh_idx >= cat_model.size()) { g_cat_model_mesh_idx = cat_model.size() - 1; }
    //rendermesh_draw(fctx, fctx.shader_xyz_n_uv, cat_model[g_cat_model_mesh_idx], tasset);
    for (RenderMesh rmesh : cat_model) {
        rendermesh_draw(fctx, fctx.shader_xyz_n_uv, rmesh, tasset);
    }

    //rendermesh_draw(fctx, mcube, tcube);
}

int main() {
    GLFWwindow *window = glfw_init((int)WINDOW_WIDTH, (int)WINDOW_HEIGHT, "OpenGL 3D Test");
    if (!window) { return -1; }
    if (!glew_init()) {
        glfwTerminate();
        return -1;
    }
    ImGuiIO &imgui_io = imgui_init(window);

    gl(glEnable(GL_DEPTH_TEST));
    gl(glEnable(GL_BLEND));
    gl(glDepthFunc(GL_LESS));
    gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // Section: Load Asset Files
    //GLB_Model model = load_glb_and_create_rmeshes("assets/behemot_cat.glb");
    std::vector<RenderMesh> cat_model = load_glb_and_create_rmeshes("assets/behemot_cat.glb");
    Transform asset_transform{ .pos{ 0.35F * WINDOW_WIDTH, 0.15F * WINDOW_HEIGHT, 0.F },
                               .scale = glm::vec3(9),
                               .ori = glm::vec3(0),
                               .angvel = { 0, 1, 0 } };

    // Section: Vertex and Index Buffers
    // clang-format off
    F32 bg_quad_vb[] = { 
    //             x    y               z     u  v    r  g  b  a
                 -000, 000,            000,   0, 0,   1, 0, 0, 1, // bot left
         WINDOW_WIDTH, 000,            000,   1, 0,   0, 1, 0, 1, // bot right
         WINDOW_WIDTH, WINDOW_HEIGHT,  000,   0, 1,   0, 0, 1, 1, // top right
                 -000, WINDOW_HEIGHT,  000,   1, 1,   1, 1, 1, 1, // top left
    };
    U32 bg_quad_ib[] = { 0, 1, 2, 2, 3, 0 };

    F32 cube_vb[] = {
        // Back face
        -0.5, -0.5, -0.5,  0, 0,  0.15F, 0.15F, 0.15F, 1,
         0.5, -0.5, -0.5,  1, 0,  0.15F, 0.15F, 0.15F, 1,
         0.5,  0.5, -0.5,  0, 1,  0.15F, 0.15F, 0.15F, 1,
        -0.5,  0.5, -0.5,  1, 1,  0.15F, 0.15F, 0.15F, 1,
        // Front Face
        -0.5, -0.5,  0.5,  0, 0,  0, 0, 0, 1,
         0.5, -0.5,  0.5,  1, 0,  0, 0, 0, 1,
         0.5,  0.5,  0.5,  0, 1,  0, 0, 0, 1,
        -0.5,  0.5,  0.5,  1, 1,  0, 0, 0, 1,
    };
   U32 cube_ib[] = {
        0,1,2, 2,3,0, // back quad
        4,5,6, 6,7,4, // front quad
        4,0,3, 3,7,4, // left quad
        1,5,6, 6,2,1, // right quad
        3,2,6, 6,7,3, // top quad
        4,5,1, 1,0,4, // bottom quad
    };
    // clang-format on

    // Section: Meshes
    RenderMesh bg_mesh = rendermesh_create(VertexFormat::xyz_uv_rgba, bg_quad_vb, bg_quad_ib, sizeof(bg_quad_vb), sizeof(bg_quad_ib));
    RenderMesh cube_mesh = rendermesh_create(VertexFormat::xyz_uv_rgba, cube_vb, cube_ib, sizeof(cube_vb), sizeof(cube_ib));

    // Section: Mesh Transforms
    Transform bg_transform{ .pos{ 1, 1, -500 }, .scale = glm::vec3(1), .ori = glm::vec3(0), .angvel = glm::vec3(0) };
    Transform cube_transform{ .pos{ 0.25F * WINDOW_WIDTH, 0.5F * WINDOW_HEIGHT, 0.F },
                              .scale{ 500, 500, 500 },
                              .ori{ 0.2, -0.4, 0 },
                              .angvel{ 0, 0.4, 0 } };

    // Section: Shader Program
    GLuint prg{};
    // XYZ UV RGBA
    prg = shader_program_create(shader_sources::vs_xyz_uv_rgba, shader_sources::fs_xyz_uv_rgba);
    ShaderData shader_xyz_uv_rgba = { prg, { { "u_mvp", shader_get_uniform_location(prg, "u_mvp") } } };
    // XYZ N UV
    prg = shader_program_create(shader_sources::vs_xyz_n_uv, shader_sources::fs_xyz_n_uv);
    ShaderData shader_xyz_n_uv = { prg,
                                   { { "u_mvp", shader_get_uniform_location(prg, "u_mvp") },
                                     { "u_texunit", shader_get_uniform_location(prg, "u_texunit") } } };

    // Set default textures
    texture_create_and_upload_from_rgba(TEXTURE_UNIT_DEBUG, 0xFF00FF);

    // Section: Shared Transforms
    const glm::mat4 view_matrix(1);
    const glm::mat4 proj_matrix = glm::ortho(0.F, WINDOW_WIDTH, 0.F, WINDOW_HEIGHT, -1000.F, 1000.F);

    // Section: Frame Setup
    //MeshRenderData bg_mesh_rd{ bg_mesh, bg_quad_vb, sizeof(bg_quad_vb), bg_transform };
    //MeshRenderData cube_mesh_rd{ cube_mesh, cube_vb, sizeof(cube_vb), cube_transform };
    double t_now_s{}, t_last_s{}, dt_s{};
    FrameContext frame_ctx = { window, dt_s, shader_xyz_uv_rgba, shader_xyz_n_uv, view_matrix, proj_matrix };
    while (!glfwWindowShouldClose(window)) {
        t_now_s = glfwGetTime();
        dt_s = t_now_s - t_last_s;
        t_last_s = t_now_s;
        update(frame_ctx, asset_transform, bg_transform, cube_transform);
        render(frame_ctx, cat_model, bg_mesh, cube_mesh, asset_transform, bg_transform, cube_transform);
        imgui_render(imgui_io, cube_transform);
        glfw_update(window);
    }
    glfwTerminate();
}

////////////////////////////////////////////////////////////////////////// Section: Garbage

    #if 0
namespace {

struct VertexData {
    F32 xyz[3];
    F32 uv[2];
    F32 rgba[4];
};

}

static VertexBufferData gen_quad_vbuffer(F32 *out_vbuffer, VertexData &vbottom_left, U32 size) {
    F32 *xyz = vbottom_left.xyz;
    F32 *uv = vbottom_left.uv;
    F32 *rgba = vbottom_left.rgba;

    F32 vb[] = { *xyz, *uv, *rgba };
}

static F32* flatten_vertex_buffer_data(VertexBufferData& vertex_buffer);

//
VertexData vbottomleft = { .xyz{ -100, 100, -100 }, .uv{ 0, 1 }, .rgba{ 1, 0, 1 } };
VertexBufferData vbuffer = gen_quad_vbuffer(vbuffer, vbottomleft, 200);
vb.flatten();
    #endif

#endif
