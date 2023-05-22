#include "utils.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>


#define IMAGE_WIDTH  (1920 * 1)
#define IMAGE_HEIGHT (1080 * 1)

#define to_screen(x, y) { x * IMAGE_WIDTH, y * IMAGE_HEIGHT }
#define to_vec(x, y)    { x * (IMAGE_WIDTH / 2), -y * (IMAGE_HEIGHT / 2) }

typedef struct
{
    uint8_t r, g, b, a;
} pixel_t;

static inline pixel_t pixel_blend(pixel_t p1, pixel_t p2, float t1)
{
    float r1 = p1.r * t1;
    float g1 = p1.g * t1;
    float b1 = p1.b * t1;
    float a1 = p1.a * t1;

    float t2 = 1.0f - t1;

    float r2 = p2.r * t2;
    float g2 = p2.g * t2;
    float b2 = p2.b * t2;
    float a2 = p2.a * t2;

    return (pixel_t) {
        .r = (uint8_t)(r1 + r2),
        .g = (uint8_t)(g1 + g2),
        .b = (uint8_t)(b1 + b2),
        .a = (uint8_t)(a1 + a2),
    };
}


typedef struct
{
    float pos;
    pixel_t color;
} color_point_t;

#define MAX_COLOR_POINTS 8

typedef struct
{
    int count;
    color_point_t points[MAX_COLOR_POINTS];
} gradient_t;

static inline pixel_t gradient_sample(gradient_t *gradient, float p)
{
    int i = 0;
    color_point_t point;

    for (; i < gradient->count; ++i) {
        point = gradient->points[i];

        if (point.pos >= p)
            break;
    }

    int i_2 = i - 1;
    if (i_2 < 0)
        i_2 = 0;

    if (i == gradient->count)
        --i;

    if (i == i_2)
        return point.color;

    color_point_t point_2 = gradient->points[i_2];

    float length = point.pos - point_2.pos;
    float p_rel  = p         - point_2.pos;

    return pixel_blend(point.color, point_2.color, p_rel / length);
}


typedef struct
{
    pixel_t *pixels;
    int width, height;
} image_t;

typedef struct
{
    float x, y;
} vec2_t;

static inline vec2_t vec2_scale(vec2_t v, float s)
{
    return (vec2_t) { v.x * s, v.y * s };
}

static inline vec2_t vec2_add(vec2_t a, vec2_t b)
{
    return (vec2_t) { a.x + b.x, a.y + b.y };
}

static inline vec2_t vec2_sub(vec2_t a, vec2_t b)
{
    return (vec2_t) { a.x - b.x, a.y - b.y };
}

