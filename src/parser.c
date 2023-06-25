#include "parser.h"

#include "utils.h"
#include "obj_parser.h"
#include "generator.h"

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
    [token_kw_Tex]      = "tex",
    [token_kw_Mod]      = "mod",
    [token_kw_Res]      = "res",
    [token_kw_Def]      = "def",
    [token_kw_Rule]     = "rule",

    [token_kw_Cylinder] = "cylinder",
    [token_kw_Sphere]   = "sphere",
    [token_kw_Object]   = "object",

    [token_kw_Int]      = "int",
    [token_kw_Float]    = "float",
    [token_kw_Bool]     = "bool",
    [token_kw_Mat]      = "mat",

    [token_kw_Rotation] = "rotation",
    [token_kw_Stretch]  = "stretch",
    [token_kw_Position] = "position",
    [token_kw_Scale]    = "scale",

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
    [token_kw_Tex] = -1,
    [token_kw_Mod] = -1,
    [token_kw_Res] = -1,
    [token_kw_Def] = -1,
    [token_kw_Rule] = -1,

    [token_kw_Cylinder] = -1,
    [token_kw_Sphere] = -1,
    [token_kw_Object] = -1,

    [token_kw_Int] = 1,
    [token_kw_Float] = 1,
    [token_kw_Bool] = 1,
    [token_kw_Mat] = -1,

    [token_kw_Rotation] = 3,
    [token_kw_Stretch] = 3,
    [token_kw_Position] = 3,
    [token_kw_Scale] = 1,

    [token_kw_PI] = 0,
    [token_kw_PHI] = 0,
};
static_assert(length(kw_arg_counts) == TOKEN_KW_COUNT, "array length missmatch");

