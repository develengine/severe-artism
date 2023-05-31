#include "editor.h"

#include "utils.h"
#include "linalg.h"

#define EDITOR_SIZE (EDITOR_WIDTH * EDITOR_HEIGHT)

static char    text_grid[EDITOR_SIZE];
static fgbg_t color_grid[EDITOR_SIZE];


static ivec2_t index_to_coords(editor_t *editor, int index)
{
    int curr_x = -1, curr_y = 0;
    int last_line = -1;

    if (index < editor->screen_index)
        return (ivec2_t) { -1, -1 };

    for (int i = editor->screen_index; i < editor->text_size; ++i) {
        if (i == editor->cursor_index) {
            curr_x = i - last_line - 1;
            break;
        }

        if (editor->text_buffer[i] == '\n') {
            ++curr_y;
            last_line = i;
        }
    }

    if (editor->cursor_index == editor->text_size) {
        curr_x = editor->text_size - last_line - 1;
    }

    return (ivec2_t) { curr_x, curr_y };
}

static int coords_to_index(editor_t *editor, ivec2_t coords)
{
    int curr_y = 0;
    int index = editor->screen_index;

    for (; index < editor->text_size && curr_y != coords.y; ++index) {
        if (editor->text_buffer[index] == '\n') {
            ++curr_y;
        }
    }

    if (curr_y < coords.y)
        return editor->text_size;

    int i = 0;

    for (; i < coords.x && index + i < editor->text_size; ++i) {
        if (editor->text_buffer[index + i] == '\n')
            break;
    }

    return index + i;
}


void editor_replace(editor_t *editor, int pos,   int dst_size,
                                      char *src, int src_size)
{
    int new_len = editor->text_size - dst_size + src_size;

    if (new_len > editor->text_capacity) {
        int new_capacity = editor->text_capacity ? editor->text_capacity * 2
                                                    : 4096;
        while (new_len > new_capacity) {
            new_capacity *= 2;
        }

        // FUN-FACT: It took me 5 years to read the realloc documentation and
        //           realize that realloc already does the allocating and moving
        //           of a new buffer in case the original can't be resized...
        editor->text_buffer = realloc(editor->text_buffer, new_capacity);
        malloc_check(editor->text_buffer);

        editor->text_capacity = new_capacity;
    }

    memmove(editor->text_buffer + pos + dst_size,
            editor->text_buffer + pos + src_size,
            editor->text_size - pos - dst_size);

    memcpy(editor->text_buffer + pos, src, src_size);

    editor->text_size = new_len;
}


void editor_init(editor_t *editor)
{
    *editor = (editor_t) {0};
}

void editor_render(editor_t *editor, int x_pos, int y_pos)
{
    ivec2_t curr = index_to_coords(editor, editor->cursor_index);

    /* populate grids with data */
    int text_pos = 0;

    for (int y = 0; y < EDITOR_HEIGHT; ++y) {
        int x = 0;

        for (; x < EDITOR_WIDTH && text_pos < editor->text_size; ++x) {
            char c = editor->text_buffer[text_pos++];

            if (c == '\n')
                break;

            text_grid[x + y * EDITOR_WIDTH] = c;
        }

        memset(text_grid + x + y * EDITOR_WIDTH, ' ', EDITOR_WIDTH - x);
    }

    for (int y = 0; y < EDITOR_HEIGHT; ++y) {
        for (int x = 0; x < EDITOR_WIDTH; ++x) {
            color_grid[x + y * EDITOR_WIDTH] = (fgbg_t) {
                .fg = 0xFFFFFFFF,
                .bg = 0xAA000000,
            };
        }
    }

    if (curr.x != -1 && curr.y != -1) {
        color_grid[curr.x + curr.y * EDITOR_WIDTH] = (fgbg_t) {
            .fg = 0xAA000000,
            .bg = 0xFFFFFFFF,
        };
    }

    /* render grids */
    gui_begin_grid();
    gui_draw_grid(text_grid, color_grid, EDITOR_WIDTH, EDITOR_HEIGHT,
                  x_pos, y_pos, 8 * EDITOR_SCALE, 16 * EDITOR_SCALE);
}

void editor_handle_mouse_button(editor_t *editor, int x, int y,
                                bagE_MouseButton event, bool down)
{
    if (!down)
        return;

    ivec2_t coords = {
        (event.x - x) / (8  * EDITOR_SCALE),
        (event.y - y) / (16 * EDITOR_SCALE),
    };

    int index = coords_to_index(editor, coords);

    editor->cursor_index = index;
}
