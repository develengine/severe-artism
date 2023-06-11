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

    [token_kw_Rotation] = "rotation",
    [token_kw_Scale]    = "scale",
    [token_kw_Position] = "position",

    [token_kw_PI]       = "PI",
    [token_kw_PHI]      = "PHI",
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

static l_inst_id_t operator_inst_ids[] = {
    [token_op_Plus]     = l_inst_Add,
    [token_op_Minus]    = l_inst_Sub,
    [token_op_Times]    = l_inst_Mul,
    [token_op_Divide]   = l_inst_Div,
    [token_op_Modulo]   = l_inst_Mod,

    [token_op_Less]     = l_inst_Less,
    [token_op_More]     = l_inst_More,
    [token_op_LessEq]   = l_inst_LessEq,
    [token_op_MoreEq]   = l_inst_MoreEq,
    [token_op_Equal]    = l_inst_Equal,
    [token_op_NotEqual] = l_inst_NotEqual,

    [token_op_And]      = l_inst_And,
    [token_op_Or]       = l_inst_Or,
    [token_op_Not]      = l_inst_Not,
};
static_assert(length(operator_inst_ids) == TOKEN_OP_COUNT, "array length missmatch");

static int kw_arg_counts[] = {
    [token_kw_Res] = -1,
    [token_kw_Def] = -1,
    [token_kw_Rule] = -1,

    [token_kw_Int] = 1,
    [token_kw_Float] = 1,
    [token_kw_Bool] = 1,
    [token_kw_Mat] = -1,

    [token_kw_Rotation] = 3,
    [token_kw_Scale] = 3,
    [token_kw_Position] = 3,

    [token_kw_PI] = 0,
    [token_kw_PHI] = 0,
};
static_assert(length(kw_arg_counts) == TOKEN_KW_COUNT, "array length missmatch");

