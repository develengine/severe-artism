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

    art_text_ctx_t art_text_ctx;
    art_text_init(&art_text_ctx);

    unsigned test_program = load_program("shaders/test.vert.glsl",
                                         "shaders/test.frag.glsl");

    float time = 0.0f;

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

        art_text_update(&art_text_ctx);

        /* rendering */
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* overlay */
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        gui_update_resolution(window_width, window_height);
        glBindVertexArray(gui.dummy_vao);

#if 0
        glUseProgram(test_program);
        glProgramUniform2i(test_program, 0, 0, 0);
        glProgramUniform2i(test_program, 1, window_width, window_height);
        glProgramUniform2i(test_program, 2, window_width, window_height);
        glProgramUniform1f(test_program, 3, time);
        glDrawArrays(GL_TRIANGLES, 0, 6);
#else
        art_text_render(&art_text_ctx, window_width, window_height);
#endif

        time += 0.01666f;

        bagE_swapBuffers();
    }
  
    art_text_free(&art_text_ctx);

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

