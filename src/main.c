#include "bag_engine.h"
#include "bag_keys.h"
#include "utils.h"
#include "linalg.h"
#include "res.h"
#include "core.h"
#include "gui.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


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

    init_gui();

    unsigned ref_program = load_program("shaders/ref.vert.glsl",
                                        "shaders/ref.frag.glsl");

    float time = 0.0f;

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

        /* rendering */
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* overlay */
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        gui_update_resolution(window_width, window_height);
        glBindVertexArray(gui.dummy_vao);

        glUseProgram(ref_program);
        glProgramUniform2i(ref_program, 0, 0, 0);
        glProgramUniform2i(ref_program, 1, window_width, window_height);
        glProgramUniform2i(ref_program, 2, window_width, window_height);
        glProgramUniform1f(ref_program, 3, time);
        glProgramUniform1f(ref_program, 4, 90.0f);
        glProgramUniform3f(ref_program, 5, 0.0f, 0.0f, 0.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (flow_time)
            time += 0.01666f;

        bagE_swapBuffers();
    }
  
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