static l_instruction_t kw_instructions[] = {
    [token_kw_Tex] = {0},
    [token_kw_Mod] = {0},
    [token_kw_Res] = {0},
    [token_kw_Def] = {0},
    [token_kw_Rule] = {0},

    [token_kw_Cylinder] = {0},
    [token_kw_Sphere] = {0},
    [token_kw_Object] = {0},

    [token_kw_Int] = { .id = l_inst_CastInt },
    [token_kw_Float] = { .id = l_inst_CastFloat },
    [token_kw_Bool] = { .id = l_inst_CastBool },
    [token_kw_Mat] = {0},

    [token_kw_Rotation] = { .id = l_inst_Rotation },
    [token_kw_Stretch] = { .id = l_inst_Stretch },
    [token_kw_Position] = { .id = l_inst_Position },
    [token_kw_Scale] = { .id = l_inst_Scale },

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


static inline int extract_string_data(sv_t sv, char *dest, int capacity)
{
    int buff_index = 0;

    for (int i = 1; sv.begin[i] != '"'; ++i, ++buff_index) {
        char c = sv.begin[i];

        if (c == '\\') {
            ++i;
            c = sv.begin[i];
        }

        assert(buff_index < capacity - 1);

        dest[buff_index] = c;
    }

    assert(buff_index < capacity);

    dest[buff_index] = '\0';

    return buff_index;
}


static parse_result_t err(tokenizer_t *toki, token_t token, char *msg)
{
    size_t col = 0;
    for (; token.data.begin - col >= toki->begin && *(token.data.begin - col) != '\n'; ++col)
        ;;

    return (parse_result_t) {
        .success = false,
        .line = token.ln, .col = col,
        .message = sv_l(msg),
    };
}


typedef struct
{
    parse_result_t res;
    l_basic_t type;
} expr_rec_t;

static expr_rec_t expr_err(tokenizer_t *toki, token_t token, char *msg)
{
    return (expr_rec_t) { .res = err(toki, token, msg) };
}


#define next_checked_token(token, toki) \
do {                                    \
    token = tokenizer_next(toki, true); \
    if (token.type == token_type_Error) \
        goto bad_token;                 \
} while (0)


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

            next_checked_token(token, toki);

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

            dck_stretchy_push(sys->instructions, instruction);
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

                dck_stretchy_push(sys->instructions, instruction);

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
                next_checked_token(token, toki);

                if (token.type != token_type_Separator || token.meta.sep != '(') {
                    return expr_err(toki, token,
                                    "Expected '(', opening parenthesis for function call!");
                }

                for (int i = 0; i < argc; ++i) {
                    if (i != 0) {
                        next_checked_token(token, toki);

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

                next_checked_token(token, toki);

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

            dck_stretchy_push(sys->instructions, inst);
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
            next_checked_token(token, toki);

            if (token.type == token_type_Empty)
                return (expr_rec_t) { .res = { .success = true } };

            if (token.type == token_type_Separator) {
                if (token.meta.sep == '(')
                    return expr_err(toki, token, "Unexpected '(', opening parenthesis!");

                tokenizer_store(toki, token);

                assert(temp_size == 1);

                return (expr_rec_t) { .res = { .success = true },
                                      .type = temp_stack[0].type, };
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

        dck_stretchy_push(sys->instructions, instruction);
    }

bad_token:
    return (expr_rec_t) { token_to_result(toki, token) };
}

// FIXME: Nasty hack to use the interpreter for type checking.
#define MAX_TEMP_VALS 256
l_value_t temp_vals[MAX_TEMP_VALS];

parse_expr_res_t parse_expression(tokenizer_t *toki, l_system_t *sys,
                                  sv_t *param_names, l_basic_t *param_types, unsigned param_count)
{
    assert(param_count < MAX_TEMP_VALS);

    for (unsigned i = 0; i < param_count; ++i) {
        temp_vals[i].type = param_types[i];
    }

    unsigned index = sys->instructions.count;

    expr_rec_t ret = expression_rec(toki, sys, param_names, temp_vals, param_count,
                                    NO_PRECEDENCE);
    if (!ret.res.success)
        return (parse_expr_res_t) { ret.res };

    unsigned count = sys->instructions.count - index;

    return (parse_expr_res_t) {
        .res = { .success = true },
        .type = ret.type,
        .expr = { .index = index, .count = count },
    };
}

// TODO: Do this differently maybe? Idunno
#define MAX_PATH_SIZE 512
char path_buffer[MAX_PATH_SIZE];

static parse_result_t parse_texture(parse_state_t *state, tokenizer_t *toki, l_system_t *sys)
{
    token_t token;

    next_checked_token(token, toki);

    if (token.type != token_type_Identifier)
        return err(toki, token, "Expected identifier!");

    sv_t name = token.data;

    for (unsigned i = 0; i < state->tex_names.count; ++i) {
        if (sv_eq(name, state->tex_names.data[i]))
            return err(toki, token, "Texture name already exists!");
    }


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != '(')
        return err(toki, token, "Expected '(', opening parenthesis!");


    next_checked_token(token, toki);

    if (token.type != token_type_Literal || token.meta.lit != token_lit_String)
        return err(toki, token, "Expected string!");

    sv_t string_data = token.data;


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != ')')
        return err(toki, token, "Expected ')', closing parenthesis!");


    /* load texture data */
    extract_string_data(string_data, path_buffer, MAX_PATH_SIZE);
    texture_data_t tex = load_texture_data(path_buffer);

    if (!tex.data)
        return err(toki, token, "Can't load this texture path!");


    dck_stretchy_push(state->tex_names, name);
    dck_stretchy_push(sys->textures, tex);

    return (parse_result_t) { .success = true };

bad_token:
    return token_to_result(toki, token);
}


