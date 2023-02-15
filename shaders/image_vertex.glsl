#version 460 core

const vec2 data[6] = {
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),

    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
};

layout(location = 0) out vec2 o_textures;

layout(location = 0) uniform ivec2 u_winRes;
layout(location = 1) uniform ivec2 u_position;
layout(location = 2) uniform ivec2 u_size;
layout(location = 3) uniform vec2 u_texOff;
layout(location = 4) uniform vec2 u_texSize;


void main(void)
{
    vec2 coords = data[gl_VertexID];
    vec2 pos    = ((vec2(u_position) + coords * u_size) / u_winRes) * 2.0 - 1;
    pos.y       = pos.y * -1.0;

    vec2 tex   = u_texOff + coords * u_texSize;
    o_textures = vec2(tex.x, 1.0 - tex.y);
    gl_Position = vec4(pos, 1.0, 1.0);
}
