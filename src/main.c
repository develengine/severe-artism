/* TODO:
 * [X] Properly compute normal matrices when merging in model builder.
 * [X] Redo scale function and add stretch function.
 * [X] Make the text editor resizable.
 * [X] Add syntax highlighting to the text editor.
 * [X] Add a way to export the model and texture.
 * [X] Do proper string extraction.
 * [X] Add a way to change iteration count.
 * [X] Add errors to editor.
 * [X] Fix paste cr bug.
 * [X] Add way to export code.
 *
 * Can wait, lol:
 *
 * [ ] Fix token error column numbers.
 * [ ] Add mouse scroll to the text editor.
 * [ ] Properly handle backspace and delete with selection.
 * [ ] Add tab indentation.
 * [ ] Make the text editor retractable.
 * [ ] Add scroll wheel acceleration.
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

#define ERROR_MESSAGE_CAPACITY 256
static char error_message_buffer[ERROR_MESSAGE_CAPACITY] = {0};

static bool has_model = false;
static model_builder_t builder = {0};
static model_object_t model_object;

static int iteration_count = 8;


static void try_rebuild(void)
{
    builder.data.vertex_count = 0;
    builder.data.index_count  = 0;

    if (has_model) {
        free_model_object(model_object);
        has_model = false;
    }

    /* iterate the system */
    for (int i = 0; i < iteration_count; ++i) {
        char *error = l_system_update(&l_system);
        if (error) {
            snprintf(error_message_buffer, ERROR_MESSAGE_CAPACITY,
                    "runtime error: %s\n", error);
        }
    }

    l_build_t build = l_system_build(&l_system, &builder);

    if (build.error) {
        snprintf(error_message_buffer, ERROR_MESSAGE_CAPACITY,
                "build error: %s\n", build.error);
        return;
    }

    model_object = create_model_object(builder.data);

    has_model = true;
}


