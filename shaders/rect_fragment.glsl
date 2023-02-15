#version 460 core

layout(location = 0) out vec4 outColor;

layout(location = 3) uniform vec4 u_color;

void main(void)
{
    outColor = u_color;
}
