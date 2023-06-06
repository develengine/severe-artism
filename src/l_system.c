#include "l_system.h"

#include <stdio.h>


unsigned l_system_add_type(l_system_t *sys, l_term_type_t *types, int count)
{
    unsigned id = sys->type_count;

    unsigned params_id = sys->param_type_count;

    // TODO: Do at once.
    for (int i = 0; i < count; ++i) {
        assert(types[i] != l_lit_Param);

        safe_push(sys->param_types, sys->param_type_count, sys->param_type_capacity, types[i]);
    }

    l_type_t type = { params_id, count };
    safe_push(sys->types, sys->type_count, sys->type_capacity, type);

    return id;
}


l_expr_t l_system_add_code(l_system_t *sys, l_instruction_t *instructions, int count)
{
    unsigned index = sys->instruction_count;

    // TODO: Do at once.
    for (int i = 0; i < count; ++i) {
        l_instruction_t inst = instructions[i];
        safe_push(sys->instructions, sys->instruction_count, sys->instruction_capacity, inst);
    }

    return (l_expr_t) { index, count };
}


void l_system_add_rule(l_system_t *sys,
                       l_match_t left,
                       unsigned *right_types,
                       l_expr_t *params, unsigned *right_param_counts,
                       int right_size)
{
    unsigned results_id = sys->result_count;
    unsigned param_pos = 0;

    for (int ri = 0; ri < right_size; ++ri) {
        l_result_t result = right[ri];

        unsigned params_id = sys->param_count;

        
    }
}


void l_system_update(l_system_t *sys)
{
}


void l_system_print(l_system_t *sys)
{
}


