#version 330 core 

in vec2 v_tex_uv;

uniform sampler2D u_texunit1;
uniform vec3 u_light_ambient_color;
uniform float u_light_ambient_intensity;

out vec4 color;

void main() {
    vec4 texel = texture(u_texunit1, v_tex_uv);
    if (texel.a == 0.0) discard;

    vec3 ambient = u_light_ambient_color.rgb * u_light_ambient_intensity;

    color = vec4((ambient*texel.rgb), texel.a);
}