static inline float vec2_length(vec2_t v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

static inline vec2_t vec2_normalize(vec2_t v)
{
    float l = vec2_length(v);

    return (vec2_t) { v.x / l,
                      v.y / l };
}

static inline vec2_t vec2_cmul(vec2_t a, vec2_t b)
{
    return (vec2_t) { a.x * b.x - a.y * b.y,
                      a.x * b.y + a.y * b.x };
}

static inline vec2_t vec2_conj(vec2_t v)
{
    return (vec2_t) { v.x, -v.y };
}

static inline vec2_t vec2_from_angle(float x)
{
    return (vec2_t) { cosf(x), -sinf(x) };
}

static inline vec2_t vec2_project(vec2_t v)
{
    return (vec2_t) { (1.0f - (v.x * (IMAGE_HEIGHT / (float)IMAGE_WIDTH))) * (IMAGE_WIDTH  / 2),
                      (1.0f - v.y) * (IMAGE_HEIGHT / 2) };
}

static inline vec2_t vec2_provect(vec2_t v)
{
    return (vec2_t) { v.x * (IMAGE_WIDTH  / (float)(2 * IMAGE_HEIGHT)),
                     -v.y * (IMAGE_HEIGHT / 2) };
}

void plot_dot(image_t image, int x, int y, int d, pixel_t source)
{
    int start_x = x - d / 2;
    if (start_x < 0) start_x = 0;

    int start_y = y - d / 2;
    if (start_y < 0) start_y = 0;

    int end_x = start_x + d;
    if (end_x >= image.width) end_x = image.width;

    int end_y = start_y + d;
    if (end_y >= image.height) end_y = image.height;

    for (int yi = start_y; yi < end_y; ++yi) {
        for (int xi = start_x; xi < end_x; ++xi) {
            int dx = xi - x;
            int dy = yi - y;
            if (dx * dx + dy * dy < d * d / 4) {
                pixel_t dest = image.pixels[xi + yi * image.width];
                image.pixels[xi + yi * image.width] = pixel_blend(source, dest, source.a / 255.0f);
            }
        }
    }
}

vec2_t bezier(float t, int off, int depth, vec2_t *points, int point_count)
{
    if (depth == point_count - 1)
        return points[off];

    return vec2_add(vec2_scale(bezier(t, off,     depth + 1, points, point_count), 1.0f - t),
                    vec2_scale(bezier(t, off + 1, depth + 1, points, point_count), t));
}

void plot_bezier(image_t image,
                 vec2_t *points, int point_count,
                 int samples, gradient_t *gradient)
{
    for (int i = 0; i < samples; ++i) {
        float t = (float)i / samples;
        vec2_t p = bezier(t, 0, 0, points, point_count);
        plot_dot(image, (int)p.x, (int)p.y, 3, gradient_sample(gradient, t));
    }
}

#define EDGE_POS to_screen(0.4f, 0.5f)

static vec2_t left_p[] = {
    EDGE_POS,
    to_screen(0.35f, 1.0f),
    to_screen(0.0f, 0.8f),
    to_screen(0.1f, 1.3f),
};

static vec2_t left_h[] = {
    EDGE_POS,
    to_screen(0.3f, 0.2f),
    to_screen(0.1f, 0.4f),
    to_screen(-0.1f, 0.15f),
};

vec2_t right_p[4];

vec2_t right_h[] = {
    to_screen(0.00000, 0.00000),
    to_screen(0.5, 0.3),
    to_screen(0.7, 0.55),
    to_screen(0.85, 0.2),
    to_screen(1.1, 0.2),
    to_screen(1.6, 0.2),
};

gradient_t grad = {
    .count  = 2,
    .points = {
        { 0.0f, { 25,   25, 125, 255 } },
        { 1.0f, { 255, 150, 0,   255 } },
    }
};
gradient_t *color = &grad;

gradient_t grad_rev = {
    .count  = 2,
    .points = {
        { 0.0f, { 255, 150, 0,   255 } },
        { 1.0f, { 25,   25, 125, 255 } },
    }
};
gradient_t *color_rev = &grad_rev;

gradient_t sky_grad = {
    .count  = 2,
    .points = {
        // { 0.0f, { 255, 150, 0,   255 } },
        { 0.0f, { 255, 255, 255,   255 } },
        { 1.0f, { 25,   25, 125, 255 } },
    }
};
gradient_t *color_sky = &sky_grad;

gradient_t path_grad = {
    .count  = 3,
    .points = {
        { 0.0f, { 255, 255, 255,   255 } },
        // { 0.5f, { 25,   25, 125, 255 } },
        { 0.5f, { 255, 150, 0,   255 } },
        { 1.0f, { 25,   25, 125, 255 } },
    }
};
gradient_t *color_path = &path_grad;


void render_terrain(image_t image)
{
    plot_bezier(image, left_p, length(left_p), 2000, color);

    plot_bezier(image, left_h, length(left_h), 2000, color);

    int iters = 64;

    for (int i = 1; i < iters; ++i) {
        vec2_t p1[] = {
            left_h[1],
            to_screen(-0.2f, 0.7f),
        };

        vec2_t p3[] = {
            left_h[3],
            to_screen(-0.3f, 0.4f),
        };

        vec2_t ps[] = {
            bezier((float)i / iters, 0, 0, left_p, length(left_p)),
            bezier((float)i / iters, 0, 0, p1, length(p1)),
            left_h[2],
            bezier((float)i / iters, 0, 0, p3, length(p3)),
        };

        plot_bezier(image, ps, length(ps), 2000, color);
    }

    iters = 64;

    for (int i = 1; i < iters; ++i) {
        vec2_t p1[] = {
            left_p[1],
            to_screen(0.15f, 1.0f),
        };

        vec2_t p2[] = {
            left_p[2],
            to_screen(-0.3f, 0.5f),
        };

        vec2_t p3[] = {
            left_p[3],
            to_screen(-0.5f, 1.0f),
        };

        vec2_t ps[] = {
            bezier((float)i / iters, 0, 0, left_h, length(left_h)),
            bezier((float)i / iters, 0, 0, p1, length(p1)),
            bezier((float)i / iters, 0, 0, p2, length(p2)),
            bezier((float)i / iters, 0, 0, p3, length(p3)),
        };

        plot_bezier(image, ps, length(ps), 2000, color_rev);
    }

    right_p[0] = vec2_add(left_p[0], (vec2_t) to_screen(0.0125f, 0.03f));
    right_p[1] = vec2_add(left_p[1], (vec2_t) to_screen(0.02f,   0.0f));
    right_p[2] = vec2_add(left_p[2], (vec2_t) to_screen(0.08f,   0.0f));
    right_p[3] = vec2_add(left_p[3], (vec2_t) to_screen(0.16f,   0.0f));

    plot_bezier(image, right_p, length(right_p), 2000, color);

    right_h[0] = right_p[0];

    plot_bezier(image, right_h, length(right_h), 2000, color);

    iters = 96;

    for (int i = 1; i < iters; ++i) {
        vec2_t p1[] = {
            right_p[1],
            to_screen(0.2f, 0.6f),
        };

        vec2_t p2[] = {
            right_p[2],
            to_screen(1.8f, 0.75f),
        };

        vec2_t p3[] = {
            right_p[3],
            to_screen(1.5f, 1.0f),
        };

        vec2_t ps[] = {
            bezier((float)i / iters, 0, 0, right_h, length(right_h)),
            bezier((float)i / iters, 0, 0, p1, length(p1)),
            bezier((float)i / iters, 0, 0, p2, length(p2)),
            bezier((float)i / iters, 0, 0, p3, length(p3)),
        };

        plot_bezier(image, ps, length(ps), 2000, color_rev);
    }

    iters = 64;

    for (int i = 1; i < iters; ++i) {
        vec2_t p1[] = {
            right_h[1],
            to_screen(0.6f, 0.9f),
        };

        vec2_t p2[] = {
            right_h[2],
            to_screen(0.6f, 0.9f),
        };

        vec2_t p3[] = {
            right_h[3],
            to_screen(1.1f, 1.3f),
        };

        vec2_t p4[] = {
            right_h[4],
            to_screen(1.6f, 1.6f),
        };

        vec2_t ps[] = {
            bezier((float)i / iters, 0, 0, right_p, length(right_p)),
            bezier((float)i / iters, 0, 0, p1, length(p1)),
            bezier((float)i / iters, 0, 0, p2, length(p2)),
            bezier((float)i / iters, 0, 0, p3, length(p3)),
            bezier((float)i / iters, 0, 0, p4, length(p4)),
        };

        plot_bezier(image, ps, length(ps), 2000, color);
    }
}

void render_path(image_t image)
{
    int iters = 12;

    for (int i = 0; i < iters; ++i) {
        vec2_t points[length(left_p)];

        for (int p = 0; p < length(points); ++p) {
            vec2_t two[] = { left_p[p], right_p[p] };

            points[p] = bezier((float)i / iters, 0, 0, two, length(two));
        }

        plot_bezier(image, points, length(points), 2000, color_path);
    }
}

vec2_t hermite(float t, vec2_t p0, vec2_t v0, vec2_t p1, vec2_t v1)
{
    float t2 = t * t;
    float t3 = t2 * t;

    return vec2_add(vec2_add(vec2_scale(p0, 2.0f * t3 - 3.0f * t2 + 1.0f),
                             vec2_scale(v0, t3 - 2.0f * t2 + t)),
                    vec2_add(vec2_scale(p1, -2.0f * t3 + 3 * t2),
                             vec2_scale(v1, t3 - t2)));
}

void plot_hermite(image_t image,
                  vec2_t *ps, vec2_t *vs, int count,
                  int samples, pixel_t color, int size)
{
    for (int i = 0; i < samples; ++i) {
        float t = (float)i / samples;
        float scaled = t * (count - 1);
        int index = (int)(scaled);

        vec2_t p = hermite(scaled - index,
                           ps[index],     vs[index],
                           ps[index + 1], vs[index + 1]);

        p = vec2_project(p);

        plot_dot(image, (int)p.x, (int)p.y, size, color);
    }
}

void plot_tree_rec(image_t image,
                   vec2_t pos,
                   vec2_t vec, vec2_t p_vec,
                   vec2_t rot1, vec2_t rot2, vec2_t rot3,
                   int depth)
{
    if (depth == 0)
        return;

    vec2_t n_pos = vec2_add(pos, vec);

    vec2_t points[] = {
        pos,
        n_pos,
    };

    vec2_t vecs[] = {
        p_vec,
        vec,
    };

    vec2_t n_pos2 = hermite(0.6f, pos, p_vec, n_pos, vec);
    vec2_t n_pos3 = hermite(0.9f, pos, p_vec, n_pos, vec);

    pixel_t col = { 0, 0, 0, 255 };

    plot_hermite(image, points, vecs, length(vecs), 500, col, depth * 2);

    plot_tree_rec(image, n_pos,  vec2_cmul(vec, rot1), vec, rot1, rot2, rot3, depth - 1);
    plot_tree_rec(image, n_pos3, vec2_cmul(vec, rot2), vec, rot1, rot2, rot3, depth - 1);
    plot_tree_rec(image, n_pos2, vec2_cmul(vec, rot3), vec, rot1, rot2, rot3, depth - 1);
}

void plot_tree(image_t image)
{
    vec2_t v = { 0.0f, 0.35f };
    vec2_t root = {-0.2f, 0.35f };

    plot_tree_rec(image,
        (vec2_t) {-0.6f,-0.9f },
        v, root,
        vec2_scale(vec2_from_angle(-0.5f), 0.9f),
        vec2_scale(vec2_from_angle(0.3f),  0.9f),
        vec2_scale(vec2_from_angle(0.55f),  0.9f),
        7
    );
}

void render_sky(image_t image)
{
    vec2_t mid = to_screen(0.0f, 0.56f);
    mid.x = left_h[0].x;

    mid.x += 4.5f;
    mid.y += 50.0f;

    float len = 1000.0f;

    int iters = 196;

    for (int i = 0; i < iters; ++i) {
        vec2_t p1 = bezier((float)i / iters, 0, 0, left_h, length(left_h));

        vec2_t ps[] = {
            p1,
            vec2_add(p1, vec2_scale(vec2_normalize(vec2_sub(p1, mid)), len)),
        };

        plot_bezier(image, ps, length(ps), 2000, color_sky);
    }

    iters = 11;

    vec2_t gap[] = {
        left_h[0],
        right_h[0],
    };

    for (int i = 0; i < iters; ++i) {
        vec2_t p1 = bezier((float)i / iters, 0, 0, gap, length(gap));

        vec2_t ps[] = {
            p1,
            vec2_add(p1, vec2_scale(vec2_normalize(vec2_sub(p1, mid)), len)),
        };

        plot_bezier(image, ps, length(ps), 2000, color_sky);
    }

    iters = 196;

    for (int i = 0; i < iters; ++i) {
        vec2_t p1 = bezier((float)i / iters, 0, 0, right_h, length(right_h));

        vec2_t ps[] = {
            p1,
            vec2_add(p1, vec2_scale(vec2_normalize(vec2_sub(p1, mid)), len)),
        };

        plot_bezier(image, ps, length(ps), 2000, color_sky);
    }
}

void render(image_t image)
{
    render_terrain(image);
    render_sky(image);
    render_path(image);
    plot_tree(image);
}

int main(void)
{
    pixel_t *pixels = malloc(sizeof(pixel_t) * IMAGE_WIDTH * IMAGE_HEIGHT);
    malloc_check(pixels);

    /* clear image */
    for (int y = 0; y < IMAGE_HEIGHT; ++y) {
        for (int x = 0; x < IMAGE_WIDTH; ++x) {
            // int c = (x + y) % 2 ? 100 : 150;
            // pixels[x + y * IMAGE_WIDTH] = (pixel_t) { c, c, c, 255 };
            pixels[x + y * IMAGE_WIDTH] = (pixel_t) { 50, 50, 50, 255 };
        }
    }

    image_t image = { pixels, IMAGE_WIDTH, IMAGE_HEIGHT };

    render(image);

    printf("rendered\n");

    if (!stbi_write_png("lol.png", IMAGE_WIDTH, IMAGE_HEIGHT, 4, pixels, sizeof(pixel_t) * IMAGE_WIDTH)) {
        fprintf(stderr, "error: Failed to write image!\n");
        exit(1);
    }

    free(pixels);

    return 0;
}
