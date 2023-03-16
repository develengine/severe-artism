#ifndef ART_CHAOS_H
#define ART_CHAOS_H

typedef struct
{
    unsigned compute_program;
    unsigned present_program;

    unsigned texture;
} art_chaos_ctx_t;


void art_chaos_init(art_chaos_ctx_t *ctx);

#endif // ART_CHAOS_H
