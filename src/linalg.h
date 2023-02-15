#ifndef MATH_H
#define MATH_H

#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    float data[16];
} matrix_t;


static inline matrix_t matrix_identity(void)
{
    matrix_t res = {{
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f,
    }};
    return res;
}


static inline matrix_t matrix_scale(float x, float y, float z)
{
    matrix_t res = {{
         x,    0.0f, 0.0f, 0.0f,
         0.0f, y,    0.0f, 0.0f,
         0.0f, 0.0f, z,    0.0f,
         0.0f, 0.0f, 0.0f, 1.0f,
    }};
    return res;
}


static inline matrix_t matrix_translation(float x, float y, float z)
{
    matrix_t res = {{
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         x,    y,    z,    1.0f,
    }};
    return res;
}


static inline matrix_t matrix_rotation_x(float angle)
{
    matrix_t res = {{
         1.0f, 0.0f,        0.0f,        0.0f,
         0.0f, cosf(angle), sinf(angle), 0.0f,
         0.0f,-sinf(angle), cosf(angle), 0.0f,
         0.0f, 0.0f,        0.0f,        1.0f,
    }};
    return res;
}


static inline matrix_t matrix_rotation_y(float angle)
{
    matrix_t res = {{
         cosf(angle), 0.0f,-sinf(angle), 0.0f,
         0.0f,        1.0f, 0.0f,        0.0f,
         sinf(angle), 0.0f, cosf(angle), 0.0f,
         0.0f,        0.0f, 0.0f,        1.0f,
    }};
    return res;
}


static inline matrix_t matrix_rotation_z(float angle)
{
    matrix_t res = {{
         cosf(angle), sinf(angle), 0.0f, 0.0f,
        -sinf(angle), cosf(angle), 0.0f, 0.0f,
         0.0f,        0.0f,        1.0f, 0.0f,
         0.0f,        0.0f,        0.0f, 1.0f,
    }};
    return res;
}


static inline matrix_t matrix_projection(
        float fov,
        float width,
        float height,
        float np,
        float fp
) {
    float ar = height / width;
    float xs = (float)((1.0 / tan((fov / 2.0) * M_PI / 180)) * ar);
    float ys = xs / ar;
    float flen = fp - np;

    matrix_t res = {{
         xs,   0.0f, 0.0f,                   0.0f,
         0.0f, ys,   0.0f,                   0.0f,
         0.0f, 0.0f,-((fp + np) / flen),    -1.0f,
         0.0f, 0.0f,-((2 * np * fp) / flen), 0.0f, // NOTE if 1.0f removes far plane clipping ???
    }};

    return res;
}


static inline matrix_t matrix_multiply(matrix_t mat1, matrix_t mat2)
{
    const float *m1 = mat1.data;
    const float *m2 = mat2.data;

    matrix_t res;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float value = 0;

            for (int k = 0; k < 4; k++)
                value += m2[j * 4 + k] * m1[k * 4 + i];

            res.data[j * 4 + i] = value;
        }
    }

    return res;
}


static inline void print_matrix(const matrix_t *mat)
{
    printf("\n");

    for (int y = 0; y < 4; ++y) {
        printf("[ ");

        for (int x = 0; x < 4; ++x)
            printf("%f ", mat->data[y * 4 + x]);

        printf("]\n");
    }
}


typedef union
{
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    float data[4];
} vector_t;

typedef vector_t color_t;


typedef union
{
    struct { float x, y, z; };
    float data[3];
} vec3_t;


typedef union
{
    struct { float x, y; };
    float data[2];
} vec2_t;


typedef union
{
    struct { float r, i, j, k; };
    struct { float w, x, y, z; };
    float data[4];
} quaternion_t;


static inline quaternion_t quaternion_n_lerp(quaternion_t a, quaternion_t b, float blend)
{
    quaternion_t res;

    float dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    float blend_i = 1.0f - blend;

    if (dot < 0) {
        res = (quaternion_t) {{
            blend_i * a.w + blend * -b.w,
            blend_i * a.x + blend * -b.x,
            blend_i * a.y + blend * -b.y,
            blend_i * a.z + blend * -b.z
        }};
    } else {
        res = (quaternion_t) {{
            blend_i * a.w + blend * b.w,
            blend_i * a.x + blend * b.x,
            blend_i * a.y + blend * b.y,
            blend_i * a.z + blend * b.z
        }};
    }

    float length = sqrtf(res.w * res.w
                       + res.x * res.x
                       + res.y * res.y
                       + res.z * res.z);
    res.w /= length;
    res.x /= length;
    res.y /= length;
    res.z /= length;

    return res;
}


static inline matrix_t quaternion_to_matrix(quaternion_t r)
{
    float xy = r.x * r.y;
    float xz = r.x * r.z;
    float xw = r.x * r.w;
    float yz = r.y * r.z;
    float yw = r.y * r.w;
    float zw = r.z * r.w;
    float xx = r.x * r.x;
    float yy = r.y * r.y;
    float zz = r.z * r.z;

    matrix_t res = {{
        1 - 2 * (yy + zz), 2 * (xy - zw),     2 * (xz + yw),     0,
        2 * (xy + zw),     1 - 2 * (xx + zz), 2 * (yz - xw),     0,
        2 * (xz - yw),     2 * (yz + xw),     1 - 2 * (xx + yy), 0,
        0,                 0,                 0,                 1
    }};

    return res;
}


static inline void position_lerp(float out[3], float a[3], float b[3], float blend)
{
    float blend_i = 1.0f - blend;
    out[0] = blend_i * a[0] + blend * b[0];
    out[1] = blend_i * a[1] + blend * b[1];
    out[2] = blend_i * a[2] + blend * b[2];
}


static inline void cross(float out[3], float a[3], float b[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}


static inline float dot(float a[3], float b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}


static inline float vec_len(float a[3])
{
    return sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}


#endif
