#version 460 core

layout(location = 0) in vec2 o_textures;

layout(location = 0) out vec4 outColor;

layout(location = 5) uniform vec4 u_color;

layout(binding = 0) uniform sampler2D textureSampler;

void main(void)
{
    outColor = texture(textureSampler, o_textures) * u_color;
}
