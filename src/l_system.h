#ifndef L_SYSTEM
#define L_SYSTEM

#include "utils.h"
#include "linalg.h"


typedef enum
{
    l_lit_Int,
    l_lit_Float,
    l_lit_Bool,
    l_lit_Mat4,

    l_lit_Param,

    L_LIT_COUNT
} l_term_type_t;

typedef struct
{
    l_term_type_t type;

    union
    {
        int integer;
        float floating;
        bool boolean;

        // TODO: Rework the whole literal approach
        //       because of this fat boy.
        //       For now we use VLIW architecture.
        matrix_t matrix;

        unsigned parameter;
    } data;
} l_terminal_t;

typedef struct
{
    l_inst_Term,

    l_inst_Add,
    l_inst_Sub,
    l_inst_Mul,
    l_inst_Div,
    l_inst_Mod,

    L_INST_COUNT
} l_inst_id_t;

typedef struct
{
    l_inst_id_t id;
    l_terminal_t operand;
} l_instruction_t;

typedef struct
{
    unsigned params_index, params_count;
} l_type_t;

typedef struct
{
    unsigned type;
    unsigned data_index, data_size;
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
    l_match left;

    unsigned right;
    unsigned right_size;
} l_rule_t;

typedef struct
{
    l_terminal_t *data_buffers[2];
    unsigned data_lengths[2];
    unsigned data_capacities[2];

    l_symbol_t *buffers[2];
    unsigned lengths[2];
    unsigned capacities[2];

    unsigned id;

    /* types */
    l_term_type_t *param_types;
    unsigned param_type_count, param_type_capacity;

    l_type_t *types;
    unsigned type_count, type_capacity;

    /* code */
    l_instructions_t *instructions;
    unsigned instruction_count, instruction_capacity;

    /* rules */
    l_expr_t *params;
    unsigned param_count, param_capacity;

    l_result_t *results;
    unsigned result_count, result_capacity;

    l_rule_t *rules;
    unsigned rule_count, rule_capacity;
} l_system_t;

unsigned l_system_add_type(l_system_t *sys, l_term_type_t *types, int count);
l_expr_t l_system_add_code(l_system_t *sys, l_instruction_t *instructions, int count);
void l_system_add_rule(l_system_t *sys,
                       l_match_t left,
                       unsigned *right_types,
                       l_expr_t *params, unsigned *right_param_counts,
                       int right_size);

void l_system_seed(l_system_t *sys, unsigned type, l_terminal_t *params);
void l_system_update(l_system_t *sys);
void l_system_print(l_system_t *sys);

#endif // L_SYSTEM
