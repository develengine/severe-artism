#include "editor.h"

#include "utils.h"
#include "linalg.h"

#include "bag_keys.h"

#define EDITOR_SIZE (EDITOR_WIDTH * EDITOR_HEIGHT)

static char    text_grid[EDITOR_SIZE];
static fgbg_t color_grid[EDITOR_SIZE];


static ivec2_t index_to_coords(editor_t *editor, int index)
{
    int curr_x = 0, curr_y = 0;
    int last_nl = editor->screen_index - 1;

    if (index < editor->screen_index)
        return (ivec2_t) { -1, -1 };

    for (int i = editor->screen_index; i < editor->text_size; ++i, ++curr_x) {
        if (i == editor->cursor_index)
            break;

        if (editor->text_buffer[i] == '\n' ||  i - last_nl == EDITOR_WIDTH) {
            ++curr_y;
            curr_x = -1;
            last_nl = i;
        }
    }

    return (ivec2_t) { curr_x, curr_y };
}

static int coords_to_index(editor_t *editor, ivec2_t coords)
{
    int curr_y = 0;
    int index = editor->screen_index;
    int last_nl = 0;

    for (; index < editor->text_size && curr_y != coords.y; ++index) {
        if (editor->text_buffer[index] == '\n'
         || index - last_nl == EDITOR_WIDTH)
        {
            ++curr_y;
            last_nl = index;
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

static int index_move_up_n(editor_t *editor, int index, int count)
{
    (void)count; // TODO:

    bool passed_nl = false;
    int i = index - 1;

    for (; i >= 0; --i) {
        if (editor->text_buffer[i] == '\n') {
            if (passed_nl)
                break;

            passed_nl = true;
        }
    }

    return i + 1;
}

static int index_move_down_n(editor_t *editor, int index, int count)
{
    (void)count; // TODO:

    bool crossed_nl = false;
    int i = index;

    for (; i < editor->text_size - 1; ++i) {
        if (editor->text_buffer[i] == '\n') {
            crossed_nl = true;
            break;
        }
    }

    if (!crossed_nl)
        return index;

    return i + 1;
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

    memmove(editor->text_buffer + pos + src_size,
            editor->text_buffer + pos + dst_size,
            editor->text_size - pos - dst_size);

    memcpy(editor->text_buffer + pos, src, src_size);

    editor->text_size = new_len;
}

void editor_reserve(editor_t *editor, int pos, int size)
{
    int new_len = editor->text_size + size;

    if (new_len > editor->text_capacity) {
        int new_capacity = editor->text_capacity ? editor->text_capacity * 2
                                                 : 4096;
        while (new_len > new_capacity) {
            new_capacity *= 2;
        }

        editor->text_buffer = realloc(editor->text_buffer, new_capacity);
        malloc_check(editor->text_buffer);

        editor->text_capacity = new_capacity;
    }

    memmove(editor->text_buffer + pos + size,
            editor->text_buffer + pos,
            editor->text_size - pos);

    editor->text_size = new_len;
}

void editor_init(editor_t *editor)
{
    *editor = (editor_t) {0};
}

void editor_render(editor_t *editor, int x_pos, int y_pos)
{
    while (editor->cursor_index < editor->screen_index) {
        editor->screen_index = index_move_up_n(editor, editor->screen_index, 1);
    }

    ivec2_t curr = index_to_coords(editor, editor->cursor_index);

    while (curr.y >= EDITOR_HEIGHT) {
        editor->screen_index = index_move_down_n(editor,
                                                 editor->screen_index, 1);

        curr = index_to_coords(editor, editor->cursor_index);
    }

    /* populate grids with data */
    for (int y = 0; y < EDITOR_HEIGHT; ++y) {
        for (int x = 0; x < EDITOR_WIDTH; ++x) {
            color_grid[x + y * EDITOR_WIDTH] = (fgbg_t) {
                .fg = 0xFFFFFFFF,
                .bg = 0xAA000000,
            };
        }
    }

    int selection_min = editor->selection_start;
    int selection_max = editor->cursor_index;
    if (selection_min > selection_max) {
        int temp = selection_min;
        selection_min = selection_max;
        selection_max = temp;
    }

    int text_pos = editor->screen_index;

    for (int y = 0; y < EDITOR_HEIGHT; ++y) {
        int x = 0;

        for (; x < EDITOR_WIDTH && text_pos < editor->text_size; ++x)
        {
            if (editor->selecting
             && text_pos >= selection_min && text_pos <= selection_max)
            {
                color_grid[x + y * EDITOR_WIDTH] = (fgbg_t) {
                    .fg = 0xFFFFFFFF,
                    .bg = 0xAAAAAAAA,
                };
            }

            char c = editor->text_buffer[text_pos++];

            if (c == '\n')
                break;

            text_grid[x + y * EDITOR_WIDTH] = c;
        }

        memset(text_grid + x + y * EDITOR_WIDTH, ' ', EDITOR_WIDTH - x);
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
                  x_pos, y_pos, 1 * EDITOR_SCALE, 2 * EDITOR_SCALE);
}

bool editor_handle_mouse_button(editor_t *editor, int x, int y,
                                bagE_MouseButton event, bool down)
{
    if (down) {
        if (!rect_contains(editor_area(editor, x, y), event.x, event.y))
            return false;

        ivec2_t coords = {
            (event.x - x) / (1 * EDITOR_SCALE),
            (event.y - y) / (2 * EDITOR_SCALE),
        };

        int index = coords_to_index(editor, coords);

        editor->cursor_index = index;

        editor->mouse_down = true;
        editor->selecting = true;
        editor->selection_start = index;
    }
    else {
        editor->mouse_down = false;

        if (editor->selection_start == editor->cursor_index) {
            editor->selecting = false;
        }
    }

    return true;
}

bool editor_handle_mouse_position(editor_t *editor, int x, int y,
                                  bagE_Mouse event)
{
    if (!editor->mouse_down)
        return false;

    ivec2_t coords = {
        (event.x - x) / (1 * EDITOR_SCALE),
        (event.y - y) / (2 * EDITOR_SCALE),
    };

    if      (coords.x < 0)             coords.x = 0;
    else if (coords.x >= EDITOR_WIDTH) coords.x = EDITOR_WIDTH - 1;

    if      (coords.y < 0)              coords.y = 0;
    else if (coords.y >= EDITOR_HEIGHT) coords.y = EDITOR_HEIGHT - 1;

    int index = coords_to_index(editor, coords);

    editor->cursor_index = index;

    return true;
}

bool editor_handle_key(editor_t *editor, bagE_Key event, bool down)
{
    switch (event.key) {
        case KEY_CONTROL_LEFT:
            editor->ctrl_down = down;
            break;

        case KEY_UP: {
            if (!down)
                break;

            ivec2_t p = index_to_coords(editor, editor->cursor_index);

            editor->cursor_index = index_move_up_n(editor,
                                                   editor->cursor_index, 1);

            for (int i = 0; i < p.x; ++i) {
                int index = editor->cursor_index;

                if (index == editor->text_size
                 || editor->text_buffer[index] == '\n')
                    break;

                editor->cursor_index++;
            }
        } break;

        case KEY_DOWN: {
            if (!down)
                break;

            ivec2_t p = index_to_coords(editor, editor->cursor_index);

            editor->cursor_index = index_move_down_n(editor,
                                                     editor->cursor_index, 1);

            for (int i = 0; i < p.x; ++i) {
                int index = editor->cursor_index;

                if (index == editor->text_size
                 || editor->text_buffer[index] == '\n')
                    break;

                editor->cursor_index++;
            }
        } break;

        case KEY_LEFT: {
            if (!down)
                break;

            if (editor->cursor_index > 0) {
                --editor->cursor_index;
            }
        } break;

        case KEY_RIGHT: {
            if (!down)
                break;

            if (editor->cursor_index < editor->text_size) {
                ++editor->cursor_index;
            }
        } break;

        case KEY_RETURN: {
            if (!down)
                break;

            editor_replace(editor, editor->cursor_index, 0, "\n", 1);
            editor->cursor_index++;
        } break;

        case KEY_DELETE: {
            if (!down)
                break;

            editor_replace(editor, editor->cursor_index, 1, "", 0);
        } break;

        case KEY_BACK: {
            if (!down)
                break;

            if (editor->cursor_index > 0) {
                editor_replace(editor, editor->cursor_index - 1, 1, "", 0);
                --editor->cursor_index;
            }
        } break;

        case KEY_C: {
            if (!down)
                break;

            if (!editor->ctrl_down)
                break;

            int sel_min = editor->selection_start;
            int sel_max = editor->cursor_index;
            if (sel_min > sel_max) {
                int temp = sel_min;
                sel_min = sel_max;
                sel_max = temp;
            }

            bagE_clipCopy(editor->text_buffer + sel_min,
                          sel_max - sel_min + (sel_max < editor->text_size));
        } break;

        case KEY_V: {
            if (!down)
                break;

            if (!editor->ctrl_down)
                break;

            int size = bagE_clipSize();

            if (size < 0)
                break;

            editor_reserve(editor, editor->cursor_index, size);

            bagE_clipPaste(editor->text_buffer + editor->cursor_index, size);

            editor->cursor_index += size;
        } break;

        case KEY_X: {
            if (!down)
                break;

            if (!editor->ctrl_down)
                break;

            int sel_min = editor->selection_start;
            int sel_max = editor->cursor_index;
            if (sel_min > sel_max) {
                int temp = sel_min;
                sel_min = sel_max;
                sel_max = temp;
            }

            int size = sel_max - sel_min + (sel_max < editor->text_size);

            bagE_clipCopy(editor->text_buffer + sel_min, size);

            editor_replace(editor, sel_min, size, "", 0);

            editor->cursor_index = sel_min;
            editor->selecting = false;
        } break;
    }

    return true;
}

bool editor_handle_utf8(editor_t *editor, bagE_TextUTF8 event)
{
    if (editor->ctrl_down)
        return false;

    unsigned char c = event.text[0];

    if (c < 32 || c > 126)
        return false;

    editor_replace(editor, editor->cursor_index, 0, (char *)&c, 1);
    editor->cursor_index++;

    return true;
}
