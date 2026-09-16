#pragma once

#include "hc_log.h"
#include "hc_assert.h"
#include "hc_types.h"
#include "hc_mat.h"

#include "renderer.h"

#include <GL/glew.h>

////////////////////////////////////////////////////////////////////////// SECTION: MISC

#define GL(gl_operation)                                                                                                                             \
    gl_clear_errors();                                                                                                                               \
    gl_operation;                                                                                                                                    \
    ASSERT_MSG(!gl_check_errors(), #gl_operation)
B32 gl_check_errors(void);
void gl_clear_errors(void);

////////////////////////////////////////////////////////////////////////// SECTION: VAO (VERTEX ARRAY OBJECT)

/// Use this in GL draw calls
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    GLsizei index_count;
} GL_VAOInfo;

GL_VAOInfo gl_vao_create(const F32 *vb, const U32 *ib, size_t vb_size, size_t ib_size);
void gl_vao_delete(GL_VAOInfo *vao_info);
static void gl_vao_bind(GL_VAOInfo vao_info);
void gl_vao_draw(GL_VAOInfo vao_info, GLuint prg, F32 *u_color);

////////////////////////////////////////////////////////////////////////// SECTION: PRG (SHADERS)

// Higher-Level GL Functions
// -------------------------
//void gl_prg_set_1i(R_ShaderProgram prg, const char *name, GLint v);
//void gl_prg_set_1f(R_ShaderProgram prg, const char *name, GLfloat v);
//void gl_prg_set_3f(R_ShaderProgram prg, const char *name, GLfloat v1, GLfloat v2, GLfloat v3);
//void gl_prg_set_vec3fv(R_ShaderProgram prg, const char *name, GLsizei count, Vec3F32 *vecs);
//void gl_prg_set_mat4fv(R_ShaderProgram prg, const char *name, GLsizei count, GLboolean transpose, Mat4F32 *mats);
//
//void gl_prg_set_1i_loc(GLint loc, GLint v);
//void gl_prg_set_1f_loc(GLint loc, GLfloat v);
//void gl_prg_set_mat4fv_loc(GLint loc, GLsizei count, GLboolean transpose, Mat4F32 *mats);

// Lower-Level GL Abstractions
// ---------------------------

/// Use as locations in shaders
typedef enum {
    // Basic attributes
    GL_ATTR_LOC_POSITION = 0,
    GL_ATTR_LOC_NORMAL = 1,
    GL_ATTR_LOC_TANGENT = 2,
    GL_ATTR_LOC_TEXCOORD = 3,
    GL_ATTR_LOC_COLOR = 4,

    // Instancing
    GL_ATTR_LOC_INSTANCE_POSITION = 5,
    GL_ATTR_LOC_INSTANCE_LIGHT = 6,
} GL_AttributeLocation;

// Lower-Level GL Functions
// ------------------------
//GLuint gl_prg_create(const char *vs_src, const char *fs_src);
//void gl_prg_delete(GLuint *prg);
//B32 gl_prg_src_verify(GLuint shader, GLenum shader_type);
//B32 gl_prg_bind(GLuint prg);
//void gl_prg_unbind(void);
//GLint gl_prg_get_u_loc(GLuint shader_program, const char *name);
//B32 gl_prg_verify(GLuint prg);

////////////////////////////////////////////////////////////////////////// SECTION: TEXTURES

GLuint gl_texture_from_image(const char *fpath);
void gl_texture_delete(GLuint *texture);
