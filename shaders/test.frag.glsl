#version 460 core

layout(location = 0) in vec2 o_coords;
layout(location = 0) out vec4 out_color;

layout(location = 0) uniform ivec2 u_position;
layout(location = 1) uniform ivec2 u_size;
layout(location = 2) uniform ivec2 u_screen;
layout(location = 3) uniform float u_time;

void main(void)
{
    float x = 2.0 * o_coords.x - 1.0;
    float y = 2.0 * o_coords.y - 1.0;
    y *= float(u_screen.y) / float(u_screen.x);

    vec2 p = vec2(x, y);
    float t = u_time;

    float bop = sin(t * 2.0) * 0.125;
    // float bop = 0.0f;

    float width = 0.5;

    vec2 e1 = p * 2.0 - vec2(-0.5, 0.5 + bop);
    vec2 e2 = p * 2.0 - vec2( 0.5, 0.5 - bop);
    vec2 e3 = p * 2.0;

    float l1 = sqrt(e1.x * e1.x + e1.y * e1.y);
    float l2 = sqrt(e2.x * e2.x + e2.y * e2.y);
    float l3 = sqrt(e3.x * e3.x + e3.y * e3.y);

    if (dot(e3, vec2(sin(bop * 2.0), cos(bop * 2.0))) > 0.0)
        l3 += width * 2.0;

    float light = min(l1, min(l2, l3 * 0.5));

    if (light < width) {
        light = 1.0 - light;
    } else {
        light = sin(1.0 / (light * light * light * light) + t);
    }

    out_color = vec4(vec3(light), 1.0);
}

