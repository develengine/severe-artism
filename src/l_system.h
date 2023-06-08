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

    l_inst_CastInt,
    l_inst_CastFloat,

    L_INST_COUNT
} l_inst_id_t;

typedef struct
{
    l_inst_id_t id;
    l_value_t op;
} l_instruction_t;

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
    l_expr_t expr;
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

void l_system_update(l_system_t *sys);
void l_system_print(l_system_t *sys);

l_value_t l_evaluate(l_system_t *sys, l_expr_t expr, unsigned data_index, bool compute);

#endif // L_SYSTEM
