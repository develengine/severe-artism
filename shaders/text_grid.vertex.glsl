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
layout(location = 1) out vec4 o_fg;
layout(location = 2) out vec4 o_bg;

layout(location = 0) uniform ivec2 u_winRes;
layout(location = 1) uniform ivec2 u_position;
layout(location = 2) uniform ivec2 u_size;
layout(location = 3) uniform ivec2 u_resolution;

layout(std430, binding = 0) buffer Text
{
    uint data[];
} text;

layout(std430, binding = 1) buffer Color
{
    uvec2 data[];
} color;

void main(void)
{
    uint ch = text.data[gl_InstanceID / 4];
    uint n = gl_InstanceID % 4;
    uint chr = ((ch << (24 - 8 * n)) >> 24) - 33;

    vec2 coords  = data[gl_VertexID];
    ivec2 offset = u_size * ivec2(gl_InstanceID % u_resolution.x,
                                  gl_InstanceID / u_resolution.x);

    vec2 pos = ((vec2(u_position) + coords * u_size + offset) / u_winRes) * 2.0 - 1;
    pos.y = pos.y * -1.0;

    gl_Position = vec4(pos, 1.0, 1.0);

    vec2 box = vec2(1.0 / 32.0, 1.0 / 7.0);
    vec2 c = vec2(chr % 32, chr / 32) * box + vec2(coords.x, coords.y) * box;
    o_coords = vec2(c.x, 1.0 - c.y);

    uvec2 colors = color.data[gl_InstanceID];
    o_fg = unpackUnorm4x8(colors.x);
    o_bg = unpackUnorm4x8(colors.y);
}
