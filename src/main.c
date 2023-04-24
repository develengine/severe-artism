#include "bag_engine.h"
#include "bag_keys.h"
#include "utils.h"
#include "linalg.h"
#include "res.h"
#include "animation.h"
#include "core.h"
#include "audio.h"
#include "gui.h"

#include "art_text.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


/* NOTES:
 * - arbitrary but mood associated color schemes
 * - meant mostly for generating looping videos
 *   - support for perfect looping specifically
 * - exporting of images and video
 *   - possibly ffmpeg
 * - all methods sould have performance parameters that can be
 *   tweaked for online and offline rendering
 * - way of adding and tagging motifs
 * - post processing and glitches, text errors
 *
 * Associations:
 * - recursion
 * - affect
 * - meaning
 * - reflection
 * - symmetry
 * - analogy
 * - approximation
 *
 * - To simplify the architecture, the system should be able to
 *   provide input up-front for subsystems, but also the subsystems
 *   should be able to request additional data to be filled in.
 *   - Talking back and forth between subsystems.
 *
 * - Maybe also sound input?
 * - Maybe showing code that creates the things themself?
 *   - Procedural shader generator?
 */

#define EPS 0.001
#define END 100.0


typedef struct
{
    uint8_t r, g, b, a;
} pixel_t;


static inline float capsule_sdf(vec3_t x, float mn, float mx, float r)
{
    vec3_t d = (vec3_t) {{ 0.0, clamp(x.y, mn, mx), 0.0 }};
    return vec3_length(vec3_sub(x, d)) - r;
}

static inline float tree_sdf(vec3_t x)
{
    float dist = capsule_sdf(x, 0.0f, 0.5f, 0.1f);

    for (int i = 0; i < 5; ++i) {
        x.y -= 0.5f;

        float ud = 0.7f;
        float u = x.x < 0.0 ? -0.3f : 0.3f;

        float su = sinf(u);
        float cu = cosf(u);

        float sud = sinf(ud);
        float cud = cosf(ud);

        x = (vec3_t) {{
            x.x * cu - x.y * su,
            x.x * su + x.y * cu,
            x.z
        }};

        x = (vec3_t) {{
            x.x * cud - x.z * sud,
            x.y,
            x.x * sud + x.z * cud
        }};

        float cd = capsule_sdf(x, 0.0f, 0.5f, 0.1f);
        if (cd < dist)
            dist = cd;
    }

    return dist;
}

static inline float sdf(vec3_t x)
{
    vec3_t c = {{ 0.0f, -1.5f, 2.5f }};
    x = vec3_sub(x, c);

    float t = 0.0f;

    x = (vec3_t) {{
        x.x * cosf(t) - x.z * sinf(t),
        x.y,
        x.x * sinf(t) + x.z * cosf(t)
    }};

    return tree_sdf(x);
}

static inline vec3_t get_normal(vec3_t p)
{
    return vec3_normalize((vec3_t) {{
        sdf((vec3_t) {{ p.x + EPS, p.y, p.z }}) - sdf((vec3_t) {{ p.x - EPS, p.y, p.z }}),
        sdf((vec3_t) {{ p.x, p.y + EPS, p.z }}) - sdf((vec3_t) {{ p.x, p.y - EPS, p.z }}),
        sdf((vec3_t) {{ p.x, p.y, p.z + EPS }}) - sdf((vec3_t) {{ p.x, p.y, p.z - EPS }})
    }});
}

