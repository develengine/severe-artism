/* TODO:
 * [X] Properly compute normal matrices when merging in model builder.
 * [X] Redo scale function and add stretch function.
 * [X] Make the text editor resizable.
 * [X] Add syntax highlighting to the text editor.
 * [ ] Add scroll wheel acceleration.
 * [ ] Properly handle backspace and delete with selection.
 * [ ] Add a way to export the model and texture.
 * [ ] Add a way to change iteration count.
 * [ ] Add mouse scroll to the text editor.
 * [ ] Add errors to editor.
 * [ ] Make the text editor retractable.
 * [ ] Add way to export code.
 * [ ] Do proper string extraction.
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
#include "obj_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


static bool running = true;

static int window_width  = 1080;
static int window_height = 720;

// TODO: Get rid of these state based modifier key checks.
static bool f11_down   = false;
static bool fullscreen = false;

// TODO: Get rid of these state based modifier key checks.
static bool ctrl_down = false;

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
static model_builder_t builder = {0};
static model_object_t model_object;



static void try_compile(void)
{
    builder.data.vertex_count = 0;
    builder.data.index_count  = 0;

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

    l_build_t build = l_system_build(&l_system, &builder);

    if (build.error) {
        printf("build error: %s\n", build.error);
        return;
    }

    model_object = create_model_object(builder.data);

    has_model = true;
}


typedef struct
{
    ivec2_t mouse;
    bool mouse_down, mouse_clicked;

    int hot_id;

} im_t;

static im_t im = {0};


static inline bool im_button(int id, int x, int y, int w, int h, char *text)
{
    int rel_x = im.mouse.x - x;
    int rel_y = im.mouse.y - y;

    if (rel_x >= 0 && rel_x < w
     && rel_y >= 0 && rel_y < h) {
        im.hot_id = id;
    }

    color_t bg = { 0.75f, 0.75f, 1.0f, 0.5f };
    color_t fg = { 1.0f,  1.0f,  1.0f, 1.0f };

    bool was_clicked = im.hot_id == id && im.mouse_down && im.mouse_clicked;

    if (was_clicked) {
        bg = (color_t) { 0.0f, 0.0f, 0.0f, 1.0f };
        fg = (color_t) { 1.0f, 1.0f, 1.0f, 1.0f };
    }
    else if (im.hot_id == id) {
        bg = (color_t) { 1.0f, 1.0f, 1.0f, 1.0f };
        fg = (color_t) { 0.0f, 0.0f, 0.0f, 1.0f };
    }

    gui_begin_rect();
    gui_draw_rect(x, y, w, h, bg);

    int text_size = (int)strlen(text);

    int scale = 2;
    int width = text_size * 8 * scale;
    int height = 16 * scale;

    gui_begin_text();
    gui_draw_text_n(text, text_size,
                    x + (w - width) / 2, y + (h - height) / 2,
                    8 * scale, 16 * scale,
                    0, fg);

    if (was_clicked) {
        return true;
    }

    return false;
}


static inline void im_label(int x, int y, int w, int h,
                            int scale, color_t fg, color_t bg, char *text)
{
    gui_begin_rect();
    gui_draw_rect(x, y, w, h, bg);

    int text_size = (int)strlen(text);

    int width = text_size * 8 * scale;
    int height = 16 * scale;

    gui_begin_text();
    gui_draw_text_n(text, text_size,
                    x + (w - width) / 2, y + (h - height) / 2,
                    8 * scale, 16 * scale,
                    0, fg);

}


static void export(void)
{
    if (!has_model) {
        printf("No model to export!\n");
        return;
    }

    char buffer[512] = {0};
    strcpy(buffer, "output");

    OPENFILENAMEA param = {
        .lpstrFile = buffer,
        .nMaxFile  = sizeof(buffer) - 5, // extension space and null byte
    };

    param.lStructSize = sizeof(param);

    if (!GetSaveFileNameA(&param))
        return;


    int path_len = strlen(buffer);
    char *ext_obj = ".obj";

    for (int i = 0; ext_obj[i]; ++i) {
        buffer[path_len + i] = ext_obj[i];
    }

    model_data_export_to_obj_file(builder.data, buffer);


    char *ext_png = ".png";

    for (int i = 0; ext_png[i]; ++i) {
        buffer[path_len + i] = ext_png[i];
    }

    stbi_flip_vertically_on_write(1);

    if (!stbi_write_png(buffer,
                        l_system.atlas.width,
                        l_system.atlas.height,
                        4,
                        l_system.atlas.data,
                        l_system.atlas.width * 4))
    {
        fprintf(stderr, "Failed to export atlas image!\n");
    }
}


static void render_gui(void)
{
    editor_render(&editor, 0, 0, 81, window_height / (EDITOR_SCALE * 2));


    im.hot_id = -1;

    int id = -1;

    int butt_marg = 25;
    int butt_gap = 10;
    int butt_w = 200;
    int butt_h = 50;
    int butt_x = window_width - butt_w - butt_marg;
    int butt_y = butt_marg;

    char *tool_tip = NULL;

    int recompile_id = ++id;
    if (im_button(recompile_id, butt_x, butt_y, butt_w, butt_h, "recompile")) {
        try_compile();
    }

    if (im.hot_id == recompile_id) {
        tool_tip = "(ctrl + r)";
    }

    butt_y += butt_h + butt_gap;

    int export_id = ++id;
    if (im_button(export_id, butt_x, butt_y, butt_w, butt_h, "export")) {
        export();
    }

    if (im.hot_id == export_id) {
        tool_tip = "(ctrl + e)";
    }

    if (tool_tip) {
        color_t fg = { 1.0f, 1.0f, 1.0f, 1.0f };
        color_t bg = { 0.0f, 0.0f, 0.0f, 0.75f };
        int size = (int)strlen(tool_tip);
        im_label(im.mouse.x + 16, im.mouse.y, size * 8 + 8, 24, 1, fg, bg, tool_tip);
    }

    im.mouse_clicked = false;
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
"    head(position(float(n), 0.0, 0.0) * scale(n * 0.75))\n"
"    hair(position(float(n), 2.5 * n * 0.75, -0.25 * n * 0.75) * scale(n * 0.75))\n"
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
            500.0f
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

            im.mouse_clicked = down != im.mouse_down;
            im.mouse_down = down;

            if (!free_look && im.hot_id == -1
             && editor_handle_mouse_button(&editor, 0, 0, mb, down))
                break;

            if (!down && !free_look && im.hot_id == -1) {
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

                case KEY_CONTROL_LEFT: {
                    ctrl_down = down;
                } break;

                case KEY_R: {
                    if (ctrl_down || (!down && free_look)) {
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

            im.mouse.x = event->data.mouse.x;
            im.mouse.y = event->data.mouse.y;

            if (!free_look && im.hot_id == -1) {
                editor_handle_mouse_position(&editor, 0, 0, m);
            }
        } break;

        default: break;
    }

    return 0;
}

