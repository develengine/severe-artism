#version 460 core

layout(location = 0) in vec4 i_color_lum;
layout(location = 1) in vec3 i_center;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D smp_position;
layout(binding = 1) uniform sampler2D smp_normal;
layout(binding = 2) uniform sampler2D smp_color_lum;

void main()
{
    vec4 t_position  = texture(smp_position,  gl_FragCoord.xy);
    vec4 t_normal    = texture(smp_normal,    gl_FragCoord.xy);
    vec4 t_color_lum = texture(smp_color_lum, gl_FragCoord.xy);

    vec3 g_position = t_position.xyz;
    vec3 g_normal   = t_normal.xyz;
    vec3 g_albedo   = t_color_lum.xyz;

    float light_lum   = i_color_lum.a;
    vec3  light_color = i_color_lum.rgb;
    vec3  light_pos   = i_center;

    float intensity = max(0.0, dot(g_normal, g_position - light_pos))
                    * max(0.0, light_lum - distance(g_position, light_pos));
                    // FIXME: this is retarded

    o_color = vec4(g_albedo * light_color * intensity, 1.0);
}

