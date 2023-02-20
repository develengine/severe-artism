#version 460 core

layout(location = 0) in vec2 o_coords;
layout(location = 0) out vec4 out_color;

layout(location = 2) uniform float time;

void main(void)
{
    float x = 2.0 * o_coords.x - 1.0;
    float y = 2.0 * o_coords.y - 1.0;

    float light = 0.5 + 0.5 * sin((x * x + y * y) * 8.0 + time);
    light = round(light);

    out_color = vec4(vec3(light), 1.0);
}
