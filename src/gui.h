#ifndef GUI_H
#define GUI_H

#include "linalg.h"

#include <stdbool.h>


typedef struct
{
    int x, y, w, h;
} rect_t;

typedef struct
{
    unsigned fg, bg;
} fgbg_t;


#define MAX_GUI_TEXT_LENGTH (256 * 4 * 4)

typedef struct
{
    unsigned rect_program;
    unsigned text_program;
    unsigned image_program;
    unsigned grid_program;

    unsigned text_font;

    unsigned dummy_vao;

    unsigned grid_text;
    unsigned grid_color;
} gui_t;

extern gui_t gui;


typedef void (*gui_button_callback_t)(void);
typedef bool (*gui_element_callback_t)(int, int, bool);


void init_gui(void);
void exit_gui(void);

void gui_update_resolution(int window_width, int window_height);

void gui_begin_rect(void);
void gui_begin_text(void);
void gui_begin_image(void);
void gui_begin_grid(void);

void gui_use_image(unsigned texture);

void gui_draw_rect(int x, int y, int w, int h, color_t color);
void gui_draw_text(const char *text, int x, int y, int w, int h, int s, color_t color);
void gui_draw_text_n(const char *text, int n,
                     int x, int y, int w, int h, int s, color_t color);
void gui_draw_image_colored(int x, int y, int w, int h,
                            float tx, float ty, float tw, float th,
                            color_t color);
void gui_draw_image(int x, int y, int w, int h,
                    float tx, float ty, float tw, float th);
void gui_draw_grid(const char *text, fgbg_t *colors, int cols, int rows,
                   int x, int y, int w, int h);

#endif
