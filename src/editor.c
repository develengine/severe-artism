#include "editor.h"

#include "utils.h"
#include "linalg.h"
#include "parser.h"

#include "bag_keys.h"

// static char    text_grid[EDITOR_SIZE];
// static fgbg_t color_grid[EDITOR_SIZE];

static dck_stretchy_t (char,   unsigned)  text_grid = {0};
static dck_stretchy_t (fgbg_t, unsigned) color_grid = {0};


static inline unsigned token_type_to_color(token_type_t type)
{
    switch (type) {
        case token_type_Identifier: return 0xFFFFFFFF;
        case token_type_Comment:    return 0xFF66AA66;
        case token_type_Keyword:    return 0xFF66FFFF;
        case token_type_Literal:    return 0xFF66AAFF;
        case token_type_Separator:  return 0xFFFFAAAA;
        case token_type_Operator:   return 0xFFFFAAAA;
        case token_type_Error:      return 0xFF0000FF;
        case token_type_Empty:      return 0xFFFFFFFF;

        case TOKEN_TYPE_COUNT: unreachable();
    }

    unreachable();
}


static ivec2_t index_to_coords(editor_t *editor, int index)
{
    int curr_x = 0, curr_y = 0;
    int last_nl = editor->screen_index - 1;

    if (index < editor->screen_index)
        return (ivec2_t) { -1, -1 };

    for (int i = editor->screen_index; i < editor->text_size; ++i, ++curr_x) {
        if (i == editor->cursor_index)
            break;

        if (editor->text_buffer[i] == '\n' ||  i - last_nl == editor->col_count) {
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
         || index - last_nl == editor->col_count)
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
        //           of a new buffer in case the original can't be resized / remapped...
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

void editor_render(editor_t *editor, int x_pos, int y_pos, int col_count, int row_count)
{
    /* set size and resize grid buffers */
    editor->col_count = col_count;
    editor->row_count = row_count;

    int char_count = col_count * row_count;

    text_grid.count  = 0;
    dck_stretchy_reserve(text_grid, char_count);

    color_grid.count = 0;
    dck_stretchy_reserve(color_grid, char_count);

    /* if cursor is out of view move it in */
    while (editor->cursor_index < editor->screen_index) {
        editor->screen_index = index_move_up_n(editor, editor->screen_index, 1);
    }

    ivec2_t curr = index_to_coords(editor, editor->cursor_index);

    while (curr.y >= editor->row_count) {
        editor->screen_index = index_move_down_n(editor,
                                                 editor->screen_index, 1);

        curr = index_to_coords(editor, editor->cursor_index);
    }

    /* populate grids with data */
    for (int y = 0; y < editor->row_count; ++y) {
        for (int x = 0; x < editor->col_count; ++x) {
            color_grid.data[x + y * editor->col_count] = (fgbg_t) {
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

    tokenizer_t toki = {
        .begin = editor->text_buffer,
        .pos   = editor->text_buffer,
        .end   = editor->text_buffer + editor->text_size
    };

    token_t token = tokenizer_next(&toki, false);

    while (text_pos >= token.data.end - toki.begin && token.type != token_type_Empty) {
        token = tokenizer_next(&toki, false);
    }


    for (int y = 0; y < editor->row_count; ++y) {
        int x = 0;

        for (; x < editor->col_count && text_pos < editor->text_size; ++x)
        {
            while (text_pos >= token.data.end - toki.begin && token.type != token_type_Empty) {
                token = tokenizer_next(&toki, false);
            }

            if (text_pos >= token.data.begin - toki.begin
             && text_pos <  token.data.end   - toki.begin) {
                color_grid.data[x + y * editor->col_count].fg = token_type_to_color(token.type);
            }

            if (editor->selecting
             && text_pos >= selection_min && text_pos <= selection_max)
            {
                color_grid.data[x + y * editor->col_count].bg = 0xAAAAAAAA;
            }

            char c = editor->text_buffer[text_pos++];

            if (c == '\n')
                break;

            text_grid.data[x + y * editor->col_count] = c;
        }

        memset(text_grid.data + x + y * editor->col_count, ' ', editor->col_count - x);
    }

    if (curr.x != -1 && curr.y != -1) {
        color_grid.data[curr.x + curr.y * editor->col_count] = (fgbg_t) {
            .fg = 0xAA000000,
            .bg = 0xFFFFFFFF,
        };
    }

    /* render grids */
    gui_begin_grid();
    gui_draw_grid(text_grid.data, color_grid.data, editor->col_count, editor->row_count,
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
        if (!editor->selecting && !editor->mouse_down)
            return false;

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

    if      (coords.x < 0)                  coords.x = 0;
    else if (coords.x >= editor->col_count) coords.x = editor->col_count - 1;

    if      (coords.y < 0)                  coords.y = 0;
    else if (coords.y >= editor->row_count) coords.y = editor->row_count - 1;

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
