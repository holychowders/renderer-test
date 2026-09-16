#include "renderer_gl.h"
#include "renderer_gl_platform.h"
#include "platform.h"
#include "renderer.h"

#include "hc_basic.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

////////////////////////////////////////////////////////////////////////// SECTION: MISC

static inline B32 glew_init(void) {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        ferror_re("GL", "Failed to initialize GLEW: %s", (const char *)glewGetErrorString(err));
        return false;
    }
    finfo_re("GL", "Using GLEW version %s", (const char *)glewGetString(GLEW_VERSION));
    return true;
}

// TODO: Renderer API for these options
static inline void gl_init(void) {
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_BLEND));
    GL(glDepthFunc(GL_LESS));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
}

B32 gl_check_errors(void) {
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

void gl_clear_errors(void) {
    while (glGetError() != GL_NO_ERROR) {}
}

////////////////////////////////////////////////////////////////////////// SECTION: VAO (VERTEX ARRAY OBJECT)

/// Create a Vertex Array Object from a vertex and index buffer, which we can then draw
GL_VAOInfo gl_vao_create(const F32 *vb, const U32 *ib, size_t vb_size, size_t ib_size) {
    // Create VAO
    // ----------
    GLuint vao = { 0 };
    GL(glGenVertexArrays(1, &vao));
    GL(glBindVertexArray(vao));

    // Create VBO
    // ----------
    GLuint vbo = { 0 };
    GL(glGenBuffers(1, &vbo));
    GL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL(glBufferData(GL_ARRAY_BUFFER, vb_size, vb, GL_STATIC_DRAW)); // init vertex buffer data

    // Define Vertex Attributes
    // ------------------------
    // Position XYZ
    GL(glVertexAttribPointer(GL_ATTR_LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0)); // NOLINT(modernize-use-nullptr)
    GL(glEnableVertexAttribArray(GL_ATTR_LOC_POSITION));

    // Texcoord UV
    GL(glVertexAttribPointer(GL_ATTR_LOC_TEXCOORD,
                             2,
                             GL_FLOAT,
                             GL_FALSE,
                             5 * sizeof(GLfloat),
                             (void *)(3 * sizeof(GLfloat)))); // NOLINT(modernize-use-nullptr)
    GL(glEnableVertexAttribArray(GL_ATTR_LOC_TEXCOORD));

    // Create IBO
    // ----------
    GLuint ibo = { 0 };
    GL(glGenBuffers(1, &ibo));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib_size, ib, GL_STATIC_DRAW)); // init index buffer data

    GL(glBindVertexArray(0));

    GLsizei index_count = (GLsizei)ib_size / (GLsizei)sizeof(U32);

    GL_VAOInfo vao_info = { vao, vbo, ibo, index_count };

    return vao_info;
}

//void gl_vao_add_instance_buffer(GL_VAOInfo) {
//    GL(glBindVertexArray(sample_vao_info.vao)); // make sure the instance buffer will be attached to our VAO
//
//    U32 instance_buffer = { 0 };
//    GL(glGenBuffers(1, &instance_buffer));
//    GL(glBindBuffer(GL_ARRAY_BUFFER, instance_buffer));
//    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(instance_transforms), instance_transforms, GL_STATIC_DRAW));
//
//    GL(glEnableVertexAttribArray(GL_ATTR_LOC_INSTANCE_POSITION)); // NOTE: This assumes there were only two attribute arrays enabled before it
//    GL(glVertexAttribPointer(GL_ATTR_LOC_INSTANCE_POSITION,
//                             3,
//                             GL_FLOAT,
//                             GL_FALSE,
//                             sizeof(instance_transforms[0]),
//                             (void *)(0)));                      // NOLINT(modernize-use-nullptr)
//    GL(glVertexAttribDivisor(GL_ATTR_LOC_INSTANCE_POSITION, 1)); // attribute advances onces per instance, not vertex
//}

//create a vertex attribute array buffer for defining vertex attributes
//create an vertex element array buffer for selecting elements from an attribute array buffer

