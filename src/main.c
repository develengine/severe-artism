/* TODO:
 * [ ] Properly compute normal matrices when merging in model builder.
 * [ ] Add mouse scroll to the text editor.
 * [ ] Add errors to editor.
 * [ ] Add syntax highlighting to the text editor.
 * [ ] Make the text editor retractable.
 * [ ] Add way to export programs.
 */

#include "bag_engine.h"
#include "bag_keys.h"
#include "bag_time.h"

#include "utils.h"
#include "linalg.h"
#include "res.h"
#include "core.h"
#include "gui.h"

#include "editor.h"
#include "generator.h"
#include "l_system.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


static bool running = true;

static int window_width  = 1080;
static int window_height = 720;

static bool f11_down   = false;
static bool fullscreen = false;

static float fov = 90.0f;

static float camera_pitch = 0.0f;
static float camera_yaw   = 0.0f;
static vec3_t camera_pos  = { 0.0f, 0.0f, 10.0f };

static bool free_look = false;
static float look_sensitivity = 0.002f;

typedef enum
{
    input_Up,
    input_Down,
    input_Left,
    input_Right,
    input_Forth,
    input_Back,

    INPUT_COUNT
} input_t;

static bool input[INPUT_COUNT] = {0};


static color_t foreground = { 1.0f, 1.0f, 1.0f, 1.0f };
static color_t background = { 0.2f, 0.2f, 0.2f, 0.7f };


static editor_t editor = {0};
static l_system_t l_system = {0};

static parse_state_t parse_state = {0};

static bool has_model = false;
static model_object_t model_object;


static void try_compile(void)
{
    has_model = false;

    parse(&l_system, &parse_state, editor.text_buffer, editor.text_size);

    if (!parse_state.res.success) {
        printf("%zd:%zd ", parse_state.res.line, parse_state.res.col);
        sv_fwrite(parse_state.res.message, stdout);
        printf("\n");
        return;
    }

    /* iterate the system */
    for (int i = 0; i < 8; ++i) {
        char *error = l_system_update(&l_system);
        if (error) {
            printf("runtime error: %s\n", error);
        }
    }

    l_build_t build = l_system_build(&l_system);

    if (build.error) {
        printf("build error: %s\n", build.error);
        return;
    }

    model_object = build.object;
    has_model = true;
}



static void render_gui(void)
{
    editor_render(&editor, 0, 0);
}


int bagE_main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    bagT_init();

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(opengl_callback, 0);

    print_context_info();

    bagE_setWindowTitle("severe artism");

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


    editor_init(&editor);
    char text[] =
