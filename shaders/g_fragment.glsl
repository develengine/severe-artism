#version 460 core

layout(location = 0) in vec3 i_normals;
layout(location = 1) in vec3 i_position;
layout(location = 2) in vec2 i_textures;

layout(location = 0) out vec4 o_position;
layout(location = 1) out vec4 o_normal;
layout(location = 2) out vec4 o_color_lum;

layout(binding = 0) uniform sampler2D textureSampler;

void main()
{
    o_position  = vec4(i_position, 1.0);
    o_normal    = vec4(normalize(i_normals), 0.0);
    o_color_lum = texture(textureSampler, i_textures);
}

