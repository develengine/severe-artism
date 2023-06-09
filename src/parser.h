#ifndef PARSER_H
#define PARSER_H

#include "sv.h"


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
    token_kw_Res,
    token_kw_Def,
    token_kw_Rule,

    token_kw_Int,
    token_kw_Float,
    token_kw_Bool,
    token_kw_Mat,

    token_kw_PI,
    token_kw_PHI,

    token_kw_Rotation,
    token_kw_Scale,
    token_kw_Position,

    TOKEN_KW_COUNT,
} token_kw_t;

typedef enum
{
    token_lit_Decimal,
    token_lit_Fraction,
    token_lit_Boolean,

    TOKEN_LIT_COUNT
} token_lit_t;

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

    token_op_And,
    token_op_Or,
    token_op_Not,

    TOKEN_OP_COUNT
} token_op_t;

typedef struct
{
    token_type_t type;
    unsigned meta;
    sv_t data;
} token_t;

typedef struct
{
    char *text;
    size_t pos, size;
    size_t ln, ln_pos;
} tokenizer_t;

token_t tokenizer_next(tokenizer_t *toki);

#endif // PARSER_H