void gl_vao_delete(GL_VAOInfo *vao_info) {
    if (!vao_info) {
        warn_re("GL", "Failed to delete VAO (null VAO info provided)");
        return;
    }
    GL(glDeleteVertexArrays(1, &vao_info->vao));
    GL(glDeleteBuffers(1, &vao_info->vbo));
    GL(glDeleteBuffers(1, &vao_info->ibo));
    *vao_info = (GL_VAOInfo){ 0 };
}

static void gl_vao_bind(GL_VAOInfo vao_info) {
    GL(glBindVertexArray(vao_info.vao));
    GL(glBindBuffer(GL_ARRAY_BUFFER, vao_info.vbo));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao_info.ibo));
}

//void gl_vao_draw_instanced(GL_VAOInfo vao_info) {}

void gl_vao_draw(GL_VAOInfo vao_info, GLuint prg, F32 *u_color) {
    //if (!shader_bind(shader.prg)) { return; }

    // FIXME: use the appropriate texture unit for each mesh
    //if (shader.u_locs.contains("u_texunit")) {
    //    GL(glUniform1i(shader.u_locs["u_texunit"], TEXTURE_UNIT_CAT_BASE_COLOR));
    //}

    //glm::mat4 u_mvp = calculate_mvp(transform, fctx.view_matrix, fctx.proj_matrix);
    //GL(glUniformMatrix4fv(shader.u_locs["u_mvp"], 1, GL_FALSE, &u_mvp[0][0]));

    // This should be cached
    //GL(glUniform4f(gl_prg_get_u_loc(prg, "u_color"), u_color[0], u_color[1], u_color[2], u_color[3]));

    gl_vao_bind(vao_info);
    GL(glDrawElements(GL_TRIANGLES, vao_info.index_count, GL_UNSIGNED_INT, NULL));
}

////////////////////////////////////////////////////////////////////////// SECTION: PRG (SHADERS)

// Lower-Level GL Functions
// ------------------------

static inline void gl_prg_delete(GLuint *prg) {
    if (!prg) {
        warn_re("GL", "Failed to delete shader program (null shader program provided)");
        return;
    }
    GL(glDeleteProgram(*prg));
    *prg = 0;
}

