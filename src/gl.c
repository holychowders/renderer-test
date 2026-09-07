#include "gl.h"

#include <GL/glew.h>
//#include <GL/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

////////////////////////////////////////////////////////////////////////// SECTION: MISC

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

void gl_clear_background(F32 r, F32 g, F32 b, F32 a) {
    //GL(glViewport(0, 0, window_width, window_height));
    GL(glClearColor(r, g, b, a));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

B32 gl_glew_init(void) {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        error_re("GL/GLEW", (const char *)glewGetErrorString(err));
        return false;
    }
    finfo_re("Renderer/GL/GLEW", "Version %s", (const char *)glewGetString(GLEW_VERSION));
    return true;
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
    GL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0)); // NOLINT(modernize-use-nullptr)
    // Texcoord UV
    GL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)))); // NOLINT(modernize-use-nullptr)
    GL(glEnableVertexAttribArray(0));
    GL(glEnableVertexAttribArray(1));

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

void gl_vao_draw(GL_VAOInfo vao_info, GLuint prg, F32 *u_color) {
    //if (!shader_bind(shader.prg)) { return; }

    // FIXME: use the appropriate texture unit for each mesh
    //if (shader.ulocs.contains("u_texunit")) {
    //    GL(glUniform1i(shader.ulocs["u_texunit"], TEXTURE_UNIT_CAT_BASE_COLOR));
    //}

    //glm::mat4 u_mvp = calculate_mvp(transform, fctx.view_matrix, fctx.proj_matrix);
    //GL(glUniformMatrix4fv(shader.ulocs["u_mvp"], 1, GL_FALSE, &u_mvp[0][0]));

    // This should be cached
    //GL(glUniform4f(gl_prg_get_uloc(prg, "u_color"), u_color[0], u_color[1], u_color[2], u_color[3]));

    gl_vao_bind(vao_info);
    GL(glDrawElements(GL_TRIANGLES, vao_info.index_count, GL_UNSIGNED_INT, NULL));
}

////////////////////////////////////////////////////////////////////////// SECTION: PRG (SHADERS)

/// Return created shader program object. Return 0 on failure.
GLuint gl_prg_create(const char *vs_src, const char *fs_src) {
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

void gl_prg_delete(GLuint *prg) {
    if (!prg) {
        warn_re("GL", "Failed to delete shader program (null shader program provided)");
        return;
    }
    GL(glDeleteProgram(*prg));
    *prg = 0;
}

B32 gl_prg_verify(GLuint prg) {
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

B32 gl_prg_src_verify(GLuint shader, GLenum shader_type) {
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

B32 gl_prg_bind(GLuint prg) {
    if (!prg) {
        error_re("GL", "Failed to bind shader program (null shader program provided)");
        return false;
    }
    GL(glUseProgram(prg));
    return true;
}

void gl_prg_unbind(void) { GL(glUseProgram(0)); }

GLint gl_prg_get_uloc(GLuint shader_program, const char *name) {
    GL(GLint location = glGetUniformLocation(shader_program, name));
    if (location == -1) { fwarn_re("GL", "Failed to get uniform location: %s", name); }
    return location;
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

GLuint gl_texture_delete(GLuint *texture) {
    GL(glDeleteTextures(1, texture));
    *texture = 0;
}