"#* Hi, this is a barely working proof of concept...\n"
" *\n"
" * You can click on this editor here to change the code.\n"
" * If the code fails to compile an error message will be shown\n"
" * in the stdout 'command prompt'.\n"
" *\n"
" * You can also click outside, then you can move around like in minecraft\n"
" * creative mode, (w, a, s, d, shift, space and mouse).\n"
" * Whilst in this mode you can press `c` to recompile the code.\n"
" * Press 'esc' to gain back cursor. *#\n"
"\n"
"tex stone_tex(\"res/stone.png\")\n"
"tex rat_tex(\"res/rat.png\")\n"
"\n"
"mod head_mod(\"res/head.obj\")\n"
"\n"
"res head = object(head_mod, stone_tex)\n"
"res hair = sphere(3, rat_tex)\n"
"\n"
"def symbol_1(n: int) {\n"
"    head(position(float(n), 0.0, 0.0))\n"
"    hair(position(float(n), 2.5, -0.25))\n"
"}\n"
"\n"
"# this is a rewriting rule, it matches\n"
"# by the specified type `symbol_1` and an expression `true`\n"
"# the expression can depend on the above defined parameters\n"
"\n"
"rule symbol_1(true) {\n"
"    symbol_1(n)\n"
"    symbol_1(n * 2)\n"
"}\n"
"\n"
"# this way you insert starting symbols into the system\n"
"# the system will iterate 8 times, (for now fixed number)\n"
"\n"
"symbol_1(1)\n"
"\n"
;
    editor_replace(&editor, 0, 0, text, (int)strlen(text));

    try_compile();

    /* crap goes here */
    unsigned texture_program = load_program(
            "shaders/texture.vert.glsl",
            "shaders/texture.frag.glsl"
    );

    unsigned cam_ubo = create_buffer_object(
        sizeof(matrix_t) * 3 + sizeof(float) * 4,
        NULL,
        GL_DYNAMIC_STORAGE_BIT
    );

    unsigned env_ubo = create_buffer_object(
        sizeof(float) * 4 * 4,
        NULL,
        GL_DYNAMIC_STORAGE_BIT
    );

    glActiveTexture(GL_TEXTURE0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cam_ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, env_ubo);

    struct {
        float ambient[4];
        float to_light[4];
        float sun_color[4];
    } env_data = {
        .ambient   = { 0.1f, 0.2f, 0.3f, 1.0f },
        .to_light  = { 0.6f, 0.7f, 0.5f },
        .sun_color = { 1.0f, 1.0f, 1.0f },
    };

    glNamedBufferSubData(env_ubo, 0, sizeof(env_data), &env_data);

    float model_rot = 0.0f;


    int64_t cumm = 0;
    int64_t first = bagT_getTime();
    int frame_count = 0;

    float dt = 0.01666f;

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

        if (free_look) {
            float vx = 0.0f, vz = 0.0f;
            float speed = 3.0f;

            if (input[input_Left]) {
                vx -= 0.1f * cosf(camera_yaw);
                vz -= 0.1f * sinf(camera_yaw);
            }

            if (input[input_Right]) {
                vx += 0.1f * cosf(camera_yaw);
                vz += 0.1f * sinf(camera_yaw);
            }

            if (input[input_Forth]) {
                vx += 0.1f * sinf(camera_yaw);
                vz -= 0.1f * cosf(camera_yaw);
            }

            if (input[input_Back]) {
                vx -= 0.1f * sinf(camera_yaw);
                vz += 0.1f * cosf(camera_yaw);
            }

            if (input[input_Up]) {
                camera_pos.y += 0.1f * speed;
            }

            if (input[input_Down]) {
                camera_pos.y -= 0.1f * speed;
            }

            camera_pos.x += vx * speed;
            camera_pos.z += vz * speed;
        }


        /* rendering */
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);


        matrix_t view = matrix_multiply(
            matrix_rotation_x(camera_pitch),
            matrix_multiply(
                matrix_rotation_y(camera_yaw),
                matrix_translation(-camera_pos.x, -camera_pos.y, -camera_pos.z)
            )
        );

        matrix_t proj = matrix_projection(
            fov,
            (float)window_width,
            (float)window_height,
            0.1f,
            100.0f
        );

        matrix_t vp = matrix_multiply(proj, view);

        struct {
            matrix_t mats[3];
            float pos[4];
        } cam_data = {
            .mats = { view, proj, vp },
            .pos  = { camera_pos.x, camera_pos.y, camera_pos.z }
        };

        glNamedBufferSubData(cam_ubo, 0, sizeof(cam_data), &cam_data);

        matrix_t model = matrix_identity();

        model_rot += dt;

        if (has_model) {
            glUseProgram(texture_program);
            glBindVertexArray(model_object.vao);
            glBindTextureUnit(0, l_system.atlas_texture);

            glProgramUniformMatrix4fv(texture_program, 0, 1, false, model.data);

            glDrawElements(GL_TRIANGLES, model_object.index_count, GL_UNSIGNED_INT, NULL);
        }

        /* overlay */
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBindVertexArray(gui.dummy_vao);

        render_gui();

        gui_update_resolution(window_width, window_height);
        glBindVertexArray(gui.dummy_vao);

        bagE_swapBuffers();

        int64_t second = bagT_getTime();
        int64_t diff = second - first;
        first = second;

        dt = (float)diff / bagT_getFreq();
        cumm += diff;
        ++frame_count;

        if (cumm >= bagT_getFreq()) {
            cumm %= bagT_getFreq();

            char buffer[64] = {0};
            snprintf(buffer, sizeof(buffer), "severe artism, FPS: %d\n", frame_count);
            bagE_setWindowTitle(buffer);

            frame_count = 0;
        }
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

        case bagE_EventMouseButtonDown:
            down = true;
            /* fallthrough */
        case bagE_EventMouseButtonUp: {
            bagE_MouseButton mb = event->data.mouseButton;

            if (!free_look && editor_handle_mouse_button(&editor, 0, 0, mb, down))
                break;

            if (!down && !free_look) {
                free_look = !free_look;
                bagE_setHiddenCursor(free_look);
            }
        } break;

        case bagE_EventKeyDown: 
            down = true;
            /* fallthrough */
        case bagE_EventKeyUp: {
            bagE_Key *key = &(event->data.key);
            switch (key->key) {
                case KEY_F11: {
                    if (down) {
                        if(!f11_down) {
                            f11_down = true;
                            fullscreen = !fullscreen;
                            bagE_setFullscreen(fullscreen);
                        }
                    } else {
                        f11_down = false;
                    }
                } break;

                case KEY_ESCAPE: {
                    if (!down && free_look) {
                        free_look = false;
                        bagE_setHiddenCursor(false);
                    }
                } break;

                case KEY_C: {
                    if (!down && free_look) {
                        try_compile();
                    }
                } break;

                case KEY_SPACE:      { input[input_Up]    = down; } break;
                case KEY_SHIFT_LEFT: { input[input_Down]  = down; } break;
                case KEY_W:          { input[input_Forth] = down; } break;
                case KEY_S:          { input[input_Back]  = down; } break;
                case KEY_A:          { input[input_Left]  = down; } break;
                case KEY_D:          { input[input_Right] = down; } break;
            }

            editor_handle_key(&editor, *key, down);
        } break;

        case bagE_EventMouseMotion: {
            bagE_MouseMotion mm = event->data.mouseMotion;

            if (free_look) {
                camera_yaw   += mm.x * look_sensitivity;
                camera_pitch += mm.y * look_sensitivity;
            }
        } break;

        case bagE_EventTextUTF8: {
            if (!free_look) {
                editor_handle_utf8(&editor, event->data.textUTF8);
            }
        } break;

        case bagE_EventMousePosition: {
            bagE_Mouse m = event->data.mouse;
            if (!free_look) {
                editor_handle_mouse_position(&editor, 0, 0, m);
            }
        } break;

        default: break;
    }

    return 0;
}

