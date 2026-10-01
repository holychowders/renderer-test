#version 330 core 

layout (location = 0) in vec3 a_pos_xyz;

void main() {
    gl_Position = vec4(a_pos_xyz, 1.0);
}
