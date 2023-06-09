#include "parser.h"

#include "utils.h"

#include <ctype.h>


static char *keywords[] = {
    [token_kw_Res]      = "res",
    [token_kw_Def]      = "def",
    [token_kw_Rule]     = "rule",

    [token_kw_Int]      = "int",
    [token_kw_Float]    = "float",
    [token_kw_Bool]     = "bool",
    [token_kw_Mat]      = "mat",

    [token_kw_PI]       = "PI",
    [token_kw_PHI]      = "PHI",

    [token_kw_Rotation] = "rotation",
    [token_kw_Scale]    = "scale",
    [token_kw_Position] = "position",
};
static_assert(length(keywords) == TOKEN_KW_COUNT, "array length missmatch");

static char separators[] = { '(', ')', '{', '}', ':', ',', '=' };

static char *operators[] = {
    [token_op_Plus]   = "+",
    [token_op_Minus]  = "-",
    [token_op_Times]  = "*",
    [token_op_Divide] = "/",
    [token_op_Modulo] = "%",

    [token_op_Less]   = "<",
    [token_op_More]   = ">",
    [token_op_LessEq] = "<=",
    [token_op_MoreEq] = ">=",
    [token_op_Equal]  = "==",

    [token_op_And]    = "&&",
    [token_op_Or]     = "||",
    [token_op_Not]    = "!",
};
static_assert(length(operators) == TOKEN_OP_COUNT, "array length missmatch");


static inline bool is_empty(tokenizer_t *toki)
{
    return toki->pos == toki->size;
}

static inline char peek(tokenizer_t *toki)
{
    return toki->text[toki->pos];
}

static inline char bite(tokenizer_t *toki)
{
    char c = toki->text[toki->pos];

    if (c == '\n') {
        toki->ln++;
        toki->ln_pos = toki->pos;
    }

    toki->pos++;

    return c;
}

static inline void chew_space(tokenizer_t *toki)
{
    while (!is_empty(toki) && isspace(peek(toki))) {
        bite(toki);
    }
}


token_t tokenizer_next(tokenizer_t *toki)
{
    (void)toki;
    return (token_t) {0};
}

