#version 330 core 

in vec2 v_tex_uv;
out vec4 color;
uniform sampler2D u_texunit1;

void main() {
    vec4 texel = texture(u_texunit1, v_tex_uv);
    if (texel.a == 0.0) discard;
    color = texel;
}
