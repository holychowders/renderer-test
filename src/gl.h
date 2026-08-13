#pragma once

#include "hc_log.h"
#include "hc_assert.h"
#include "hc_types.h"

#include <GL/glew.h>
//#include <GL/gl.h>

////////////////////////////////////////////////////////////////////////// SECTION: MISC

#define GL(gl_operation)                                                                                                                             \
    gl_clear_errors();                                                                                                                               \
    gl_operation;                                                                                                                                    \
    ASSERT_MSG(!gl_check_errors(), #gl_operation)
B32 gl_check_errors(void);
void gl_clear_errors(void);

void gl_clear_background(F32 r, F32 g, F32 b, F32 a);

B32 gl_glew_init(void);

////////////////////////////////////////////////////////////////////////// SECTION: VAO (VERTEX ARRAY OBJECT)

/// Use this in GL draw calls
typedef struct GL_VAOInfo {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    GLsizei index_count;
} GL_VAOInfo;

GL_VAOInfo gl_vao_create(F32 *vb, U32 *ib, size_t vb_size, size_t ib_size);
void gl_vao_bind(GL_VAOInfo vao_info);
void gl_vao_draw(GL_VAOInfo vao_info);

////////////////////////////////////////////////////////////////////////// SECTION: PRG (SHADERS)

GLuint gl_prg_create(const char *vs_src, const char *fs_src);
B32 gl_prg_src_verify(GLuint shader, GLenum shader_type);
B32 gl_prg_bind(GLuint prg);
void gl_prg_unbind(void);
GLint gl_prg_get_uniform_location(GLuint shader_program, const char *name);
B32 gl_prg_verify(GLuint prg);
