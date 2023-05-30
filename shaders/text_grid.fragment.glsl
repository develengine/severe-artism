#version 460 core

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 o_coords;
layout(location = 1) in vec4 o_fg;
layout(location = 2) in vec4 o_bg;

layout(binding = 0) uniform sampler2D u_sampler;

void main(void)
{
    vec4 tex = texture(u_sampler, o_coords);
    outColor = o_fg * tex.a
             + o_bg * (1.0 - tex.a);
}
