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


layout(location = 0) uniform ivec2 u_winRes;
layout(location = 1) uniform ivec2 u_position;
layout(location = 2) uniform ivec2 u_size;
layout(location = 3) uniform ivec2 u_spaces;

layout(location = 5) uniform uvec4 u_text[256];

void main(void)
{
    vec2 box = vec2(1.0 / 32.0, 1.0 / 7.0);
    uint ch = u_text[gl_InstanceID / 16][(gl_InstanceID % 16) / 4];
    uint n = gl_InstanceID % 4;
    uint chr = ((ch << (24 - 8 * n)) >> 24) - 33;

    vec2 coords  = data[gl_VertexID];
    ivec2 offset = (u_size + u_spaces) * ivec2(gl_InstanceID, 0);
    vec2 pos     = ((vec2(u_position) + coords * u_size + offset) / u_winRes) * 2.0 - 1;
    pos.y        = pos.y * -1.0;

    gl_Position = vec4(pos, 1.0, 1.0);

    vec2 c = vec2(chr % 32, chr / 32) * box + vec2(coords.x, coords.y) * box;
    o_coords = vec2(c.x, 1.0 - c.y);
}
