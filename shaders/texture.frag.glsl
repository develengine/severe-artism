#version 460 core

layout(location = 0) in vec3 o_normals;
layout(location = 1) in vec3 o_position;
layout(location = 2) in vec2 o_textures;
layout(location = 3) in vec3 o_cameraPos;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D textureSampler;

layout(std140, binding = 1) uniform Env
{
    vec4 ambient;
    vec3 toLight;
    vec3 sunColor;
} env;

void main()
{
    vec3 normal = normalize(o_normals);

    vec3 objColor = texture(textureSampler, o_textures).rgb;

    float brightness = max(dot(normal, env.toLight), 0.0) * 0.9;
    vec3 lightColor = env.sunColor * brightness;

    vec3 ambientColor = env.ambient.xyz * env.ambient.w;

    vec3 toCamera = normalize(o_cameraPos - o_position);
    vec3 reflection = normalize(reflect(-env.toLight, normal));
    float shine = pow(max(dot(toCamera, reflection), 0.0), 32);
    vec3 specular = shine * 0.5 * lightColor;

    o_color = vec4(ambientColor + lightColor + specular, 1.0)
            * vec4(objColor, 1.0);
}

