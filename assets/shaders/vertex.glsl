#version 330 core 

layout (location = 0) in vec3 a_pos_xyz;
layout (location = 1) in vec2 a_tex_uv;
out vec2 v_tex_uv;

void main() {
    gl_Position = vec4(a_pos_xyz, 1.0);
    v_tex_uv = a_tex_uv;
}
