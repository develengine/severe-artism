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

layout(location = 0) uniform vec2 u_position;
layout(location = 1) uniform vec2 u_size;


void main(void)
{
    vec2 coords = data[gl_VertexID];
    vec2 size = coords * u_size;
    size.y = -size.y;
    vec2 pos = u_position + size;

    o_coords    = coords;
    gl_Position = vec4(pos, 0.0, 1.0);
}
