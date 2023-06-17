#ifndef MATH_H
#define MATH_H

#include <stdio.h>
#include <math.h>
#include <stdbool.h>

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


static inline matrix_t matrix_transpose(matrix_t m)
{
    matrix_t res;

    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            res.data[y * 4 + x] = m.data[x * 4 + y];
        }
    }

    return res;
}


/* https://gitlab.freedesktop.org/mesa/glu/-/blob/glu-9.0.0/src/libutil/project.c
 *
 * static int __gluInvertMatrixd(const GLdouble m[16], GLdouble invOut[16])
 *
 * Contributed by David Moore (See Mesa bug #6748)
 */
static inline matrix_t matrix_inverse(matrix_t mat)
{
    matrix_t res;

    float *m = mat.data;

    float inv[16];

    inv[0] =   m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
             + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
             - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
             + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
             - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
             - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
             + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
             - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
             + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
             + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
    inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
             - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
             + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
             - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
    inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
             - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
    inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
             + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
             - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
             + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

    float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (det == 0)
        return mat;

    det = 1.0f / det;

    for (int i = 0; i < 16; i++) {
        res.data[i] = inv[i] * det;
    }

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

static inline vector_t vector_transform(vector_t v, matrix_t m)
{
    vector_t res = {{ 0.0f, 0.0f, 0.0f, 0.0f }};

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            res.data[i] += m.data[i + j * 4] * v.data[j];
        }
    }

    return res;
}


typedef union
{
    struct { float x, y, z; };
    float data[3];
} vec3_t;

static inline vec3_t vec3_add(vec3_t a, vec3_t b)
{
    return (vec3_t) {{
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
    }};
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b)
{
    return (vec3_t) {{
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
    }};
}

static inline vec3_t vec3_scale(vec3_t a, float s)
{
    return (vec3_t) {{
        a.x * s,
        a.y * s,
        a.z * s,
    }};
}

static inline float vec3_length(vec3_t v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline vec3_t vec3_normalize(vec3_t v)
{
    float len = vec3_length(v);
    return (vec3_t) {{
        v.x / len,
        v.y / len,
        v.z / len,
    }};
}


typedef union
{
    struct { float x, y; };
    float data[2];
} vec2_t;

typedef union
{
    struct { int x, y; };
    int data[2];
} ivec2_t;

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

// FIXME: Make it vec3
static inline float vec_len(float a[3])
{
    return sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

static inline float clamp(float x, float mn, float mx)
{
    if (x < mn) return mn;
    if (x > mx) return mx;
    return x;
}


typedef struct
{
    int x, y, w, h;
} rect_t;

static inline bool rect_contains(rect_t rect, int x, int y)
{
    return x >= rect.x && x < rect.x + rect.w
        && y >= rect.y && y < rect.y + rect.h;
}


typedef struct
{
    float x, y, w, h;
} frect_t;

static inline frect_t frect_make(rect_t rect, int width, int height)
{
    return (frect_t) {
        .x = rect.x / (float)width,
        .y = rect.y / (float)height,
        .w = rect.w / (float)width,
        .h = rect.h / (float)height,
    };
}

#endif
