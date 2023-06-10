#include "parser.h"

#include "utils.h"

#include <ctype.h>

static const char *types[] = {
    [token_type_Identifier] = "identifier",
    [token_type_Comment]    = "comment",
    [token_type_Keyword]    = "keyword",
    [token_type_Literal]    = "literal",
    [token_type_Separator]  = "separator",
    [token_type_Operator]   = "operator",
    [token_type_Error]      = "error",
    [token_type_Empty]      = "empty",
};
static_assert(length(types) == TOKEN_TYPE_COUNT, "array length missmatch");

const char *token_type_to_str(token_type_t type)
{
    return types[type];
}

static const char *keywords[] = {
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

static const char *operators[] = {
    [token_op_Plus]     = "+",
    [token_op_Minus]    = "-",
    [token_op_Times]    = "*",
    [token_op_Divide]   = "/",
    [token_op_Modulo]   = "%",

    [token_op_Less]     = "<",
    [token_op_More]     = ">",
    [token_op_LessEq]   = "<=",
    [token_op_MoreEq]   = ">=",
    [token_op_Equal]    = "==",
    [token_op_NotEqual] = "!=",

    [token_op_And]      = "&&",
    [token_op_Or]       = "||",
    [token_op_Not]      = "!",
};
static_assert(length(operators) == TOKEN_OP_COUNT, "array length missmatch");

// a + b * c * d - e
// a b c * d * + e -

//   0   3   2   3   0
// . a * b + c * d - e
//   a b * c d * + e -

// a + b + c + d
// a b + c + d +

static int precedences[] = {
    [token_op_Plus] = 4,
    [token_op_Minus] = 4,
    [token_op_Times] = 3,
    [token_op_Divide] = 3,
    [token_op_Modulo] = 3,

    [token_op_Less] = 6,
    [token_op_More] = 6,
    [token_op_LessEq] = 6,
    [token_op_MoreEq] = 6,
    [token_op_Equal] = 7,
    [token_op_NotEqual] = 7,

    [token_op_And] = 11,
    [token_op_Or] = 12,
    [token_op_Not] = 2,
};
static_assert(length(precedences) == TOKEN_OP_COUNT, "array length missmatch");

#define NO_PRECEDENCE 666

static const char true_literal[]  = "true";
static const char false_literal[] = "false";


static inline bool is_empty(tokenizer_t *toki)
{
    return toki->pos == toki->end;
}

static inline char peek(tokenizer_t *toki)
{
    return *(toki->pos);
}

static inline char bite(tokenizer_t *toki)
{
    char c = *(toki->pos);

    if (c == '\n') {
        toki->ln++;
        toki->ln_pos = toki->pos - toki->begin;
    }

    toki->pos++;

    return c;
}

static inline bool bite_n(tokenizer_t *toki, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (toki->pos >= toki->end)
            return false;

        if (*(toki->pos) == '\n') {
            toki->ln++;
            toki->ln_pos = toki->pos - toki->begin;
        }

        toki->pos++;
    }

    return true;
}

static inline void chew_space(tokenizer_t *toki)
{
    while (!is_empty(toki) && isspace(peek(toki))) {
        bite(toki);
    }
}

static inline bool follows(tokenizer_t *toki, const char *str)
{
    for (size_t i = 0; str[i]; ++i) {
        if (toki->pos + i >= toki->end || str[i] != toki->pos[i])
            return false;
    }

    return true;
}

static inline bool is_decimal(char c)
{
    return c >= '0' && c <= '9';
}