static void try_compile(void)
{
    memset(error_message_buffer, 0, ERROR_MESSAGE_CAPACITY);

    if (has_model) {
        free_model_object(model_object);
        has_model = false;
    }

    parse(&l_system, &parse_state, editor.text_buffer, editor.text_size);

    if (!parse_state.res.success) {
        int i = snprintf(error_message_buffer, ERROR_MESSAGE_CAPACITY,
                        "%zd:%zd ", parse_state.res.line, parse_state.res.col);
        for (char *p = parse_state.res.message.begin; p != parse_state.res.message.end; ++p, ++i) {
            error_message_buffer[i] = *p;
        }

        return;
    }

    try_rebuild();
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

    color_t bg = color_from_uint(0xAA000000);
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


static inline bool im_label(int x, int y, int w, int h,
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

    int rel_x = im.mouse.x - x;
    int rel_y = im.mouse.y - y;

    return rel_x >= 0 && rel_x < w && rel_y >= 0 && rel_y < h;
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


    int path_len = (int)strlen(buffer);
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
    im.hot_id = -1;
    int id = -1;
    char *tool_tip = NULL;

    /* Text editor and Errors */
    {
        color_t bg = color_from_uint(0xAA000000);
        color_t fg = { 1.0f,  1.0f,  1.0f, 1.0f };

        int err_min_height = 64;
        int editor_rows = (window_height - err_min_height) / (EDITOR_SCALE * 2);

        editor_render(&editor, 0, 0, 81, editor_rows);

        int err_y = editor_rows * EDITOR_SCALE * 2;
        int gap = 4;

        char *text = "Compilation successful!";
        if (error_message_buffer[0]) {
            text = error_message_buffer;
        }

        if (im_label(0, err_y + gap, 81 * EDITOR_SCALE,
                     window_height - err_y - gap, 1, fg, bg, text))
        {
            im.hot_id = -2;
        }
    }

    int butt_marg = 25;
    int butt_gap = 10;
    int butt_w = 200;
    int butt_h = 50;
    int butt_x = window_width - butt_w - butt_marg;
    int butt_y = butt_marg;

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

    butt_y += butt_h + butt_gap;

    int butt_slim = butt_w / 8;

    if (im_button(++id, butt_x, butt_y, butt_slim, butt_h, "<")) {
        if (iteration_count > 0) {
            --iteration_count;
        }

        // TODO: Implement rebuild so that we don't have to recompile the whole system again.
        try_compile();
    }

    if (im_button(++id, butt_x + butt_slim * 7, butt_y, butt_slim, butt_h, ">")) {
        ++iteration_count;

        // TODO: Implement rebuild so that we don't have to recompile the whole system again.
        try_compile();
    }

    {
        char iter_buffer[16] = { '0' };

        int iter_count = iteration_count;

        int idx = 0;
        for (; iter_count; ++idx) {
            int dec = iter_count % 10;
            iter_count /= 10;
            iter_buffer[idx] = '0' + (char)dec;
        }

        for (int i = 0; i < idx  / 2; ++i) {
            char tmp = iter_buffer[i];
            iter_buffer[i] = iter_buffer[idx - i - 1];
            iter_buffer[idx - i - 1] = tmp;
        }

        color_t bg = color_from_uint(0xAA000000);
        color_t fg = { 1.0f,  1.0f,  1.0f, 1.0f };

        if (im_label(butt_x + butt_slim + butt_gap, butt_y,
                     butt_slim * 6 - butt_gap * 2, butt_h,
                     2, fg, bg, iter_buffer))
        {
            tool_tip = "Iteration Count";
            im.hot_id = -2;
        }
    }

    butt_y += butt_h + butt_gap;

    int save_to_clip_id = ++id;
    if (im_button(save_to_clip_id, butt_x, butt_y, butt_w, butt_h, "copy code")) {
        bagE_clipCopy(editor.text_buffer, editor.text_size);
        printf("Copied\n");
    }

    if (im.hot_id == save_to_clip_id) {
        tool_tip = "Copies whole source code to clipboard.";
    }

    butt_y += butt_h + butt_gap;

    int clear_id = ++id;
    if (im_button(clear_id, butt_x, butt_y, butt_w, butt_h, "clear code")) {
        editor.text_size = 0;
        editor.cursor_index = 0;
        editor.screen_index = 0;
        editor.selecting = false;
        editor.selection_start = 0;

        printf("Cleared\n");
    }

    if (im.hot_id == clear_id) {
        tool_tip = "Removes current code.";
    }

    if (tool_tip) {
        color_t fg = { 1.0f, 1.0f, 1.0f, 1.0f };
        color_t bg = { 0.0f, 0.0f, 0.0f, 0.75f };
        int size = (int)strlen(tool_tip);

        int tip_x = im.mouse.x + 16;
        int tip_w = size * 8 + 8;

        if (tip_x + tip_w > window_width) {
            tip_x -= size * 8 + 32;
        }

        im_label(tip_x, im.mouse.y, tip_w, 24, 1, fg, bg, tool_tip);
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

    bagE_setWindowTitle("LLL");

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
"#* https://github.com/develengine/severe-artism\n"
"   author: Jakub Boros *#\n"
"\n"
"tex stone_tex(\"res/stone.png\")\n"
"tex rat_tex(\"res/rat.png\")\n"
"\n"
"mod head_mod(\"res/head.obj\")\n"
"\n"
"res head = object(head_mod, stone_tex)\n"
"res hair = sphere(3, rat_tex)\n"
"\n"
"def brug(n: int, end: bool) {\n"
"    head(position(float(n), 0.0, 0.0) * scale(n * 0.75))\n"
"    hair(position(float(n), 2.5 * n * 0.75, -0.25 * n * 0.75) * scale(n * 0.75))\n"
"}\n"
"\n"
"rule brug(!end) {\n"
"    brug(n, true)\n"
"    brug(n * 2, false)\n"
"}\n"
"\n"
"rule brug(end) {\n"
"    brug(n, end)\n"
"}\n"
"\n"
"brug(1, false)\n"
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
        glClearColor(0.2f, 0.6f, 1.0f, 1.0f);
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
            snprintf(buffer, sizeof(buffer), "LLL, FPS: %d\n", frame_count);
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

            if (down && !free_look && im.hot_id == -1) {
                free_look = !free_look;
                bagE_setHiddenCursor(free_look);
                break;
            }

            if (free_look && down) {
                free_look = false;
                bagE_setHiddenCursor(false);
                break;
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

                case KEY_E: {
                    if (ctrl_down) {
                        export();
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