static parse_result_t parse_model(parse_state_t *state, tokenizer_t *toki, l_system_t *sys)
{
    token_t token;

    next_checked_token(token, toki);

    if (token.type != token_type_Identifier)
        return err(toki, token, "Expected identifier!");

    sv_t name = token.data;

    for (unsigned i = 0; i < state->mod_names.count; ++i) {
        if (sv_eq(name, state->mod_names.data[i]))
            return err(toki, token, "Model name already exists!");
    }


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != '(')
        return err(toki, token, "Expected '(', opening parenthesis!");


    next_checked_token(token, toki);

    if (token.type != token_type_Literal || token.meta.lit != token_lit_String)
        return err(toki, token, "Expected string!");

    sv_t string_data = token.data;


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != ')')
        return err(toki, token, "Expected ')', closing parenthesis!");


    /* load model data */
    extract_string_data(string_data, path_buffer, MAX_PATH_SIZE);
    model_data_t mod = load_obj_file(path_buffer);

    if (!mod.vertices)
        return err(toki, token, "Can't load this model path!");


    dck_stretchy_push(state->mod_names, name);
    dck_stretchy_push(sys->models, mod);

    return (parse_result_t) { .success = true };

bad_token:
    return token_to_result(toki, token);
}


static parse_result_t parse_resource(parse_state_t *state, tokenizer_t *toki, l_system_t *sys)
{
    token_t token;

    next_checked_token(token, toki);

    if (token.type != token_type_Identifier)
        return err(toki, token, "Expected identifier!");

    sv_t name = token.data;

    for (unsigned i = 0; i < state->res_names.count; ++i) {
        if (sv_eq(name, state->res_names.data[i]))
            return err(toki, token, "Resource name already exists!");
    }


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != '=')
        return err(toki, token, "Expected '=', equals sign!");


    next_checked_token(token, toki);

    if (token.type != token_type_Keyword)
        return err(toki, token, "Expected a resource function keyword!");

    token_kw_t kw = token.meta.kw;


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != '(')
        return err(toki, token, "Expected '(', opening parenthesis!");


    model_data_t first_arg;

    if (kw == token_kw_Cylinder || kw == token_kw_Sphere) {
        parse_expr_res_t ret = parse_expression(toki, sys, NULL, NULL, 0);
        if (!ret.res.success)
            return ret.res;

        if (ret.type != l_basic_Int)
            return err(toki, token, "Expression needs to be of type `int`!");

        l_eval_res_t res = l_evaluate(sys, ret.expr, false, 0, 0, true);

        assert(res.val.type == l_basic_Int);

        if (res.error)
            return err(toki, token, res.error);

        if (kw == token_kw_Cylinder) {
            if (res.val.data.integer < 2)
                return err(toki, token, "Cylinder needs at least 2 edges!");

            frect_t view = { 0.0f, 0.0f, 1.0f, 1.0f };
            first_arg = generate_cylinder(res.val.data.integer, view);
        }
        else {
            if (res.val.data.integer < 0)
                return err(toki, token, "Sphere can't have negative resolution!");

            frect_t view = { 0.0f, 0.0f, 1.0f, 1.0f };
            first_arg = generate_quad_sphere(res.val.data.integer, view);
        }
    }
    else if (kw == token_kw_Object) {
        next_checked_token(token, toki);

        if (token.type != token_type_Identifier)
            return err(toki, token, "Expected a model identifier!");

        sv_t mod_name = token.data;

        unsigned index = 0;
        for (; index < state->mod_names.count; ++index) {
            if (sv_eq(mod_name, state->mod_names.data[index]))
                break;
        }

        if (index == state->mod_names.count)
            return err(toki, token, "Unknown model name!");

        first_arg = copy_model_data(sys->models.data[index]);
    }
    else {
        return err(toki, token, "Unknown resource function!");
    }


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != ',')
        return err(toki, token, "Expected ',', argument separating comma!");

    
    next_checked_token(token, toki);

    if (token.type != token_type_Identifier)
        return err(toki, token, "Expected a texture identifier!");

    sv_t tex_name = token.data;

    unsigned second_arg = 0;
    for (; second_arg < state->tex_names.count; ++second_arg) {
        if (sv_eq(tex_name, state->tex_names.data[second_arg]))
            break;
    }

    if (second_arg == state->tex_names.count)
        return err(toki, token, "Unknown texture name!");


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != ')')
        return err(toki, token, "Expected ')', closing parenthesis!");


    l_resource_t resource = {
        .model = first_arg,
        .texture_index = second_arg,
    };

    dck_stretchy_push(state->res_names, name);
    dck_stretchy_push(sys->resources, resource);

    return (parse_result_t) { .success = true };

