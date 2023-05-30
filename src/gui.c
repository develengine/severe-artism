#include "gui.h"

#include "bag_engine.h"
#include "res.h"
#include "core.h"
#include "utils.h"

gui_t gui;


static char text_scratch[MAX_GUI_TEXT_LENGTH];

#define DEFAULT_GRID_CAPACITY 4
static char *grid_text_scratch;
static fgbg_t *grid_color_scratch;
static int grid_capacity = DEFAULT_GRID_CAPACITY;


static void init_grid_buffers(void)
{
    grid_text_scratch = malloc(grid_capacity);
    malloc_check(grid_text_scratch);

    grid_color_scratch = malloc(grid_capacity * sizeof(fgbg_t));
    malloc_check(grid_color_scratch);

    glCreateBuffers(1, &gui.grid_text);
    glNamedBufferStorage(gui.grid_text, grid_capacity, NULL,
                         GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &gui.grid_color);
    glNamedBufferStorage(gui.grid_color, grid_capacity * sizeof(fgbg_t), NULL,
                         GL_DYNAMIC_STORAGE_BIT);
}


static void free_grid_buffers(void)
{
    glDeleteBuffers(1, &gui.grid_text);
    glDeleteBuffers(1, &gui.grid_color);
    free(grid_text_scratch);
    free(grid_color_scratch);
}


void init_gui(void)
{
    glCreateVertexArrays(1, &gui.dummy_vao);

    gui.rect_program = load_program(
            "shaders/rect_vertex.glsl",
            "shaders/rect_fragment.glsl"
    );

    gui.text_program = load_program(
            "shaders/text_vertex.glsl",
            "shaders/text_fragment.glsl"
    );

    gui.image_program = load_program(
            "shaders/image_vertex.glsl",
            "shaders/image_fragment.glsl"
    );

    gui.grid_program = load_program(
            "shaders/text_grid.vertex.glsl",
            "shaders/text_grid.fragment.glsl"
    );

    gui.text_font = load_texture("res/font_base.png");

    init_grid_buffers();
}


void exit_gui(void)
{
    glDeleteVertexArrays(1, &gui.dummy_vao);

    glDeleteProgram(gui.rect_program);
    glDeleteProgram(gui.text_program);
    glDeleteProgram(gui.image_program);
    glDeleteProgram(gui.grid_program);

    glDeleteTextures(1, &gui.text_font);

    free_grid_buffers();
}


void gui_update_resolution(int window_width, int window_height)
{
    glProgramUniform2i(gui.rect_program,  0, window_width, window_height);
    glProgramUniform2i(gui.text_program,  0, window_width, window_height);
    glProgramUniform2i(gui.image_program, 0, window_width, window_height);
    glProgramUniform2i(gui.grid_program,  0, window_width, window_height);
}


void gui_begin_rect(void)
{
    glUseProgram(gui.rect_program);
}


void gui_begin_text(void)
{
    glUseProgram(gui.text_program);
    glBindTextureUnit(0, gui.text_font);
}


void gui_begin_image(void)
{
    glUseProgram(gui.image_program);
}


void gui_use_image(unsigned texture)
{
    glBindTextureUnit(0, texture);
}


void gui_begin_grid(void)
{
    glUseProgram(gui.grid_program);
    glBindTextureUnit(0, gui.text_font);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gui.grid_text);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gui.grid_color);
}


void gui_draw_rect(int x, int y, int w, int h, color_t color)
{
    glProgramUniform2i(gui.rect_program, 1, x, y);
    glProgramUniform2i(gui.rect_program, 2, w, h);
    glProgramUniform4fv(gui.rect_program, 3, 1, color.data);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void gui_draw_text_n(const char *text, int n,
                     int x, int y, int w, int h, int s, color_t color)
{
    for (int i = 0; i < n; ++i)
        text_scratch[i] = text[i];

    int word_count = n / 16 + (n % 16 != 0);

    glProgramUniform2i(gui.text_program, 1, x, y);
    glProgramUniform2i(gui.text_program, 2, w, h);
    glProgramUniform2i(gui.text_program, 3, s, s);
    glProgramUniform4fv(gui.text_program, 4, 1, color.data);
    glProgramUniform4uiv(gui.text_program, 5, word_count, (unsigned *)text_scratch);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, n);
}


void gui_draw_text(const char *text, int x, int y, int w, int h, int s, color_t color)
{
    int text_len = 0;
    for (; text[text_len] != '\0'; ++text_len)
        text_scratch[text_len] = text[text_len];

    int word_count = text_len / 16 + (text_len % 16 != 0);

    glProgramUniform2i(gui.text_program, 1, x, y);
    glProgramUniform2i(gui.text_program, 2, w, h);
    glProgramUniform2i(gui.text_program, 3, s, s);
    glProgramUniform4fv(gui.text_program, 4, 1, color.data);
    glProgramUniform4uiv(gui.text_program, 5, word_count, (unsigned *)text_scratch);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, text_len);
}


void gui_draw_grid(const char *text, fgbg_t *colors, int cols, int rows,
                   int x, int y, int w, int h)
{
    int count = cols * rows;

    if (count > grid_capacity) {
        while (count > grid_capacity) {
            grid_capacity *= 2;
        }

        free_grid_buffers();
        init_grid_buffers();
    }

    for (int i = 0; i < count; ++i) {
        grid_text_scratch[i] = text[i];
    }

    for (int i = 0; i < count; ++i) {
        grid_color_scratch[i] = colors[i];
    }

    glNamedBufferSubData(gui.grid_text,  0, count, grid_text_scratch);
    glNamedBufferSubData(gui.grid_color, 0, count * sizeof(fgbg_t), grid_color_scratch);

    glProgramUniform2i(gui.grid_program, 1, x, y);
    glProgramUniform2i(gui.grid_program, 2, w, h);
    glProgramUniform2i(gui.grid_program, 3, cols, rows);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
}


void gui_draw_image_colored(int x, int y, int w, int h,
                            float tx, float ty, float tw, float th,
                            color_t color)
{
    glProgramUniform2i(gui.image_program, 1, x, y);
    glProgramUniform2i(gui.image_program, 2, w, h);
    glProgramUniform2f(gui.image_program, 3, tx, ty);
    glProgramUniform2f(gui.image_program, 4, tw, th);
    glProgramUniform4f(gui.image_program, 5, color.r, color.g, color.b, color.a);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void gui_draw_image(int x, int y, int w, int h,
                    float tx, float ty, float tw, float th)
{
    color_t color = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
    gui_draw_image_colored(x, y, w, h, tx, ty, tw, th, color);
}
