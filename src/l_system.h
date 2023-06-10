#ifndef L_SYSTEM
#define L_SYSTEM

#include "utils.h"
#include "linalg.h"


typedef enum
{
    l_basic_Int,
    l_basic_Float,
    l_basic_Bool,
    l_basic_Mat4,

    L_BASIC_COUNT
} l_basic_t;

static inline const char *l_basic_name(l_basic_t type)
{
    switch (type) {
        case l_basic_Int:   return "int";
        case l_basic_Float: return "float";
        case l_basic_Bool:  return "bool";
        case l_basic_Mat4:  return "mat";

        case L_BASIC_COUNT: unreachable();
    }

    unreachable();
}

typedef struct
{
    l_basic_t type;

    union
    {
        int integer;
        float floating;
        bool boolean;

        // TODO: Rework the whole literal approach
        //       because of this fat boy.
        //       For now we use VLIW architecture.
        matrix_t matrix;
    } data;
} l_value_t;

typedef enum
{
    l_inst_Value,
    l_inst_Param,

    l_inst_Add,
    l_inst_Sub,
    l_inst_Mul,
    l_inst_Div,
    l_inst_Mod,
    l_inst_Neg,

    l_inst_Less,
    l_inst_More,
    l_inst_LessEq,
    l_inst_MoreEq,
    l_inst_Equal,
    l_inst_NotEqual,

    l_inst_And,
    l_inst_Or,
    l_inst_Not,

    l_inst_CastInt,
    l_inst_CastFloat,

    l_inst_Rotation,
    l_inst_Scale,
    l_inst_Position,

    l_inst_Noop,

    L_INST_COUNT
} l_inst_id_t;

typedef struct
{
    l_inst_id_t id;
    l_value_t op;
} l_instruction_t;

static inline void fprint_instruction(l_instruction_t inst, FILE *file)
{
    switch (inst.id) {
        case l_inst_Value: {
            switch (inst.op.type) {
                case l_basic_Int:
                    fprintf(file, "value { %d: int }\n", inst.op.data.integer);
                    break;
                case l_basic_Float:
                    fprintf(file, "value { %f: float }\n", inst.op.data.floating);
                    break;
                case l_basic_Bool:
                    fprintf(file, "value { %s: bool }\n", inst.op.data.boolean ? "true" : "false");
                    break;
                case l_basic_Mat4:
                    fprintf(file, "value { ...: mat }\n");
                    break;
                default:
                    assert(0 && "unknown value");
            }
        } break;

        case l_inst_Param: {
            fprintf(file, "param { %d }\n", inst.op.data.integer);
        } break;

        case l_inst_Add: { fprintf(file, "add {}\n"); } break;
        case l_inst_Sub: { fprintf(file, "sub {}\n"); } break;
        case l_inst_Mul: { fprintf(file, "mul {}\n"); } break;
        case l_inst_Div: { fprintf(file, "div {}\n"); } break;
        case l_inst_Mod: { fprintf(file, "mod {}\n"); } break;
        case l_inst_Neg: { fprintf(file, "neg {}\n"); } break;
        case l_inst_Less: { fprintf(file, "less {}\n"); } break;
        case l_inst_More: { fprintf(file, "more {}\n"); } break;
        case l_inst_LessEq: { fprintf(file, "less eq {}\n"); } break;
        case l_inst_MoreEq: { fprintf(file, "more eq {}\n"); } break;
        case l_inst_Equal: { fprintf(file, "equal {}\n"); } break;
        case l_inst_NotEqual: { fprintf(file, "not equal {}\n"); } break;
        case l_inst_And: { fprintf(file, "and {}\n"); } break;
        case l_inst_Or: { fprintf(file, "or {}\n"); } break;
        case l_inst_Not: { fprintf(file, "not {}\n"); } break;
        case l_inst_CastInt: { fprintf(file, "cast int {}\n"); } break;
        case l_inst_CastFloat: { fprintf(file, "cast float {}\n"); } break;
        case l_inst_Rotation: { fprintf(file, "rotation {}\n"); } break;
        case l_inst_Scale: { fprintf(file, "scale {}\n"); } break;
        case l_inst_Position: { fprintf(file, "position {}\n"); } break;
        case l_inst_Noop: { fprintf(file, "noop {}\n"); } break;

        default:
            assert(0 && "unknown value");
    }
}

typedef struct
{
    unsigned params_index, params_count;
} l_type_t;

typedef struct
{
    unsigned type;
    unsigned data_index;
} l_symbol_t;

typedef struct
{
    unsigned index, count;
} l_expr_t;

typedef struct
{
    unsigned type;
    l_expr_t predicate;
} l_match_t;

typedef struct
{
    unsigned type;
    unsigned params_index;
} l_result_t;

typedef struct
{
    l_match_t left;

    unsigned right_index;
    unsigned right_size;
} l_rule_t;

typedef struct
{
    l_value_t *data_buffers[2];
    unsigned data_lengths[2];
    unsigned data_capacities[2];

    l_symbol_t *buffers[2];
    unsigned lengths[2];
    unsigned capacities[2];

    unsigned id;

    /* types */
    l_basic_t *param_types;
    unsigned param_type_count, param_type_capacity;

    l_type_t *types;
    unsigned type_count, type_capacity;

    /* code */
    l_instruction_t *instructions;
    unsigned instruction_count, instruction_capacity;

    /* rules */
    l_expr_t *params;
    unsigned param_count, param_capacity;

    l_result_t *results;
    unsigned result_count, result_capacity;

    // TODO: Optimize type matching.
    //       We can make an offset list on top for
    //       individual type indices.
    l_rule_t *rules;
    unsigned rule_count, rule_capacity;

    /* evaluation stack */
    l_value_t *eval_stack;
    unsigned eval_stack_size, eval_stack_capacity;
} l_system_t;

unsigned l_system_add_type(l_system_t *sys, l_basic_t *types, int count);
l_expr_t l_system_add_code(l_system_t *sys, l_instruction_t *instructions, int count);
void l_system_add_rule(l_system_t *sys,
                       l_match_t left,
                       unsigned *right_types,
                       l_expr_t *params,
                       int right_size);

void l_system_empty(l_system_t *sys);
void l_system_append(l_system_t *sys, unsigned type, l_value_t *params);

typedef struct
{
    l_value_t val;
    unsigned eaten;
    char *error;
} l_eval_res_t;

l_eval_res_t l_evaluate_instruction(l_instruction_t inst,
                                    l_value_t *params, unsigned param_count,
                                    l_value_t *data_top, unsigned data_size,
                                    bool compute);

l_value_t l_evaluate(l_system_t *sys,
                     l_expr_t expr,
                     unsigned type_index,
                     unsigned data_index,
                     bool compute);

void l_system_update(l_system_t *sys);
void l_system_print(l_system_t *sys);

#endif // L_SYSTEM