bad_token:
    return token_to_result(toki, token);
}


// TODO: Do we really need to change this?
#define MAX_TEMP 256
static sv_t      temp_names[MAX_TEMP];
static l_basic_t temp_types[MAX_TEMP];

static l_type_load_t temp_loads[MAX_TEMP];

static parse_result_t parse_define(parse_state_t *state, tokenizer_t *toki, l_system_t *sys)
{
    token_t token;

    next_checked_token(token, toki);

    if (token.type != token_type_Identifier)
        return err(toki, token, "Expected identifier!");

    sv_t name = token.data;


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != '(')
        return err(toki, token, "Expected '(', opening parenthesis!");


    unsigned param_count = 0;

    for (;;) {
        if (param_count > 0) {
            next_checked_token(token, toki);

            if (token.type != token_type_Separator
             || (token.meta.sep != ',' && token.meta.sep != ')'))
                return err(toki, token, "Unexpected token in argument list!");

            if (token.meta.sep == ')')
                break;
        }

        next_checked_token(token, toki);

        if (token.type == token_type_Separator) {
            if (token.meta.sep == ')')
                break;

            return err(toki, token, "Unexpected token in argument list!");
        }

        if (token.type != token_type_Identifier)
            return err(toki, token, "Expected a parameter identifier!");

        sv_t param_name = token.data;


        next_checked_token(token, toki);

        if (token.type != token_type_Separator || token.meta.sep != ':')
            return err(toki, token, "Expected ':', type specifier comma!");


        next_checked_token(token, toki);

        if (token.type != token_type_Keyword)
            return err(toki, token, "Expected a type keyword!");

        token_kw_t type_kws[] = { token_kw_Int, token_kw_Float, token_kw_Bool, token_kw_Mat };
        l_basic_t  b_types[]  = { l_basic_Int,  l_basic_Float,  l_basic_Bool,  l_basic_Mat4 };

        unsigned kw_index = 0;
        for (; kw_index < length(type_kws); ++kw_index) {
            if (token.meta.kw == type_kws[kw_index])
                break;
        }

        if (kw_index == length(type_kws))
            return err(toki, token, "Invalid keyword for type!");


        assert(param_count < MAX_TEMP);
        temp_names[param_count] = param_name;
        temp_types[param_count] = b_types[kw_index];
        ++param_count;
    }

    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != '{')
        return err(toki, token, "Expected '{', opening braces!");


    unsigned load_count = 0;

    for (;;) {
        next_checked_token(token, toki);

        if (token.type == token_type_Separator) {
            if (token.meta.sep != '}')
                return err(toki, token, "Unexpected separator in definition body!");

            break;
        }

        if (token.type != token_type_Identifier)
            return err(toki, token, "Expected a resource identifier!");

        sv_t res_name = token.data;

        unsigned res_index = 0;
        for (; res_index < state->res_names.count; ++res_index) {
            if (sv_eq(res_name, state->res_names.data[res_index]))
                break;
        }

        if (res_index == state->res_names.count)
            return err(toki, token, "Unknown resource name!");


        next_checked_token(token, toki);

        if (token.type != token_type_Separator || token.meta.sep != '(')
            return err(toki, token, "Expected '(', opening parenthesis!");


        parse_expr_res_t ret = parse_expression(toki, sys, temp_names, temp_types, param_count);
        if (!ret.res.success)
            return ret.res;

        if (ret.type != l_basic_Mat4)
            return err(toki, token, "Expression needs to be of type `mat`!");


        next_checked_token(token, toki);

        if (token.type != token_type_Separator || token.meta.sep != ')')
            return err(toki, token, "Expected ')', closing parenthesis!");


        assert(load_count < MAX_TEMP);
        temp_loads[load_count] = (l_type_load_t) {
            .resource_index = res_index,
            .expr = ret.expr,
        };
        ++load_count;
    }

    dck_stretchy_reserve(state->param_names, param_count);

    for (unsigned i = 0; i < param_count; ++i) {
        state->param_names.data[state->param_names.count + i] = temp_names[i];
    }

    state->param_names.count += param_count;

    dck_stretchy_push(state->def_names, name);

    l_system_add_type(sys, temp_types, param_count, temp_loads, load_count);


    return (parse_result_t) { .success = true };

