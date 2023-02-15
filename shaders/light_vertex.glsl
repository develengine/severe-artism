#version 460 core

layout(location = 0) in vec3 i_position;

layout(location = 0) out vec4 o_color_lum;
layout(location = 1) out vec3 o_center;

layout(location = 0) uniform vec4 u_color_lum;
layout(location = 1) uniform vec4 u_center_scale;

layout(std140, binding = 0) uniform Cam
{
    mat4 viewMat;
    mat4 projMat;
    mat4 vpMat;
    vec3 pos;
} cam;

void main()
{
    vec4 position = vec4(i_position * u_center_scale.w + u_center_scale.xyz, 1.0);

    gl_Position = cam.vpMat * position;

    o_color_lum = u_color_lum;;
    o_center    = u_center_scale.xyz;
}

