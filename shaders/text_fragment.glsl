#version 460 core

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 o_coords;

layout(location = 4) uniform vec4 u_color;

layout(binding = 0) uniform sampler2D u_sampler;

void main(void)
{
    outColor = u_color * texture(u_sampler, o_coords);
}
