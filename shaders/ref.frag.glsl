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


float sphere_sdf(vec3 x)
{
    return length(x) - 1.0;
}

float cube_sdf(vec3 x)
{
    vec3 v = abs(x) - vec3(1.0);
    return max(v.x, max(v.y, v.z));
}

float box_sdf(vec3 x, vec3 s)
{
    vec3 v = abs(x) - s;
    return max(v.x, max(v.y, v.z));
}

float capsule_sdf(vec3 x, float mn, float mx, float r)
{
    vec3 d = vec3(0.0, clamp(x.y, mn, mx), 0.0);
    return length(x - d) - r;
}

void grid_sdf(inout vec3 x)
{
    for (int i = 0; i < 3; ++i) {
        x *= 3.0;

        x.x -= sign(x.x) * 2.0;
        x.y -= sign(x.y) * 2.0;
        x.z -= sign(x.z) * 2.0;
    }
}

float smin(float a, float b, float k)
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float tree_sdf(vec3 x)
{
    // float dist = capsule_sdf(x, 0.0, 0.5, 0.1);
    float dist = capsule_sdf(x, 0.0, 0.5, 0.1);

    for (int i = 0; i < 5; ++i) {
        x.y -= 0.5;

        float ud = 0.7;
        float u = sign(x.x) * 0.3;

        float su = sin(u);
        float cu = cos(u);

        float sud = sin(ud);
        float cud = cos(ud);

        x = vec3(x.x * cu - x.y * su,
                 x.x * su + x.y * cu,
                 x.z);

        x = vec3(x.x * cud - x.z * sud,
                 x.y,
                 x.x * sud + x.z * cud);

        dist = smin(dist, capsule_sdf(x, 0.0, 0.5, 0.1), 0.05);
    }

    return dist;
}

float cross_sdf(vec3 p)
{
  float da = cube_sdf(vec3(p.xy, 0.0));
  float db = cube_sdf(vec3(p.yz, 0.0));
  float dc = cube_sdf(vec3(p.zx, 0.0));

  return min(da, min(db, dc));
}

float map(vec3 p)
{
    float d = cube_sdf(p);

    float s = 1.0;

    for (int m = 0; m < 2; ++m) {
        vec3 a = mod(p * s, 2.0) - 1.0;
        s *= 3.0;
        vec3 r = 1.0 - 3.0 * abs(a);
    
        float c = cross_sdf(r) / s;
        d = max(d, c);
    }

    return d;
}

float sdf_one(vec3 x)
{
    float t = u_time;

    x = vec3(x.x * cos(t) - x.z * sin(t),
             x.y,
             x.x * sin(t) + x.z * cos(t));

    vec3 p = x;

    // grid_sdf(p);
    return map(p);

    // return max(cube_sdf(x),
            // -sphere_sdf(x / 1.25) * 1.25);
    // return max(-(cube_sdf(p) / 27.0),
                 // cube_sdf(x * 1.01) / 1.01);
    // return capsule_sdf(p, 0.0, 0.5, 0.05);
}

float sdf(vec3 x)
{
    vec3 c = vec3(0.0, 0.0, 3.0);
    x = x - c;

    return sdf_one(mod(x + vec3(5.0), 10.0) - vec3(5.0));
    // return sdf_one(x);
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

    float depth = 0.1;
    float dist;

    for (int i = 0; i < 256; i++) {
        dist = sdf(u_eye + ray_dir * depth);

        if (dist <= EPS)
            break;

        depth += dist;

        if (depth >= END) {
            break;
        }
    }

    // vec3 light_vec = normalize(vec3(sin(u_time), 1.0, -cos(u_time)));
    vec3 light_vec = vec3(1.0, 1.0, -1.0);

    // vec3 p = u_eye + ray_dir * depth;
    vec3 p = u_eye + ray_dir * depth;
    vec3 n = get_normal(p);

    if (depth < END) {
        vec3 col = vec3(max(dot(light_vec, n), 0.0));
        col = pow(col, vec3(0.45));
        out_color = vec4(col, 1.0);
    } else {
        out_color = vec4(vec3(o_coords.x, o_coords.y, 1.0), 1.0);
    }
}

