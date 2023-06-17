#ifndef EDITOR_H
#define EDITOR_H

#include "gui.h"

#include "bag_engine.h"

#define EDITOR_SCALE 8


typedef struct
{
    char *text_buffer;
    int text_size, text_capacity;

    int cursor_index;
    int screen_index;

    bool mouse_down;
    bool ctrl_down;

    bool selecting;
    int selection_start;

    int col_count, row_count;
} editor_t;

void editor_init(editor_t *editor);
void editor_render(editor_t *editor, int x, int y, int col_count, int row_count);
void editor_replace(editor_t *editor, int pos,   int dst_size,
                                      char *src, int src_size);

bool editor_handle_mouse_button(editor_t *editor, int x, int y,
                                bagE_MouseButton event, bool down);
bool editor_handle_mouse_position(editor_t *editor, int x, int y,
                                  bagE_Mouse event);
bool editor_handle_key(editor_t *editor, bagE_Key event, bool down);
bool editor_handle_utf8(editor_t *editor, bagE_TextUTF8 event);

static inline rect_t editor_area(editor_t *editor, int x, int y)
{
    (void)editor;

    return (rect_t) {
        x, y,
        editor->col_count * 1 * EDITOR_SCALE,
        editor->row_count * 2 * EDITOR_SCALE,
    };
}

#endif // EDITOR_H