static l_instruction_t kw_instructions[] = {
    [token_kw_Res] = {0},
    [token_kw_Def] = {0},
    [token_kw_Rule] = {0},

    [token_kw_Int] = { .id = l_inst_CastInt },
    [token_kw_Float] = { .id = l_inst_CastFloat },
    [token_kw_Bool] = { .id = l_inst_CastBool },
    [token_kw_Mat] = {0},

    [token_kw_Rotation] = { .id = l_inst_Rotation },
    [token_kw_Scale] = { .id = l_inst_Scale },
    [token_kw_Position] = { .id = l_inst_Position },

    [token_kw_PI] = { .id = l_inst_Value,
                      .op = { .type = l_basic_Float, .data.floating = 3.1415927f } },
    [token_kw_PHI] = { .id = l_inst_Value,
                      .op = { .type = l_basic_Float, .data.floating = 1.6180339f } },
};
static_assert(length(kw_instructions) == TOKEN_KW_COUNT, "array length missmatch");

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
        return (token_t) { .type = token_type_Empty, .ln = toki->ln };

    char c = peek(toki);

    /* comments */
    if (c == '#') {
        char *begin = toki->pos;
        size_t ln = toki->ln;

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
                .ln = ln,
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
            size_t ln = toki->ln;

            token = (token_t) {
                .type = token_type_Operator,
                .data = { .begin = begin, .end = begin + len },
                .ln = ln,
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
            .ln = toki->ln,
            .meta.sep = bite(toki),
        };
    }

    /* string literals */
    if (c == '"') {
        char *begin = toki->pos;
        size_t ln = toki->ln;

        bite(toki);

        for (;;) {
            char c2 = 0;

            if (is_empty(toki) || (c2 = bite(toki)) == '\n') {
                return (token_t) {
                    .type = token_type_Error,
                    .data = c2 ? sv_l("Unexpected end of file in string literal!")
                               : sv_l("Unexpected end of line in string literal!"),
                    .ln = ln,
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
                        .ln = ln,
                    };
                }

                continue;
            }

            if (c2 == '"') {
                return (token_t) {
                    .type = token_type_Literal,
                    .data = { .begin = begin, .end = toki->pos },
                    .ln = ln,
                };
            }
        }
    }

    /* number literals */
    if (is_decimal(c)) {
        char *begin = toki->pos;
        size_t ln = toki->ln;

        bite(toki);

        bool has_period = false;

        while (!is_empty(toki)) {
            char c2 = peek(toki);

            if (c2 == '.') {
                if (has_period) {
                    return (token_t) {
                        .type = token_type_Error,
                        .data = sv_l("Multimple periods in number literal!"),
                        .ln = ln,
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
            .ln = ln,
            .meta.lit = has_period ? token_lit_Fraction : token_lit_Decimal,
        };
    }

    /* invalid symbol */
    if (c != '_' && !isalnum(c)) {
        bite(toki);

        return (token_t) {
            .type = token_type_Error,
            .data = sv_l("Invalid character in identifier!"),
            .ln = toki->ln,
        };
    }

    char *begin = toki->pos;
    size_t ln = toki->ln;

    while (!is_empty(toki) && ((c = peek(toki)) == '_' || isalnum(c))) {
        bite(toki);
    }

    sv_t data = { .begin = begin, .end = toki->pos };

    /* boolean literals */
    if (sv_is(data, true_literal)) {
        return (token_t) {
            .type = token_type_Literal,
            .data = data,
            .ln = ln,
            .meta.lit = token_lit_Boolean,
        };
    }

    if (sv_is(data, false_literal)) {
        return (token_t) {
            .type = token_type_Literal,
            .data = data,
            .ln = ln,
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
            .ln = ln,
            .meta.kw = kw,
        };
    }

    /* identifier */
    return (token_t) {
        .type = token_type_Identifier,
        .data = data,
        .ln = ln,
    };
}


void tokenizer_store(tokenizer_t *toki, token_t token)
{
    assert(!toki->has_stored);

    toki->has_stored = true;
    toki->stored = token;
}


static inline parse_result_t token_to_result(tokenizer_t *toki, token_t token)
{
    assert(token.type == token_type_Error);

    size_t col = 0;
    for (; token.data.begin - col >= toki->begin && *(token.data.begin - col) != '\n'; ++col)
        ;;

    return (parse_result_t) {
        .success = false,
        .line = token.ln,
        .col = col,
        .message = token.data,
    };
}

static inline int data_to_int(sv_t sv)
{
    char *p = sv.begin;

    int res = 0;

    for (; p < sv.end; ++p) {
        res *= 10;
        res += *p - '0';
    }

    return res;
}

static inline float data_to_float(sv_t sv)
{
    char *p = sv.begin;

    int full = 0;
    int div = 0;

    for (; p < sv.end; ++p) {
        if (*p == '.') {
            div = 1;
            continue;
        }

        full *= 10;
        div *= 10;
        full += *p - '0';
    }

    return (float)full / div;
}


typedef struct
{
    parse_result_t res;
    l_basic_t type;
} expr_rec_t;

static expr_rec_t expr_err(tokenizer_t *toki, token_t token, char *msg)
{
    size_t col = 0;
    for (; token.data.begin - col >= toki->begin && *(token.data.begin - col) != '\n'; ++col)
        ;;

    return (expr_rec_t) {
        .res = {
            .success = false,
            .line = token.ln, .col = col,
            .message = sv_l(msg),
        },
    };
}



static expr_rec_t expression_rec(tokenizer_t *toki, l_system_t *sys,
                                 sv_t *param_names, l_value_t *params, unsigned param_count,
                                 int precedence)
{
    // NOTE: Used for type checking.
    //       Bit of a hack.
    l_value_t temp_stack[4];
    unsigned temp_size = 0;

    int op_prec = 0;
    l_inst_id_t inst_id = 0;

    bool has_unary = false;

    token_t token = tokenizer_next(toki, true);

    switch (token.type) {
        case token_type_Error:
            return (expr_rec_t) { token_to_result(toki, token) };

        case token_type_Empty:
            return expr_err(toki, token, "Unexpected end of expression!");

        /* parenthesis */
        case token_type_Separator: {
            /* if we run into other separator we return */
            if (token.meta.sep != '(') {
                tokenizer_store(toki, token);

                return expr_err(toki, token, "Unexpected end of expression!");
            }

            expr_rec_t ret = expression_rec(toki, sys, param_names, params, param_count,
                                            NO_PRECEDENCE);

            if (!ret.res.success)
                return ret;

            token = tokenizer_next(toki, true);
            if (token.type == token_type_Error)
                return (expr_rec_t) { token_to_result(toki, token) };

            if (token.type != token_type_Separator || token.meta.sep != ')')
                return expr_err(toki, token, "Expected ')', closing parenthesis!");

            temp_stack[temp_size++] = (l_value_t) { .type = ret.type };
        } break;

        /* literals */
        case token_type_Literal: {
            if (token.meta.lit == token_lit_String)
                return expr_err(toki, token, "Unexpected string literal!");

            l_instruction_t instruction = { .id = l_inst_Value };

            if (token.meta.lit == token_lit_Decimal) {
                instruction.op = (l_value_t) {
                    .type = l_basic_Int,
                    .data.integer = data_to_int(token.data),
                };
            }
            else if (token.meta.lit == token_lit_Fraction) {
                instruction.op = (l_value_t) {
                    .type = l_basic_Float,
                    .data.floating = data_to_float(token.data),
                };
            }
            else if (token.meta.lit == token_lit_Boolean) {
                instruction.op = (l_value_t) {
                    .type = l_basic_Bool,
                    .data.boolean = token.data.begin[0] == true_literal[0],
                };
            }

            temp_stack[temp_size++].type = instruction.op.type;

            safe_push(sys->instructions,
                      sys->instruction_count,
                      sys->instruction_capacity,
                      instruction);
        } break;

        /* parameters */
        case token_type_Identifier: {
            bool found = false;

            for (unsigned i = 0; i < param_count; ++i) {
                if (!sv_eq(token.data, param_names[i]))
                    continue;

                l_instruction_t instruction = {
                    .id = l_inst_Param,
                    .op = {
                        .type = l_basic_Int,
                        .data.integer = (int)i,
                    },
                };

                temp_stack[temp_size++].type = params[i].type;

                safe_push(sys->instructions,
                          sys->instruction_count,
                          sys->instruction_capacity,
                          instruction);

                found = true;
                break;
            }

            if (!found)
                return expr_err(toki, token, "Unknown identifier!");
        } break;

        /* predefined constants and functions */
        case token_type_Keyword: {
            token_kw_t kw = token.meta.kw;
            int argc = kw_arg_counts[kw];

            if (argc == -1)
                return expr_err(toki, token, "Illegal keyword in context!");

            if (argc > 0) {
                token = tokenizer_next(toki, true);
                if (token.type == token_type_Error)
                    return (expr_rec_t) { token_to_result(toki, token) };

                if (token.type != token_type_Separator || token.meta.sep != '(') {
                    return expr_err(toki, token,
                                    "Expected '(', opening parenthesis for function call!");
                }

                for (int i = 0; i < argc; ++i) {
                    if (i != 0) {
                        token = tokenizer_next(toki, true);
                        if (token.type == token_type_Error)
                            return (expr_rec_t) { token_to_result(toki, token) };

                        if (token.type != token_type_Separator || token.meta.sep != ',') {
                            return expr_err(toki, token,
                                            "Expected ',', separating comma in argument list!");
                        }
                    }

                    expr_rec_t ret = expression_rec(toki, sys,
                                                    param_names, params, param_count,
                                                    NO_PRECEDENCE);
                    if (!ret.res.success)
                        return ret;

                    temp_stack[temp_size++].type = ret.type;
                }

                token = tokenizer_next(toki, true);
                if (token.type == token_type_Error)
                    return (expr_rec_t) { token_to_result(toki, token) };

                if (token.type != token_type_Separator || token.meta.sep != ')')
                    return expr_err(toki, token, "Expected ')', closing parenthesis!");
            }

            l_instruction_t inst = kw_instructions[kw];

            /* type check */
            l_eval_res_t eval_res = l_evaluate_instruction(inst, params, param_count,
                                                           temp_stack + temp_size - 1, temp_size,
                                                           false);
            if (eval_res.error)
                return expr_err(toki, token, eval_res.error);

            assert(temp_size == eval_res.eaten);
            temp_size -= eval_res.eaten;
            temp_stack[temp_size++].type = eval_res.val.type;

            safe_push(sys->instructions,
                      sys->instruction_count,
                      sys->instruction_capacity,
                      inst);
        } break;

        /* unary operators */
        case token_type_Operator: {
            if (token.meta.op == token_op_Minus) {
                has_unary = true;
                op_prec = 2;
                inst_id = l_inst_Neg;
                break;
            }

            if (token.meta.op == token_op_Not) {
                has_unary = true;
                op_prec = 2;
                inst_id = l_inst_Not;
                break;
            }

            return expr_err(toki, token, "Unexpected non-unary operator!");
        } break;

        case token_type_Comment: unreachable();
        case TOKEN_TYPE_COUNT:   unreachable();
    }

    /* operators */
    for (;;) {
        if (!has_unary) {
            token = tokenizer_next(toki, true);
            if (token.type == token_type_Error)
                return (expr_rec_t) { token_to_result(toki, token) };

            if (token.type == token_type_Empty)
                return (expr_rec_t) { .res = { .success = true } };

            if (token.type == token_type_Separator) {
                if (token.meta.sep == '(')
                    return expr_err(toki, token, "Unexpected '(', opening parenthesis!");

                tokenizer_store(toki, token);

                return (expr_rec_t) { .res = { .success = true } };
            }

            if (token.type != token_type_Operator) {
                return expr_err(toki, token, "Expected an operator!");
            }

            op_prec = precedences[token.meta.op];
            inst_id = operator_inst_ids[token.meta.op];
        }

                                 // NOTE: Unary operators associate to the right.
        if (op_prec < precedence || (has_unary && op_prec == precedence)) {
            expr_rec_t ret = expression_rec(toki, sys, param_names, params, param_count, op_prec);
            if (!ret.res.success)
                return ret;

            temp_stack[temp_size++].type = ret.type;
        }
        else {
            tokenizer_store(toki, token);

            return (expr_rec_t) { .res = { .success = true } };
        }

        has_unary = false;

        l_instruction_t instruction = { .id = inst_id };

        /* type check */
        l_eval_res_t eval_res = l_evaluate_instruction(instruction, params, param_count,
                                                       temp_stack + temp_size - 1, temp_size,
                                                       false);
        if (eval_res.error)
            return expr_err(toki, token, eval_res.error);

        assert(temp_size == eval_res.eaten);
        temp_size -= eval_res.eaten;
        temp_stack[temp_size++].type = eval_res.val.type;

        safe_push(sys->instructions,
                  sys->instruction_count,
                  sys->instruction_capacity,
                  instruction);
    }
}

parse_expr_res_t parse_expression(tokenizer_t *toki, l_system_t *sys,
                                  bool has_params, unsigned type)
{
    unsigned index = sys->instruction_count;

    expr_rec_t ret = expression_rec(toki, sys, NULL, NULL, 0, NO_PRECEDENCE);
    if (!ret.res.success)
        return (parse_expr_res_t) { ret.res };

    unsigned count = sys->instruction_count - index;

    return (parse_expr_res_t) {
        .res = { .success = true },
        .expr = { .index = index, .count = count },
    };
}

