#ifndef EDITOR_H
#define EDITOR_H

#include "gui.h"

#include "bag_engine.h"

#define EDITOR_WIDTH  80
#define EDITOR_HEIGHT 20
#define EDITOR_SCALE 2

typedef struct
{
    char *text_buffer;
    int text_size, text_capacity;

    int cursor_index;
    int screen_index;
} editor_t;

void editor_init(editor_t *editor);
void editor_render(editor_t *editor, int x, int y);
void editor_replace(editor_t *editor, int pos,   int dst_size,
                                      char *src, int src_size);

void editor_handle_mouse_button(editor_t *editor, int x, int y,
                                bagE_MouseButton event, bool down);

static inline rect_t editor_area(editor_t *editor, int x, int y)
{
    (void)editor;

    return (rect_t) {
        x, y,
        EDITOR_WIDTH  * 8  * EDITOR_SCALE,
        EDITOR_HEIGHT * 16 * EDITOR_SCALE,
    };
}

#endif // EDITOR_H