void render_tree(pixel_t *buffer, int buffer_width, int buffer_height)
{
    for (int yi = 0; yi < buffer_height; ++yi) {
        for (int xi = 0; xi < buffer_width; ++xi) {
            float x = 2.0f * ((float)xi / buffer_width)  - 1.0f;
            float y = 2.0f * ((float)yi / buffer_height) - 1.0f;

            vec3_t ray_dir = {{ x, y * ((float)buffer_height / buffer_width), 1.0f }};
            ray_dir = vec3_normalize(ray_dir);

            float depth = 0.1f;
            float dist;

            vec3_t at = vec3_scale(ray_dir, depth);

            for (int i = 0; i < 256; i++) {
                dist = sdf(at);

                depth += dist;

                if (dist <= EPS)
                    break;

                at = vec3_add(at, vec3_scale(ray_dir, dist));

                if (depth >= END) {
                    break;
                }
            }

            vec3_t light_vec = {{ 1.0f, 1.0f, -1.0f }};

            vec3_t p = at;
            vec3_t n = get_normal(p);
        }
    }
}

void render_cpu(void)
{
    
}


bool running = true;

int window_width  = 1080;
int window_height = 720;

bool f11_down   = false;
bool fullscreen = false;

bool flow_time = true;

int bagE_main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(opengl_callback, 0);

    print_context_info();

    bagE_setWindowTitle("severe artism");

    // TODO: when interpolation is needed this should be removed
    bagE_setSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_MULTISAMPLE);
    // glEnable(GL_MULTISAMPLE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    // glEnable(GL_FRAMEBUFFER_SRGB);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    render_cpu();

    init_audio();
    init_gui();

    // art_text_ctx_t art_text_ctx;
    // art_text_init(&art_text_ctx);

    // unsigned test_program = load_program("shaders/test.vert.glsl",
                                         // "shaders/test.frag.glsl");

    unsigned ref_program = load_program("shaders/ref.vert.glsl",
                                        "shaders/ref.frag.glsl");

    float time = 0.0f;

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

        // art_text_update(&art_text_ctx);

        /* rendering */
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* overlay */
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        gui_update_resolution(window_width, window_height);
        glBindVertexArray(gui.dummy_vao);

#if 1
#if 0
        glUseProgram(test_program);
        glProgramUniform2i(test_program, 0, 0, 0);
        glProgramUniform2i(test_program, 1, window_width, window_height);
        glProgramUniform2i(test_program, 2, window_width, window_height);
        glProgramUniform1f(test_program, 3, time);
        glDrawArrays(GL_TRIANGLES, 0, 6);
#else
        glUseProgram(ref_program);
        glProgramUniform2i(ref_program, 0, 0, 0);
        glProgramUniform2i(ref_program, 1, window_width, window_height);
        glProgramUniform2i(ref_program, 2, window_width, window_height);
        glProgramUniform1f(ref_program, 3, time);
        glProgramUniform1f(ref_program, 4, 90.0f);
        glProgramUniform3f(ref_program, 5, 0.0f, 0.0f, 0.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
#else
        art_text_render(&art_text_ctx, window_width, window_height);
#endif

        if (flow_time)
            time += 0.01666f;

        bagE_swapBuffers();
    }
  
    // art_text_free(&art_text_ctx);

    exit_audio(); /* NOTE: exit audio before all the sound buffers get freed */
    exit_gui();

    return 0;
}


int bagE_eventHandler(bagE_Event *event)
{
    bool down = false;

    switch (event->type)
    {
        case bagE_EventWindowClose:
            running = false;
            return 1;

        case bagE_EventWindowResize: {
            bagE_WindowResize *wr = &(event->data.windowResize);

            window_width  = wr->width;
            window_height = wr->height;
            glViewport(0, 0, wr->width, wr->height);
        } break;

        case bagE_EventKeyDown: 
            down = true;
            /* fallthrough */
        case bagE_EventKeyUp: {
            bagE_Key *key = &(event->data.key);
            switch (key->key) {
                case KEY_F11:
                    if (down) {
                        if(!f11_down) {
                            f11_down = true;
                            fullscreen = !fullscreen;
                            bagE_setFullscreen(fullscreen);
                        }
                    } else {
                        f11_down = false;
                    }
                    break;

                case KEY_P:
                    if (down)
                        break;
                    flow_time = !flow_time;
                    break;
            }
        } break;

        default: break;
    }

    return 0;
}