token_t tokenizer_next(tokenizer_t *toki, bool ignore_comments)
{
tokenizer_start:
    if (toki->has_stored) {
        toki->has_stored = false;
        return toki->stored;
    }

    chew_space(toki);

    if (is_empty(toki))
        return (token_t) { .type = token_type_Empty };

    char c = peek(toki);

    /* comments */
    if (c == '#') {
        char *begin = toki->pos;

        bite(toki);

        if (!is_empty(toki) && peek(toki) == '*') {
            bite(toki);

            while (!is_empty(toki)) {
                if (bite(toki) == '*' && !is_empty(toki) && bite(toki) == '#')
                    break;
            }
        }
        else {
            while (!is_empty(toki) && peek(toki) != '\n') {
                bite(toki);
            }
        }

        if (!ignore_comments) {
            return (token_t) {
                .type = token_type_Comment,
                .data = { .begin = begin, .end = toki->pos },
            };
        }

        goto tokenizer_start;
    }

    /* operators */
    {
        token_t token = {0};
        size_t found_len = 0;

        for (token_op_t op = 0; op < TOKEN_OP_COUNT; ++op) {
            if (!follows(toki, operators[op]))
                continue;

            size_t len = strlen(operators[op]);

            if (found_len >= len)
                continue;

            found_len = len;

            char *begin = toki->pos;

            token = (token_t) {
                .type = token_type_Operator,
                .data = { .begin = begin, .end = begin + len },
                .meta.op = op,
            };
        }

        if (found_len > 0) {
            bite_n(toki, found_len);

            return token;
        }
    }

    /* separators */
    for (int i = 0; i < length(separators); ++i) {
        if (c != separators[i])
            continue;

        return (token_t) {
            .type = token_type_Separator,
            .data = {
                .begin = toki->pos,
                .end   = toki->pos + 1
            },
            .meta.sep = bite(toki),
        };
    }

    /* string literals */
    if (c == '"') {
        char *begin = toki->pos;

        bite(toki);

        for (;;) {
            char c2 = 0;

            if (is_empty(toki) || (c2 = bite(toki)) == '\n') {
                return (token_t) {
                    .type = token_type_Error,
                    .data = c2 ? sv_l("Unexpected end of file in string literal!")
                               : sv_l("Unexpected end of line in string literal!"),
                    .meta.err = { .line = toki->ln, .pos = toki->pos - toki->begin },
                };
            }

            if (c2 == '\\') {
                char nc = 0;
                if (is_empty(toki) || (nc = bite(toki)) == '\n' || (nc != '\\' && nc != '"')) {
                    sv_t msg;

                    if (!nc) {
                        msg = sv_l("Unexpected end of file in string escape!");
                    }
                    else if (nc == '\n') {
                        msg = sv_l("Unexpected end of line in string escape!");
                    }
                    else {
                        msg = sv_l("Unknown string escape!");
                    }

                    return (token_t) {
                        .type = token_type_Error,
                        .data = msg,
                        .meta.err = { .line = toki->ln, .pos = toki->pos - toki->begin },
                    };
                }

                continue;
            }

            if (c2 == '"') {
                return (token_t) {
                    .type = token_type_Literal,
                    .data = { .begin = begin, .end = toki->pos },
                };
            }
        }
    }

    /* number literals */
    if (is_decimal(c)) {
        char *begin = toki->pos;

        bite(toki);

        bool has_period = false;

        while (!is_empty(toki)) {
            char c2 = peek(toki);

            if (c2 == '.') {
                if (has_period) {
                    return (token_t) {
                        .type = token_type_Error,
                        .data = sv_l("Multimple periods in number literal!"),
                        .meta.err = { .line = toki->ln, .pos = toki->pos - toki->begin },
                    };
                }

                has_period = true;
            }
            else if (!is_decimal(c2) && c2 != '_') {
                break;
            }

            bite(toki);
        }

        return (token_t) {
            .type = token_type_Literal,
            .data = { .begin = begin, .end = toki->pos },
            .meta.lit = has_period ? token_lit_Fraction : token_lit_Decimal,
        };
    }

    /* invalid symbol */
    if (c != '_' && !isalnum(c)) {
        bite(toki);

        return (token_t) {
            .type = token_type_Error,
            .data = sv_l("Invalid character in identifier!"),
            .meta.err = { .line = toki->ln, .pos = toki->pos - toki->begin },
        };
    }

    char *begin = toki->pos;

    while (!is_empty(toki) && ((c = peek(toki)) == '_' || isalnum(c))) {
        bite(toki);
    }

    sv_t data = { .begin = begin, .end = toki->pos };

    /* boolean literals */
    if (sv_is(data, true_literal)) {
        return (token_t) {
            .type = token_type_Literal,
            .data = data,
            .meta.lit = token_lit_Boolean,
        };
    }

    if (sv_is(data, false_literal)) {
        return (token_t) {
            .type = token_type_Literal,
            .data = data,
            .meta.lit = token_lit_Boolean,
        };
    }

    /* keywords */
    for (token_kw_t kw = 0; kw < TOKEN_KW_COUNT; ++kw) {
        if (!sv_is(data, keywords[kw]))
            continue;

        return (token_t) {
            .type = token_type_Keyword,
            .data = data,
            .meta.kw = kw,
        };
    }

    /* identifier */
    return (token_t) {
        .type = token_type_Identifier,
        .data = data,
    };
}


void tokenizer_store(tokenizer_t *toki, token_t token)
{
    assert(!toki->has_stored);

    toki->stored = token;
}


static inline parse_result_t token_to_result(token_t token)
{
    assert(token.type == token_type_Error);

    return (parse_result_t) {
        .success = false,
        .line = token.meta.err.line,
        .pos = token.meta.err.pos,
        .message = token.data,
    };
}

typedef struct
{
    parse_result_t res;
    int count, next_prec;
    bool finished;
} expr_rec_t;

static expr_rec_t expression_rec(tokenizer_t *toki, l_system_t *sys,
                                 sv_t *param_names, unsigned param_count,
                                 int precedence)
{
    for (;;) {
        token_t token = tokenizer_next(toki, true);

        if (token.type == token_type_Error)
            return (expr_rec_t) { token_to_result(token) };

        if (token.type == token_type_Separator) {
            if (token.meta.sep != '(') {
                tokenizer_store(toki, token);

                return (expr_rec_t) {
                    .res = { .success = true },
                    .count = 0,
                    .finished = true,
                };
            }

            expr_rec_t ret = expression_rec(toki, sys, param_names, param_count, NO_PRECEDENCE);

            assert(ret.finished);

            if (!ret.res.success)
                return ret;

            token = tokenizer_next(toki, true);

            if (token.type == token_type_Error)
                return (expr_rec_t) { token_to_result(token) };

            if (token.type != token_type_Separator || token.meta.sep != ')') {
                return (expr_rec_t) {{
                    .success = false,
                    // TODO: Give all tokens line numbers and positions.
                    .message = sv_l("Expected ')', closing parenthesis!"),
                }};
            }

            continue;
        }

        
    }
}

l_expr_t parse_expression(tokenizer_t *toki, l_system_t *sys, bool has_params, unsigned type)
{

}