static inline B32 gl_prg_verify(GLuint prg) {
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

static inline B32 gl_prg_src_verify(GLuint shader, GLenum shader_type) {
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

/// Return created shader program object. Return 0 on failure.
static inline GLuint gl_prg_create(const char *vs_src, const char *fs_src) {
    // Create Vertex Shader
    // --------------------
    GL(GLuint vs = glCreateShader(GL_VERTEX_SHADER));
    GL(glShaderSource(vs, 1, &vs_src, NULL));
    GL(glCompileShader(vs));
    B32 vs_ok = gl_prg_src_verify(vs, GL_VERTEX_SHADER);

    // Create Fragment Shader
    // ----------------------
    GL(GLuint fs = glCreateShader(GL_FRAGMENT_SHADER));
    GL(glShaderSource(fs, 1, &fs_src, NULL));
    GL(glCompileShader(fs));
    B32 fs_ok = gl_prg_src_verify(fs, GL_FRAGMENT_SHADER);

    // Create Program
    // --------------
    GLuint prg = 0;
    if (vs_ok && fs_ok) {
        GL(prg = glCreateProgram());
        GL(glAttachShader(prg, vs));
        GL(glAttachShader(prg, fs));
        GL(glLinkProgram(prg));
        B32 prg_ok = gl_prg_verify(prg);
        if (!prg_ok) {
            error_re("GL", "Failed to create shader program");
            GL(glDeleteProgram(prg));
        }
    }

    // Delete Intermediate Shader Objects
    // ----------------------------------
    GL(glDeleteShader(vs));
    GL(glDeleteShader(fs));
    vs = 0;
    fs = 0;

    return prg;
}

static inline B32 gl_prg_bind(GLuint prg) {
    if (!prg) {
        error_re("GL", "Failed to bind shader program (null shader program provided)");
        return false;
    }
    GL(glUseProgram(prg));
    return true;
}

static inline void gl_prg_unbind(void) { GL(glUseProgram(0)); }

static inline GLint gl_prg_get_u_loc(GLuint shader_program, const char *name) {
    GL(GLint location = glGetUniformLocation(shader_program, name));
    if (location == -1) { fwarn_re("GL", "Failed to get uniform location: %s", name); }
    return location;
}

// Higher-Level GL Functions
// -------------------------

/// Returns -1 if uniform location not found
static inline GLint gl_prg_get_u_loc_cached(R_ShaderProgram prg, const char *name) {
    for (U32 u_idx = 0; u_idx < prg.u_count; u_idx++) {
        R_ShaderUniform u_loc = prg.u_locs[u_idx];
        if (STR_EQ(u_loc.name, name)) { return u_loc.loc; }
    }
    // TODO: If a uniform were to be added after caching, we should insert it into the cache here
    fwarn("Failed to find shader program uniform in cache: %s\n      Will try to retrieve directly from shader program", name);
    return gl_prg_get_u_loc(prg.handle, name);
}

static inline void gl_prg_set_1i(R_ShaderProgram prg, const char *name, GLint v) {
    GLint u_loc = gl_prg_get_u_loc_cached(prg, name);
    GL(glUniform1i(u_loc, v));
}
static inline void gl_prg_set_1f(R_ShaderProgram prg, const char *name, GLfloat v) {
    GLint u_loc = gl_prg_get_u_loc_cached(prg, name);
    GL(glUniform1f(u_loc, v));
}
static inline void gl_prg_set_3f(R_ShaderProgram prg, const char *name, GLfloat v1, GLfloat v2, GLfloat v3) {
    GLint u_loc = gl_prg_get_u_loc_cached(prg, name);
    GL(glUniform3f(u_loc, v1, v2, v3));
}
/// Upload multiple Vec3F32s
static inline void gl_prg_set_vec3fv(R_ShaderProgram prg, const char *name, GLsizei count, Vec3F32 *vecs) {
    GLint u_loc = gl_prg_get_u_loc_cached(prg, name);
    GL(glUniform3fv(u_loc, count, vecs[0].e));
}
/// Upload multiple Mat4F32s
static inline void gl_prg_set_mat4fv(R_ShaderProgram prg, const char *name, GLsizei count, GLboolean transpose, Mat4F32 *mats) {
    GLint u_loc = gl_prg_get_u_loc_cached(prg, name);
    GL(glUniformMatrix4fv(u_loc, count, transpose, &mats[0].Xx));
}

static inline void gl_prg_set_1i_loc(GLint loc, GLint v) { GL(glUniform1i(loc, v)); }
static inline void gl_prg_set_1f_loc(GLint loc, GLfloat v) { GL(glUniform1f(loc, v)); }
static inline void gl_prg_set_mat4fv_loc(GLint loc, GLsizei count, GLboolean transpose, Mat4F32 *mats) {
    GL(glUniformMatrix4fv(loc, count, transpose, &mats[0].Xx));
}

////////////////////////////////////////////////////////////////////////// SECTION: TEXTURES

/// Return created texture object. Return 0 on failure.
GLuint gl_texture_from_image(const char *fpath) {
    // Load Image from Disk
    // --------------------
    stbi_set_flip_vertically_on_load(true);
    int width = { 0 }, height = { 0 }, channels = { 0 };
    unsigned char *image_data = stbi_load(fpath, &width, &height, &channels, 4);
    if (!image_data) {
        error_re("stbi_load", stbi_failure_reason());
        return 0;
    }

    // Create Texture
    // --------------
    GLuint texture = { 0 };
    GL(glCreateTextures(GL_TEXTURE_2D, 1, &texture));

    // Define Texture Parameters
    // -------------------------
    GL(glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL(glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL(glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL(glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    // Allocate and Upload Image Data to Texture Object
    // ------------------------------------------------
    GL(glTextureStorage2D(texture, 1, GL_RGBA8, width, height));
    GL(glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image_data));

    GL(glGenerateTextureMipmap(texture));

    stbi_image_free(image_data);

    return texture;
}

void gl_texture_delete(GLuint *texture) {
    GL(glDeleteTextures(1, texture));
    *texture = 0;
}

////////////////////////////////////////////////////////////////////////// SECTION: RENDERER API

// Basic
// -----

B32 r_renderer_init(P_PlatformWindow *window) {
    if (!r_gl_platform_context_create(window)) { return false; }
    if (!glew_init()) { return false; }
    gl_init();
    return true;
}

// Vertex Buffer
// -------------

R_VertexBuffer r_vertex_buffer_create(const void *data, size_t size) {
    GLuint handle = 0;
    GL(glGenBuffers(1, &handle));
    GL(glBindBuffer(GL_ARRAY_BUFFER, handle));
    GL(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
    return (R_VertexBuffer){ .handle = handle };
}
void r_vertex_buffer_destroy(R_VertexBuffer *vb) {
    if (!vb->handle) { return; }
    GLuint handle = (GLuint)vb->handle;
    GL(glDeleteBuffers(1, &handle));
    vb->handle = 0;
}
void r_vertex_buffer_bind(R_VertexBuffer vb, B32 bind) {
    if (bind) { GL(glBindBuffer(GL_ARRAY_BUFFER, vb.handle)); }
    else { GL(glBindBuffer(GL_ARRAY_BUFFER, 0)); }
}

// Index Buffer
// ------------

R_IndexBuffer r_index_buffer_create(const U32 *data, U32 count) {
    GLuint handle = 0;
    GL(glGenBuffers(1, &handle));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle));
    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(U32), data, GL_STATIC_DRAW));
    return (R_IndexBuffer){ .handle = handle, .count = count };
}
void r_index_buffer_destroy(R_IndexBuffer *ib) {
    if (!ib->handle) { return; }
    GLuint handle = (GLuint)ib->handle;
    GL(glDeleteBuffers(1, &handle));
    ib->handle = 0;
}
void r_index_buffer_bind(R_IndexBuffer ib, B32 bind) {
    if (bind) { GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.handle)); }
    else { GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)); }
}