bad_token:
    return token_to_result(toki, token);
}


#define MAX_PARAM_EXPRS 1024
static l_expr_t temp_exprs[MAX_PARAM_EXPRS];
static unsigned temp_body_types[MAX_PARAM_EXPRS];

static parse_result_t parse_rule(parse_state_t *state, tokenizer_t *toki, l_system_t *sys)
{
    token_t token;

    next_checked_token(token, toki);

    if (token.type != token_type_Identifier)
        return err(toki, token, "Expected type identifier!");

    sv_t type_name = token.data;

    unsigned type_index = 0;
    for (; type_index < state->def_names.count; ++type_index) {
        if (sv_eq(type_name, state->def_names.data[type_index]))
            break;
    }

    if (type_index == state->def_names.count)
        return err(toki, token, "Unknown type name!");


    l_type_t type = sys->types.data[type_index];

    parse_expr_res_t ret = parse_expression(toki, sys,
                                            state->param_names.data + type.params_index,
                                            sys->param_types.data + type.params_index,
                                            type.params_count);
    if (!ret.res.success)
        return ret.res;

    if (ret.type != l_basic_Bool)
        return err(toki, token, "Predicate expression needs to be of type `bool`!");

    l_expr_t predicate = ret.expr;


    next_checked_token(token, toki);

    if (token.type != token_type_Separator || token.meta.sep != '{')
        return err(toki, token, "Expected '{', opening braces!");


    unsigned type_count = 0;
    unsigned expr_count = 0;

    for (;;) {
        next_checked_token(token, toki);

        if (token.type == token_type_Separator) {
            if (token.meta.sep != '}')
                return err(toki, token, "Unexpected separator in rule body!");

            break;
        }

        if (token.type != token_type_Identifier)
            return err(toki, token, "Expected a type identifier!");

        sv_t def_name = token.data;

        unsigned def_index = 0;
        for (; def_index < state->def_names.count; ++def_index) {
            if (sv_eq(def_name, state->def_names.data[def_index]))
                break;
        }

        if (def_index == state->def_names.count)
            return err(toki, token, "Unknown type name!");


        next_checked_token(token, toki);

        if (token.type != token_type_Separator || token.meta.sep != '(')
            return err(toki, token, "Expected '(', opening parenthesis!");


        l_type_t def_type = sys->types.data[def_index];

        for (unsigned i = 0; i < def_type.params_count; ++i) {
            if (i > 0) {
                next_checked_token(token, toki);

                if (token.type != token_type_Separator || token.meta.sep != ',')
                    return err(toki, token, "Expected ',', argument separating comma!");
            }

            ret = parse_expression(toki, sys,
                                   state->param_names.data + type.params_index,
                                   sys->param_types.data + type.params_index,
                                   type.params_count);
            if (!ret.res.success)
                return ret.res;

            if (ret.type != sys->param_types.data[def_type.params_index + i])
                return err(toki, token, "Expression type doesn't match argument!");

            assert(expr_count < MAX_PARAM_EXPRS);
            temp_exprs[expr_count++] = ret.expr;
        }


        next_checked_token(token, toki);

        if (token.type != token_type_Separator || token.meta.sep != ')')
            return err(toki, token, "Expected ')', closing parenthesis!");


        assert(type_count < MAX_PARAM_EXPRS);
        temp_body_types[type_count++] = def_index;
    }


    l_match_t left = {
        .type = type_index,
        .predicate = predicate,
    };

    l_system_add_rule(sys, left, temp_body_types, temp_exprs, type_count);

    return (parse_result_t) { .success = true };

bad_token:
    return token_to_result(toki, token);
}


