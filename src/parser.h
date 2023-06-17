#ifndef PARSER_H
#define PARSER_H

#include "sv.h"
#include "l_system.h"


typedef enum
{
    token_type_Identifier,
    token_type_Comment,

    token_type_Keyword,
    token_type_Literal,
    token_type_Separator,
    token_type_Operator,

    token_type_Error,
    token_type_Empty,

    TOKEN_TYPE_COUNT
} token_type_t;

typedef enum
{
    token_kw_Tex,
    token_kw_Mod,
    token_kw_Res,
    token_kw_Def,
    token_kw_Rule,

    token_kw_Cylinder,
    token_kw_Sphere,
    token_kw_Object,

    token_kw_Int,
    token_kw_Float,
    token_kw_Bool,
    token_kw_Mat,

    token_kw_Rotation,
    token_kw_Scale,
    token_kw_Position,

    token_kw_PI,
    token_kw_PHI,

    TOKEN_KW_COUNT,
} token_kw_t;

typedef enum
{
    token_op_Plus,
    token_op_Minus,
    token_op_Times,
    token_op_Divide,
    token_op_Modulo,

    token_op_Less,
    token_op_More,
    token_op_LessEq,
    token_op_MoreEq,
    token_op_Equal,
    token_op_NotEqual,

    token_op_And,
    token_op_Or,
    token_op_Not,

    TOKEN_OP_COUNT
} token_op_t;

typedef enum
{
    token_lit_String,
    token_lit_Decimal,
    token_lit_Fraction,
    token_lit_Boolean,

    TOKEN_LIT_TYPE_COUNT
} token_lit_t;

typedef struct
{
    bool success;

    size_t line, col;
    sv_t message;
} parse_result_t;

typedef struct
{
    token_type_t type;
    sv_t data;
    size_t ln;

    union
    {
        char sep;
        token_op_t op;
        token_lit_t lit;
        token_kw_t kw;
    } meta;
} token_t;

typedef struct
{
    char *begin, *pos, *end;
    size_t ln, ln_pos;

    bool has_stored;
    token_t stored;
} tokenizer_t;

token_t tokenizer_next(tokenizer_t *toki, bool ignore_comments);

void tokenizer_store(tokenizer_t *toki, token_t token);

const char *token_type_to_str(token_type_t type);

typedef struct
{
    parse_result_t res;
    l_basic_t type;
    l_expr_t expr;
} parse_expr_res_t;

typedef struct
{
    parse_result_t res;

    dck_stretchy_t (sv_t, unsigned) tex_names;
    dck_stretchy_t (sv_t, unsigned) mod_names;
    dck_stretchy_t (sv_t, unsigned) res_names;
    dck_stretchy_t (sv_t, unsigned) def_names;
    dck_stretchy_t (sv_t, unsigned) param_names;
} parse_state_t;

static inline void parse_state_reset(parse_state_t *state)
{
    state->tex_names.count   = 0;
    state->mod_names.count   = 0;
    state->res_names.count   = 0;
    state->def_names.count   = 0;
    state->param_names.count = 0;
}

parse_expr_res_t parse_expression(tokenizer_t *toki, l_system_t *sys,
                                  sv_t *param_names, l_basic_t *param_types, unsigned param_count);

void parse(l_system_t *sys, parse_state_t *state, char *text, size_t text_size);

#endif // PARSER_H
