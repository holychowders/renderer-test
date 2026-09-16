#version 330 core 

layout (location = 0) in vec3 a_pos_xyz;
layout (location = 3) in vec2 a_tex_uv;
layout (location = 5) in vec3 a_instance_pos_offset;

uniform mat4 u_mvp;

out vec2 v_tex_uv;

void main() {
    //gl_Position = vec4(a_pos_xyz, 1.0);
    //gl_Position = u_mvp * vec4(a_pos_xyz, 1.0);
    vec3 pos = a_pos_xyz + a_instance_pos_offset;
    gl_Position = u_mvp * vec4(pos, 1.0);
    v_tex_uv = a_tex_uv;
}
