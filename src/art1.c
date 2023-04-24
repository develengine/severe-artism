#include "utils.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdint.h>


#define IMAGE_WIDTH  1920
#define IMAGE_HEIGHT 1080

#define to_screen(x, y) { x * IMAGE_WIDTH, y * IMAGE_HEIGHT }

typedef struct
{
    uint8_t r, g, b, a;
} pixel_t;

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


void plot_dot(image_t image, int x, int y, int d, pixel_t color)
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
                image.pixels[xi + yi * image.width] = color;
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

void plot_bezier(image_t image, vec2_t *points, int point_count, int samples, pixel_t color)
{
    for (int i = 0; i < samples; ++i) {
        vec2_t p = bezier((float)i / samples, 0, 0, points, point_count);
        plot_dot(image, (int)p.x, (int)p.y, 3, color);
    }
}

void render(image_t image)
{
    vec2_t left_p[] = {
        to_screen(0.4f, 0.5f),
        to_screen(0.35f, 1.0f),
        to_screen(0.0f, 0.8f),
        to_screen(0.1f, 1.3f),
    };

    pixel_t color = { 30, 30, 30, 255 };

    plot_bezier(image, left_p, length(left_p), 2000, color);

    vec2_t left_h[] = {
        left_p[0],
        to_screen(0.3f, 0.2f),
        to_screen(0.1f, 0.4f),
        to_screen(-0.1f, 0.15f),
    };

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

        plot_bezier(image, ps, length(ps), 2000, color);
    }

    vec2_t right_p[] = {
        vec2_add(left_p[0], (vec2_t) to_screen(0.0125f, 0.03f)),
        vec2_add(left_p[1], (vec2_t) to_screen(0.02f,   0.0f)),
        vec2_add(left_p[2], (vec2_t) to_screen(0.08f,   0.0f)),
        vec2_add(left_p[3], (vec2_t) to_screen(0.16f,   0.0f)),
    };

    plot_bezier(image, right_p, length(right_p), 2000, color);

    vec2_t right_h[] = {
        right_p[0],
        to_screen(0.5, 0.3),
        to_screen(0.7, 0.55),
        to_screen(0.85, 0.2),
        to_screen(1.1, 0.2),
        to_screen(1.6, 0.2),
    };

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

        plot_bezier(image, ps, length(ps), 2000, color);
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

int main(void)
{
    pixel_t *pixels = malloc(sizeof(pixel_t) * IMAGE_WIDTH * IMAGE_HEIGHT);
    malloc_check(pixels);

    /* clear image */
    for (int y = 0; y < IMAGE_HEIGHT; ++y) {
        for (int x = 0; x < IMAGE_WIDTH; ++x) {
            int c = (x + y) % 2 ? 200 : 150;
            pixels[x + y * IMAGE_WIDTH] = (pixel_t) { c, c, c, 255 };
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
