#include "art_text.h"

#include "utils.h"
#include "gui.h"

#include "ascii.c"

#define FONT_WIDTH 8
#define MAT_WIDTH (640 / FONT_WIDTH)
#define MAT_HEIGHT (480 / FONT_WIDTH / 2)
#define MAT_SCALER 2


/* NOTE: Make sure the matrix is ' ' (space) cleared!
 *       Supports only valid ascii+128. */
static void draw_text_matrix(const unsigned char *matrix, int cols, int rows,
                             int x, int y, int font_width, color_t color)
{
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            gui_draw_text_n((char *)(matrix + row * cols), cols,
                          x, y + row * font_width * 2,
                          font_width, font_width * 2, 0, color);
        }
    }
}


bool pipes_ctx_update(pipes_ctx_t *ctx)
{
    if (ctx->q_start == ctx->q_end)
        return false;

    ivec2_t pos = ctx->queue[ctx->q_start];
    queue_pop(ctx->queue, ctx->q_capacity, ctx->q_start, ctx->q_end);

    ivec2_t neighbours[4] = {
        {{ pos.x - 1, pos.y }},
        {{ pos.x + 1, pos.y }},
        {{ pos.x, pos.y - 1 }},
        {{ pos.x, pos.y + 1 }},
    };

    unsigned conns[4][4] = {
        //single line          double line
        //neighbour current    neighbour current
        { ASCII_RS, ASCII_LS,  ASCII_RD, ASCII_LD },
        { ASCII_LS, ASCII_RS,  ASCII_LD, ASCII_RD },
        { ASCII_BS, ASCII_TS,  ASCII_BD, ASCII_TD },
        { ASCII_TS, ASCII_BS,  ASCII_TD, ASCII_BD },
    };

    bool neighbours_passed[4] = {0};

    unsigned need_mask = 0;
    unsigned no_mask   = 0;

    for (int n = 0; n < 4; ++n) {
        ivec2_t p = neighbours[n];

        if (p.x >= 0 && p.x < ctx->width && p.y >= 0 && p.y < ctx->height) {
            unsigned char c = ctx->matrix[p.x + p.y * ctx->width];
            unsigned con = ascii_edge_connections[c];

            if (c >= ASCII_EDGES_BEGIN && c < ASCII_EDGES_END) {
                if      (con & conns[n][0]) need_mask |= conns[n][1];
                else if (con & conns[n][2]) need_mask |= conns[n][3];
                else no_mask |= conns[n][1] | conns[n][3];
            } else {
                neighbours_passed[n] = c != '@' && c != ctx->fallback;
            }
        }
    }

    unsigned char fits[ASCII_EDGES_END - ASCII_EDGES_BEGIN];
    int fit_count = 0;

    for (unsigned char c = ASCII_EDGES_BEGIN; c < ASCII_EDGES_END; ++c) {
        unsigned connects = ascii_edge_connections[c];

        if ((need_mask & connects) == need_mask && !(no_mask & connects))
            fits[fit_count++] = c;
    }

    unsigned char choice = fit_count ? fits[rand() % fit_count] : ctx->fallback;
    unsigned choice_mask = ascii_edge_connections[choice];

    for (int n = 0; n < 4; ++n) {
        if (!neighbours_passed[n])
            continue;

        unsigned nm = conns[n][1] | conns[n][3];

        if (ctx->bfs_growth || ((choice_mask & nm) && !(no_mask & nm))) {
            ivec2_t p = neighbours[n];
            ctx->matrix[p.x + p.y * ctx->width] = '@';
            queue_push(ctx->queue, ctx->q_capacity, ctx->q_start, ctx->q_end, p);
        }
    }

    ctx->matrix[pos.x + pos.y * ctx->width] = choice;

    return true;
}


void art_text_init(art_text_ctx_t *ctx)
{
    ctx->text_matrix = malloc(sizeof(unsigned char) * MAT_WIDTH * MAT_HEIGHT);
    malloc_check(ctx->text_matrix);

    memset(ctx->text_matrix, ' ', MAT_WIDTH * MAT_HEIGHT);

    pipes_ctx_t *pipes = &(ctx->pipes);

    *pipes = (pipes_ctx_t) {
        .width = MAT_WIDTH,
        .height = MAT_HEIGHT,
        .matrix = ctx->text_matrix,
        .bfs_growth = false,
        .fallback = 'O',
    };

    queue_init(pipes->queue, pipes->q_capacity, pipes->q_start, pipes->q_end, 32);

    ivec2_t mid = {{ MAT_WIDTH / 2, MAT_HEIGHT / 2 }};
    ctx->text_matrix[mid.x + mid.y * MAT_WIDTH] = AsciiXS;

    queue_push(pipes->queue, pipes->q_capacity, pipes->q_start, pipes->q_end,
               ((ivec2_t){{ mid.x - 1, mid.y }}));
    queue_push(pipes->queue, pipes->q_capacity, pipes->q_start, pipes->q_end,
               ((ivec2_t){{ mid.x, mid.y - 1 }}));
    queue_push(pipes->queue, pipes->q_capacity, pipes->q_start, pipes->q_end,
               ((ivec2_t){{ mid.x + 1, mid.y }}));
    queue_push(pipes->queue, pipes->q_capacity, pipes->q_start, pipes->q_end,
               ((ivec2_t){{ mid.x, mid.y + 1 }}));
}


void art_text_free(art_text_ctx_t *ctx)
{
    free(ctx->text_matrix);
    free(ctx->pipes.queue);

}


void art_text_update(art_text_ctx_t *ctx)
{
    pipes_ctx_update(&(ctx->pipes));
}


void art_text_render(art_text_ctx_t *ctx, int window_width, int window_height)
{
    color_t text_color = {{ 1.0f, 1.0f, 1.0f, 1.0f }};

    int x = (window_width  - 640 * MAT_SCALER) / 2;
    int y = (window_height - 480 * MAT_SCALER) / 2;

    gui_begin_rect();
    gui_draw_rect(x, y, 640 * MAT_SCALER, 480 * MAT_SCALER,
                  (color_t) {{ 0.0f, 0.0f, 0.0f, 1.0f }});

    gui_begin_text();
    draw_text_matrix(ctx->text_matrix, MAT_WIDTH, MAT_HEIGHT,
                     x, y, FONT_WIDTH * MAT_SCALER, text_color);
}

