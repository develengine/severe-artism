#version 460 core

const vec2 data[6] = {
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),

    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
};

layout(location = 0) out vec2 o_coords;

layout(location = 0) uniform ivec2 u_position;
layout(location = 1) uniform ivec2 u_size;
layout(location = 2) uniform ivec2 u_screen;


void main(void)
{
    vec2 coords = data[gl_VertexID];
    vec2 pos    = ((vec2(u_position) + coords * u_size) / u_screen) * 2.0 - 1;
    pos.y       = -pos.y;

    o_coords = vec2(coords.x, 1.0 - coords.y);
    gl_Position = vec4(pos, 0.0, 1.0);
}