static parse_result_t parse_document(parse_state_t *state, tokenizer_t *toki, l_system_t *sys)
{
    token_t token;

    for (;;) {
        next_checked_token(token, toki);

        if (token.type == token_type_Empty)
            return (parse_result_t) { .success = true };

        if (token.type == token_type_Keyword) {
            if (token.meta.kw == token_kw_Tex) {
                parse_result_t res = parse_texture(state, toki, sys);
                if (!res.success)
                    return res;

                continue;
            }

            if (token.meta.kw == token_kw_Mod) {
                parse_result_t res = parse_model(state, toki, sys);
                if (!res.success)
                    return res;

                continue;
            }

            if (token.meta.kw == token_kw_Res) {
                parse_result_t res = parse_resource(state, toki, sys);
                if (!res.success)
                    return res;

                continue;
            }

            if (token.meta.kw == token_kw_Def) {
                parse_result_t res = parse_define(state, toki, sys);
                if (!res.success)
                    return res;

                continue;
            }

            if (token.meta.kw == token_kw_Rule) {
                parse_result_t res = parse_rule(state, toki, sys);
                if (!res.success)
                    return res;

                continue;
            }

            return err(toki, token, "Unexpected keyword!");
        }

        if (token.type == token_type_Identifier) {
            sv_t type_name = token.data;

            unsigned type_index = 0;
            for (; type_index < state->def_names.count; ++type_index) {
                if (sv_eq(type_name, state->def_names.data[type_index]))
                    break;
            }

            if (type_index == state->def_names.count)
                return err(toki, token, "Unknown type identifier");


            next_checked_token(token, toki);

            if (token.type != token_type_Separator || token.meta.sep != '(')
                return err(toki, token, "Expected '(', opening parenthesis!");


            l_type_t type = sys->types.data[type_index];

            for (unsigned i = 0; i < type.params_count; ++i) {
                if (i > 0) {
                    next_checked_token(token, toki);

                    if (token.type != token_type_Separator || token.meta.sep != ',')
                        return err(toki, token, "Expected ',', argument separating comma!");
                }

                parse_expr_res_t ret = parse_expression(toki, sys, NULL, NULL, 0);
                if (!ret.res.success)
                    return ret.res;

                if (ret.type != sys->param_types.data[type.params_index + i])
                    return err(toki, token, "Expression type doesn't match argument!");

                l_eval_res_t val = l_evaluate(sys, ret.expr, false, 0, 0, true);

                if (val.error)
                    return err(toki, token, val.error);

                assert(i < MAX_TEMP_VALS);
                temp_vals[i] = val.val;
            }

            next_checked_token(token, toki);

            if (token.type != token_type_Separator || token.meta.sep != ')')
                return err(toki, token, "Expected ')', closing parenthesis!");


            l_system_append(sys, type_index, temp_vals);

            continue;
        }

        return err(toki, token, "Expected a block keyword or type identifier!");
    }

bad_token:
    return token_to_result(toki, token);
}


void parse(l_system_t *sys, parse_state_t *state, char *text, size_t text_size)
{
    /* empty everything */
    l_system_empty(sys);
    l_system_reset(sys);
    parse_state_reset(state);

    /* parse */
    tokenizer_t toki = {
        .begin = text,
        .pos = text,
        .end = text + text_size,
    };

    state->res = parse_document(state, &toki, sys);

    if (!state->res.success)
        return;

    /* create texture atlas */
    if (sys->textures.count == 0) {
        state->res.success = false;
        state->res.message = sv_l("No textures provided!");
        return;
    }

    dck_stretchy_reserve(sys->views, sys->textures.count);

    sys->atlas = create_texture_atlas(sys->textures.data, sys->views.data, sys->textures.count);
    sys->atlas_texture = create_texture_object(sys->atlas);

    for (unsigned i = 0; i < sys->resources.count; ++i) {
        l_resource_t *res = sys->resources.data + i;
        model_map_textures_to_view(
            &res->model,
            frect_make(sys->views.data[res->texture_index], sys->atlas.width, sys->atlas.height)
        );
    }
}
