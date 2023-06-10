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
static vec3_t camera_pos  = { 0.0f, 0.0f, 0.0f };

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

static editor_t editor;


static void render_gui(void)
{
    const char *text = "I like killing animals with my machetay.";

    gui_begin_rect();
    gui_draw_rect(0, 0, window_width, 24, background);

    gui_begin_text();
    gui_draw_text(text, 0, 4, 8, 16, 0, foreground);

    // editor_render(&editor, 100, 100);
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
    char text[] = "\n \\__/\n  \\/\n";
    editor_replace(&editor, 0, 0, text, (int)strlen(text));


    /* expo */
#if 1
    char parse_text[] =
"1 + 2 - 3";

    tokenizer_t toki = {
        .begin = parse_text,
        .pos = parse_text,
        .end = parse_text + length(parse_text) - 1,
    };

    l_system_t sys = {0};

    parse_expr_res_t ret = parse_expression(&toki, &sys, false, 0);

    if (ret.res.success) {
        for (unsigned i = 0; i < ret.expr.count; ++i) {
            fprint_instruction(sys.instructions[ret.expr.index + i], stdout);
        }
    } else {
        printf("%zd:%zd: ", ret.res.line, ret.res.col);
        sv_fwrite(ret.res.message, stdout);
    }
#endif


    /* parso */
#if 0
    char parse_text[] =
"res res_name=cilynder(...)\n"
"res res_blame=model(\"\\\"bruh lol.obj\\\"\")\n"
"\n"
"def type_name(n:int,s:float)={\n"
"    res_name(rotate_x(s)),\n"
"}\n"
"\n"
"# this is a comment\n"
"\n"
"#* multiline\n"
"   comment *# \n"
"\n"
"rule type_name(n>=0)#* shit *#{\n"
"    type_name(n-1,s+PI/4.0),#lol\n"
"    type_name(n-1,s-PI/4.0)\n"
"}\n";

    tokenizer_t toki = {
        .begin = parse_text,
        .pos = parse_text,
        .end = parse_text + length(parse_text) - 1,
    };

    for (;;) {
        token_t token = tokenizer_next(&toki, false);

        printf("(%s, \"", token_type_to_str(token.type));
        sv_fwrite(token.data, stdout);
        printf(", %zd\")\n", token.ln);

        if (token.type == token_type_Empty)
            break;
    }
#endif


    /* systo */
#if 0
    l_system_t sys = {0};

    l_basic_t types[] = { l_basic_Float, l_basic_Int };
    unsigned type = l_system_add_type(&sys, types, length(types));
    printf("type: %d, param_type_count: %d, type_count: %d\n",
           type, sys.param_type_count, sys.type_count);

    l_instruction_t code[] = {
        { .id = l_inst_Value, .op = { .type = l_basic_Int, .data.integer = 3 } },
        { .id = l_inst_Value, .op = { .type = l_basic_Int, .data.integer = 2 } },
        { .id = l_inst_Add },
    };
    l_expr_t expr = l_system_add_code(&sys, code, length(code));
    printf("expr.index: %d, expr.count: %d, instruction_count: %d\n",
           expr.index, expr.count, sys.instruction_count);

    l_value_t val = l_evaluate(&sys, expr, type, 0, true);
    printf("val.type: %s, val.data.integer: %d, val.data.floating: %f\n",
           l_basic_name(val.type), val.data.integer, val.data.floating);

    l_value_t params[] = {
        { .type = l_basic_Float, .data.floating = 1.4f },
        { .type = l_basic_Int,   .data.integer  = 3 },
    };
    l_system_append(&sys, type, params);
    l_system_print(&sys);

    l_instruction_t predicate[] = {
        { .id = l_inst_Value, .op = { .type = l_basic_Bool, .data.boolean = true } },
    };
    l_match_t left = {
        .type = type,
        .predicate = l_system_add_code(&sys, predicate, length(predicate)),
    };
    unsigned param_types[] = { type, type };
    l_instruction_t init_float[] = {
        { .id = l_inst_Value, .op = { .type = l_basic_Float, .data.floating = 0.5 } },
        { .id = l_inst_Param, .op = { .type = l_basic_Int,   .data.integer  = 0 } },
        { .id = l_inst_Add },
    };
    l_instruction_t init_int[] = {
        { .id = l_inst_Param, .op = { .type = l_basic_Int, .data.integer = 1 } },
        { .id = l_inst_Value, .op = { .type = l_basic_Int, .data.integer = 1 } },
        { .id = l_inst_Sub },
    };
    l_expr_t initializer[] = {
        /* 0 */
        l_system_add_code(&sys, init_float, length(init_float)),
        l_system_add_code(&sys, init_int, length(init_int)),
        /* 1 */
        l_system_add_code(&sys, init_float, length(init_float)),
        l_system_add_code(&sys, init_int, length(init_int)),
    };
    l_system_add_rule(&sys, left, param_types, initializer, length(param_types));
    printf("rule_count: %d, result_count: %d, param_count: %d\n",
           sys.rule_count, sys.result_count, sys.param_count);

    l_system_update(&sys);
    l_system_update(&sys);
    l_system_print(&sys);
#endif

    /* crap goes here */
    unsigned texture_program = load_program(
            "shaders/texture.vert.glsl",
            "shaders/texture.frag.glsl"
    );

    texture_data_t textures[] = {
        load_texture_data("res/stone.png"),
        load_texture_data("res/rat.png"),
    };

    rect_t views[length(textures)];

    texture_data_t atlas_data = create_texture_atlas(textures, views, length(textures));
    unsigned atlas_texture = create_texture_object(atlas_data);

    model_data_t cylinder = generate_cylinder(8, frect_make(views[1],
                                                            atlas_data.width,
                                                            atlas_data.height));

    model_data_t sphere = generate_quad_sphere(2, frect_make(views[1],
                                                             atlas_data.width,
                                                             atlas_data.height));

    model_data_t head_model = load_model_data("res/head.model");
    model_map_textures_to_view(&head_model, frect_make(views[0],
                                                       atlas_data.width,
                                                       atlas_data.height));

    model_builder_t builder = {0};

    model_builder_merge(&builder, cylinder,
                        matrix_translation( 0.0f, 0.0f, 0.0f));

    model_builder_merge(&builder, sphere,
                        matrix_translation( 0.0f, 2.0f, 0.0f));

    model_builder_merge(&builder, head_model,
                        matrix_translation( 1.0f, 0.0f, 0.0f));

    model_builder_merge(&builder, head_model,
                        matrix_translation(-1.0f, 0.5f, 0.0f));

    model_object_t main_object = create_model_object(builder.data);

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

        matrix_t model = matrix_multiply(matrix_translation(0.0f,-0.5f,-4.0f),
                                         matrix_rotation_y(model_rot));

        model_rot += dt;

        glUseProgram(texture_program);
        glBindVertexArray(main_object.vao);
        glBindTextureUnit(0, atlas_texture);

        glProgramUniformMatrix4fv(texture_program, 0, 1, false, model.data);

        glDrawElements(GL_TRIANGLES, main_object.index_count, GL_UNSIGNED_INT, NULL);


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
            /*
            bagE_MouseButton mb = event->data.mouseButton;

            if (editor_handle_mouse_button(&editor, 100, 100, mb, down))
                break;
            */

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

                case KEY_SPACE:      { input[input_Up]    = down; } break;
                case KEY_SHIFT_LEFT: { input[input_Down]  = down; } break;
                case KEY_W:          { input[input_Forth] = down; } break;
                case KEY_S:          { input[input_Back]  = down; } break;
                case KEY_A:          { input[input_Left]  = down; } break;
                case KEY_D:          { input[input_Right] = down; } break;
            }

            // editor_handle_key(&editor, *key, down);
        } break;

        case bagE_EventMouseMotion: {
            bagE_MouseMotion mm = event->data.mouseMotion;

            if (free_look) {
                camera_yaw   += mm.x * look_sensitivity;
                camera_pitch += mm.y * look_sensitivity;
            }
        } break;

        case bagE_EventTextUTF8: {
            // editor_handle_utf8(&editor, event->data.textUTF8);
        } break;

        case bagE_EventMousePosition: {
            // bagE_Mouse m = event->data.mouse;
            // editor_handle_mouse_position(&editor, 100, 100, m);
        } break;

        default: break;
    }

    return 0;
}

