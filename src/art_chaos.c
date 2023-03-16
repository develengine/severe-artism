#include "art_chaos.h"

void art_chaos_init(art_chaos_ctx_t *ctx, int width, int height)
{
    glCreateTextures(GL_TEXTURE_2D, 1, &ctx->texture);
    glTextureParameteri(ctx->texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(ctx->texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureStorage2D(ctx->texture, 1, GL_R32F, width, height);
}

