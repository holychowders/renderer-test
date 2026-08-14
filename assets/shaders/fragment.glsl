#version 330 core 

in vec2 v_tex_uv;
out vec4 color;
uniform sampler2D u_texunit1;

void main() {
    //color = vec4(1.0f, 0.25f, 0.25f, 1.0f);
    //color = u_color;
    color = texture(u_texunit1, v_tex_uv);
    //color = mix(texture(u_texunit1, v_tex_uv), texture(u_texunit2, v_tex_uv), 0.2);
}
