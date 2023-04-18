#version 460 core

#define EPS 0.001
#define END 100.0

layout(location = 0) in vec2 o_coords;
layout(location = 0) out vec4 out_color;

layout(location = 0) uniform ivec2 u_position;
layout(location = 1) uniform ivec2 u_size;
layout(location = 2) uniform ivec2 u_screen;
layout(location = 3) uniform float u_time;

layout(location = 4) uniform float u_fov;
layout(location = 5) uniform vec3  u_eye;


float sphere_sdf(vec3 c, float r, vec3 x)
{
    return length(c - x) - r;
}

float cube_sdf(vec3 c, vec3 s, vec3 x)
{
    vec3 v = abs(c - x) - s;
    return max(v.x, max(v.y, v.z));
}

float sdf_one(vec3 x)
{
    vec3 center = vec3(0.0, 0.0, 4.0);
    return max(cube_sdf(center, vec3(1.0), x),
             sphere_sdf(center, 1.25,      x));
}

float sdf(vec3 x)
{
    return sdf_one(mod(x + vec3(5.0), 10.0) - vec3(5.0));
}

vec3 get_normal(vec3 p)
{
    return normalize(vec3(
        sdf(vec3(p.x + EPS, p.y, p.z)) - sdf(vec3(p.x - EPS, p.y, p.z)),
        sdf(vec3(p.x, p.y + EPS, p.z)) - sdf(vec3(p.x, p.y - EPS, p.z)),
        sdf(vec3(p.x, p.y, p.z + EPS)) - sdf(vec3(p.x, p.y, p.z - EPS))
    ));
}

void main(void)
{
    float x = 2.0 * o_coords.x - 1.0;
    float y = 2.0 * o_coords.y - 1.0;

    vec3 ray_dir = normalize(vec3(x, y * (float(u_screen.y) / float(u_screen.x)), 1.0));

    float depth = 1.0;

    for (int i = 0; i < 128; i++) {
        float dist = sdf(u_eye + ray_dir * depth);

        if (dist <= EPS)
            break;

        depth += dist;

        if (depth >= END) {
            depth = END;
            break;
        }
    }

    vec3 light_vec = normalize(vec3(sin(u_time), 1.0, -cos(u_time)));

    vec3 p = u_eye + ray_dir * depth;
    vec3 n = get_normal(p);

    if (depth < END) {
        vec3 col = vec3(max(dot(light_vec, n), 0.0));
        out_color = vec4(col, 1.0);
    } else {
        out_color = vec4(vec3(o_coords.x, o_coords.y, 1.0), 1.0);
    }
}

