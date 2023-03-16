#ifndef ART_TEXT_H
#define ART_TEXT_H

#include "linalg.h"

#include <stdbool.h>

typedef struct
{
    int width, height;
    unsigned char *matrix;

    ivec2_t *queue;
    int q_capacity, q_start, q_end;

    bool bfs_growth;
    unsigned char fallback;
} pipes_ctx_t;


typedef struct
{
    unsigned char *text_matrix;
    pipes_ctx_t pipes;
} art_text_ctx_t;


bool pipes_ctx_update(pipes_ctx_t *ctx);

void art_text_init(art_text_ctx_t *ctx);
void art_text_render(art_text_ctx_t *ctx, int window_width, int window_height);
void art_text_update(art_text_ctx_t *ctx);
void art_text_free(art_text_ctx_t *ctx);

#endif // ART_TEXT_H
