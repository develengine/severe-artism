#include "bag_engine.h"
#include "bag_keys.h"
#include "utils.h"
#include "linalg.h"
#include "res.h"
#include "animation.h"
#include "core.h"
#include "audio.h"
#include "gui.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "ascii.c"


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


/* NOTE: Make sure the matrix is ' ' (space) cleared!
 *       Supports only valid ascii+128. */
static void draw_text_matrix(const unsigned char *matrix, int cols, int rows,
                             int x, int y, int font_width, color_t color)
{
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            gui_draw_text_n((char *)(matrix + row * cols), cols,
                          x, y + row * font_width * 2,
                          font_width, font_width * 2, 0, color);
        }
    }
}


typedef struct
{
    int width, height;
    unsigned char *matrix;

    ivec2_t *queue;
    int q_capacity, q_start, q_end;

    bool bfs_growth;
    unsigned char fallback;
} pipes_ctx_t;


static bool pipes_ctx_update(pipes_ctx_t *ctx)
{
    if (ctx->q_start == ctx->q_end)
        return false;

    ivec2_t pos = ctx->queue[ctx->q_start];
    queue_pop(ctx->queue, ctx->q_capacity, ctx->q_start, ctx->q_end);

    ivec2_t neighbours[4] = {
        {{ pos.x - 1, pos.y }},
        {{ pos.x + 1, pos.y }},
        {{ pos.x, pos.y - 1 }},
        {{ pos.x, pos.y + 1 }},
    };

    unsigned conns[4][4] = {
        //single line          double line
        //neighbour current    neighbour current
        { ASCII_RS, ASCII_LS,  ASCII_RD, ASCII_LD },
        { ASCII_LS, ASCII_RS,  ASCII_LD, ASCII_RD },
        { ASCII_BS, ASCII_TS,  ASCII_BD, ASCII_TD },
        { ASCII_TS, ASCII_BS,  ASCII_TD, ASCII_BD },
    };

    bool neighbours_passed[4] = {0};

    unsigned need_mask = 0;
    unsigned no_mask   = 0;

    for (int n = 0; n < 4; ++n) {
        ivec2_t p = neighbours[n];

        if (p.x >= 0 && p.x < ctx->width && p.y >= 0 && p.y < ctx->height) {
            unsigned char c = ctx->matrix[p.x + p.y * ctx->width];
            unsigned con = ascii_edge_connections[c];

            if (c >= ASCII_EDGES_BEGIN && c < ASCII_EDGES_END) {
                if      (con & conns[n][0]) need_mask |= conns[n][1];
                else if (con & conns[n][2]) need_mask |= conns[n][3];
                else no_mask |= conns[n][1] | conns[n][3];
            } else {
                neighbours_passed[n] = c != '@' && c != ctx->fallback;
            }
        }
    }

    unsigned char fits[ASCII_EDGES_END - ASCII_EDGES_BEGIN];
    int fit_count = 0;

    for (unsigned char c = ASCII_EDGES_BEGIN; c < ASCII_EDGES_END; ++c) {
        unsigned connects = ascii_edge_connections[c];

        if ((need_mask & connects) == need_mask && !(no_mask & connects))
            fits[fit_count++] = c;
    }

    unsigned char choice = fit_count ? fits[rand() % fit_count] : ctx->fallback;
    unsigned choice_mask = ascii_edge_connections[choice];

    for (int n = 0; n < 4; ++n) {
        if (!neighbours_passed[n])
            continue;

        unsigned nm = conns[n][1] | conns[n][3];

        if (ctx->bfs_growth || ((choice_mask & nm) && !(no_mask & nm))) {
            ivec2_t p = neighbours[n];
            ctx->matrix[p.x + p.y * ctx->width] = '@';
            queue_push(ivec2_t, ctx->queue, ctx->q_capacity,
                        ctx->q_start, ctx->q_end, p);
        }
    }

    ctx->matrix[pos.x + pos.y * ctx->width] = choice;

    return true;
}


bool running = true;

int window_width  = 1080;
int window_height = 720;

bool f11_down   = false;
bool fullscreen = false;


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

    init_audio();
    init_gui();

    /* text matrix */

    #define FONT_WIDTH 8
    #define MAT_WIDTH (640 / FONT_WIDTH)
    #define MAT_HEIGHT (480 / FONT_WIDTH / 2)
    #define MAT_SCALER 2

    unsigned char text_matrix[MAT_WIDTH * MAT_HEIGHT];
    memset(text_matrix, ' ', MAT_WIDTH * MAT_HEIGHT);

    pipes_ctx_t pipes = {
        .width = MAT_WIDTH,
        .height = MAT_HEIGHT,
        .matrix = text_matrix,
        .bfs_growth = false,
        .fallback = 'O',
    };

    queue_init(pipes.queue, pipes.q_capacity, pipes.q_start, pipes.q_end, 32);

    ivec2_t mid = {{ MAT_WIDTH / 2, MAT_HEIGHT / 2 }};
    text_matrix[mid.x + mid.y * MAT_WIDTH] = AsciiXS;
    queue_push(ivec2_t, pipes.queue, pipes.q_capacity, pipes.q_start, pipes.q_end,
               ((ivec2_t){{ mid.x - 1, mid.y }}));
    queue_push(ivec2_t, pipes.queue, pipes.q_capacity, pipes.q_start, pipes.q_end,
               ((ivec2_t){{ mid.x, mid.y - 1 }}));
    queue_push(ivec2_t, pipes.queue, pipes.q_capacity, pipes.q_start, pipes.q_end,
               ((ivec2_t){{ mid.x + 1, mid.y }}));
    queue_push(ivec2_t, pipes.queue, pipes.q_capacity, pipes.q_start, pipes.q_end,
               ((ivec2_t){{ mid.x, mid.y + 1 }}));

    unsigned test_program = load_program("shaders/test.vert.glsl",
                                         "shaders/test.frag.glsl");

    float time = 0.0f;

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

        // pipes_ctx_update(&pipes);

        /* rendering */
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* overlay */
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        gui_update_resolution(window_width, window_height);
        glBindVertexArray(gui.dummy_vao);

        glUseProgram(test_program);

        glProgramUniform2f(test_program, 0, -0.5f, 0.5f);
        glProgramUniform2f(test_program, 1, 1.0f, 1.0f);
        glProgramUniform1f(test_program, 2, time);
        glDrawArrays(GL_TRIANGLES, 0, 6);

#if 0
        color_t text_color = {{ 1.0f, 1.0f, 1.0f, 1.0f }};

        int x = (window_width  - 640 * MAT_SCALER) / 2;
        int y = (window_height - 480 * MAT_SCALER) / 2;

        gui_begin_rect();
        gui_draw_rect(x, y, 640 * MAT_SCALER, 480 * MAT_SCALER,
                      (color_t) {{ 0.0f, 0.0f, 0.0f, 1.0f }});

        gui_begin_text();
        draw_text_matrix(text_matrix, MAT_WIDTH, MAT_HEIGHT,
                         x, y, FONT_WIDTH * MAT_SCALER, text_color);
#endif

        time += 0.01666f;

        bagE_swapBuffers();
    }
  
    free(pipes.queue);

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
            }
        } break;

        default: break;
    }

    return 0;
}