// Shaders
// -------

B32 r_shader_program_bind(R_ShaderProgram prg) { return gl_prg_bind(prg.handle); }

R_ShaderProgram r_shader_program_create(const char *vshader_path, const char *fshader_path, const char **uniform_names, U32 uniform_count) {
    R_ShaderProgram prg = { 0 };

    // Load
    char *vs_src = p_platform_load_shader_source(vshader_path);
    char *fs_src = p_platform_load_shader_source(fshader_path);
    if (!vs_src || !fs_src) { return prg; }

    // Build
    prg.handle = gl_prg_create(vs_src, fs_src); // TODO: Pass a collection of shader sources to support different shader types?

    // Free
    free(vs_src);
    free(fs_src);

    // Cache uniform locations
    // TODO: Decide how to handle this. Either make the buffer really large or dynamic.
    ASSERT_MSG(uniform_count <= ARRAY_COUNT(prg.u_locs), "Too many uniforms to store"); // Check default size of uniform store

    for (U32 u_idx = 0; u_idx < uniform_count; u_idx++) {
        finfo("Caching uniform location: %s", uniform_names[u_idx]);
        prg.u_locs[u_idx] = (R_ShaderUniform){ .name = uniform_names[u_idx], .loc = gl_prg_get_u_loc(prg.handle, uniform_names[u_idx]) };
    }
    prg.u_count = uniform_count;

    return prg;
}

// Drawing
// -------

void r_frame_begin(void) {
    // TODO: Timing
}

void r_clear_background(F32 r, F32 g, F32 b, F32 a) {
    //GL(glViewport(0, 0, window_width, window_height));
    GL(glClearColor(r, g, b, a));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void r_renderer_shutdown(void) { r_gl_platform_context_destroy(); }

void r_frame_present(void) { r_gl_platform_present(); }
