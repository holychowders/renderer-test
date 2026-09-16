#pragma once

#include "platform.h"
#include "hc_types.h"

/////////////////////////////////////////////////////////////////////////// SECTION: BASIC

B32 r_renderer_init(P_PlatformWindow *window);
void r_renderer_shutdown(void);
void r_renderer_basic(void);

/////////////////////////////////////////////////////////////////////////// SECTION: VERTEX BUFFER

typedef struct {
    U32 handle;
} R_VertexBuffer;

R_VertexBuffer r_vertex_buffer_create(const void *data, size_t size);
void r_vertex_buffer_destroy(R_VertexBuffer *vb);
void r_vertex_buffer_bind(R_VertexBuffer vb, B32 bind);

/////////////////////////////////////////////////////////////////////////// SECTION: INDEX BUFFER

typedef struct {
    U32 handle;
    U32 count;
} R_IndexBuffer;

R_IndexBuffer r_index_buffer_create(const U32 *data, U32 count);
void r_index_buffer_destroy(R_IndexBuffer *ib);
void r_index_buffer_bind(R_IndexBuffer ib, B32 bind);

/////////////////////////////////////////////////////////////////////////// SECTION: MESH

typedef struct {
    R_VertexBuffer vb;
    R_IndexBuffer ib;
} R_Mesh;

/////////////////////////////////////////////////////////////////////////// SECTION: SHADERS

typedef struct {
    const char *name;
    S32 loc;
} R_ShaderUniform;

typedef struct {
    U32 handle;
    R_ShaderUniform u_locs[64];
    /// Valid uniforms stored in u_locs
    U32 u_count;
} R_ShaderProgram;

R_ShaderProgram r_shader_program_create(const char *vshader_path, const char *fshader_path, const char **uniform_names, U32 uniform_count);
B32 r_shader_program_bind(R_ShaderProgram prg);

/////////////////////////////////////////////////////////////////////////// SECTION: DRAWING

void r_frame_begin(void);
void r_clear_background(F32 r, F32 g, F32 b, F32 a);
void r_frame_present(void);
