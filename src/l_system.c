#include "l_system.h"

#include <stdio.h>


unsigned l_system_add_type(l_system_t *sys, l_basic_t *types, int count)
{
    unsigned new_param_type_count = sys->param_type_count + count;

    if (new_param_type_count > sys->param_type_capacity) {
        unsigned capacity = sys->param_type_capacity;

        if (capacity == 0) {
            capacity = 4096 / sizeof(l_basic_t);
        }

        while (new_param_type_count > capacity) {
            capacity *= 2;
        }

        sys->param_types = realloc(sys->param_types, capacity * sizeof(l_basic_t));
        malloc_check(sys->param_types);

        sys->param_type_capacity = capacity;
    }


    unsigned id = sys->type_count;
    unsigned params_index = sys->param_type_count;

    for (int i = 0; i < count; ++i) {
        sys->param_types[params_index + i] = types[i];
    }

    sys->param_type_count = new_param_type_count;


    l_type_t type = { params_index, count };
    safe_push(sys->types, sys->type_count, sys->type_capacity, type);

    return id;
}


l_expr_t l_system_add_code(l_system_t *sys, l_instruction_t *instructions, int count)
{
    unsigned new_instruction_count = sys->instruction_count + count;

    if (new_instruction_count > sys->instruction_capacity) {
        unsigned capacity = sys->instruction_capacity;

        if (capacity == 0) {
            capacity = 4096 / sizeof(l_instruction_t);
        }

        while (new_instruction_count > capacity) {
            capacity *= 2;
        }

        sys->instructions = realloc(sys->instructions, capacity * sizeof(l_instruction_t));
        malloc_check(sys->instructions);

        sys->instruction_capacity = capacity;
    }


    unsigned index = sys->instruction_count;

    for (int i = 0; i < count; ++i) {
        sys->instructions[index + i] = instructions[i];
    }

    sys->instruction_count = new_instruction_count;


    return (l_expr_t) { index, count };
}


void l_system_add_rule(l_system_t *sys,
                       l_match_t left,
                       unsigned *right_types,
                       l_expr_t *params,
                       int right_size)
{
    unsigned acc_param_count = 0;

    for (int i = 0; i < right_size; ++i) {
        unsigned type = right_types[i];
        acc_param_count += sys->types[type].params_count;
    }


    unsigned new_param_count = sys->param_count + acc_param_count;

    if (new_param_count > sys->param_capacity) {
        unsigned capacity = sys->param_capacity;

        if (capacity == 0) {
            capacity = 4096 / sizeof(l_expr_t);
        }

        while (new_param_count > capacity) {
            capacity *= 2;
        }

        sys->params = realloc(sys->params, capacity * sizeof(l_expr_t));
        malloc_check(sys->params);

        sys->param_capacity = capacity;
    }


    unsigned new_result_count = sys->result_count + right_size;

    if (new_result_count > sys->result_capacity) {
        unsigned capacity = sys->result_capacity;

        if (capacity == 0) {
            capacity = 4096 / sizeof(l_result_t);
        }

        while (new_result_count > capacity) {
            capacity *= 2;
        }

        sys->results = realloc(sys->results, capacity * sizeof(l_result_t));
        malloc_check(sys->results);

        sys->result_capacity = capacity;
    }


    unsigned right_index = sys->result_count;
    unsigned param_pos = 0;

    for (int ri = 0; ri < right_size; ++ri) {
        unsigned type = right_types[ri];
        unsigned params_count = sys->types[type].params_count;
        unsigned params_index = sys->param_count;

        for (unsigned pi = 0; pi < params_count; ++pi) {
            sys->params[params_index + pi] = params[param_pos + pi];
        }

        sys->param_count += params_count;


        sys->results[right_index + ri] = (l_result_t) {
            .type = type,
            .params_index = params_index,
        };

        param_pos += params_count;
    }

    sys->result_count = new_result_count;


    l_rule_t rule = {
        .left = left,
        .right_index = right_index,
        .right_size = right_size,
    };

    safe_push(sys->rules, sys->rule_count, sys->rule_capacity, rule);
}


void l_system_empty(l_system_t *sys)
{
    sys->data_lengths[0] = 0;
    sys->data_lengths[1] = 0;
    sys->lengths[0] = 0;
    sys->lengths[1] = 0;
    sys->id = 0;
}


void l_system_append(l_system_t *sys, unsigned type, l_value_t *params)
{
    unsigned param_count = sys->types[type].params_count;
    unsigned data_index = sys->data_lengths[sys->id];


    unsigned new_data_length = data_index + param_count;

    if (new_data_length > sys->data_capacities[sys->id]) {
        unsigned capacity = sys->data_capacities[sys->id];

        if (capacity == 0) {
            capacity = 4096 / sizeof(l_value_t);
        }

        while (new_data_length > capacity) {
            capacity *= 2;
        }

        sys->data_buffers[sys->id] = realloc(sys->data_buffers[sys->id],
                                             capacity * sizeof(l_value_t));
        malloc_check(sys->data_buffers[sys->id]);

        sys->data_capacities[sys->id] = capacity;
    }


    for (unsigned i = 0; i < param_count; ++i) {
        sys->data_buffers[sys->id][data_index + i] = params[i];
    }

    sys->data_lengths[sys->id] = new_data_length;


    l_symbol_t symbol = {
        .type = type,
        .data_index = data_index,
    };

    safe_push(sys->buffers[sys->id], sys->lengths[sys->id], sys->capacities[sys->id], symbol);
}


// NOTE: `compute` flag determines if we also compute the values or not.
//       If `compute` is 'false', only the `type` attribute is valid.
//       If it is 'true' the `data` is valid as well.
//
//       This is useful for example to avoid divide by zero on integer values
//       when doing type checking.
l_value_t l_evaluate(l_system_t *sys, l_expr_t expr, unsigned data_index, bool compute)
{
    l_value_t *params = sys->data_buffers[sys->id] + data_index;
    l_instruction_t *code = sys->instructions + expr.index;

    for (unsigned i = 0; i < expr.count; ++i) {
        l_instruction_t inst = code[i];
        

        switch (inst.id) {
            case l_inst_Value: {
                safe_push(sys->eval_stack, sys->eval_stack_size, sys->eval_stack_capacity, inst.op);
            } break;

            case l_inst_Param: {
                l_value_t val = params[inst.op.data.integer];
                safe_push(sys->eval_stack, sys->eval_stack_size, sys->eval_stack_capacity, val);
            } break;

            case l_inst_Add: {
                assert(sys->eval_stack_size >= 2);

                l_value_t b = sys->eval_stack[sys->eval_stack_size-- - 1];
                l_value_t a = sys->eval_stack[sys->eval_stack_size-- - 1];

                assert(a.type != l_basic_Bool);
                assert(a.type != l_basic_Mat4);
                assert(b.type != l_basic_Bool);
                assert(b.type != l_basic_Mat4);

                bool has_float = a.type == l_basic_Float || b.type == l_basic_Float;

                l_value_t res = {
                    .type = has_float ? l_basic_Float : l_basic_Int,
                };

                if (compute) {
                    if (has_float) {
                        if      (a.type == l_basic_Int) {
                            res.data.floating = (float)(a.data.integer) + b.data.floating;
                        }
                        else if (b.type == l_basic_Int) {
                            res.data.floating = a.data.floating + (float)(b.data.integer);
                        }
                        else {
                            res.data.floating = a.data.floating + b.data.floating;
                        }
                    }
                    else {
                        res.data.integer = a.data.integer + b.data.integer;
                    }
                }

                safe_push(sys->eval_stack, sys->eval_stack_size, sys->eval_stack_capacity, res);
            } break;

            case l_inst_Sub: {
                assert(sys->eval_stack_size >= 2);

                l_value_t b = sys->eval_stack[sys->eval_stack_size-- - 1];
                l_value_t a = sys->eval_stack[sys->eval_stack_size-- - 1];

                assert(a.type != l_basic_Bool);
                assert(a.type != l_basic_Mat4);
                assert(b.type != l_basic_Bool);
                assert(b.type != l_basic_Mat4);

                bool has_float = a.type == l_basic_Float || b.type == l_basic_Float;

                l_value_t res = {
                    .type = has_float ? l_basic_Float : l_basic_Int,
                };

                if (compute) {
                    if (has_float) {
                        if      (a.type == l_basic_Int) {
                            res.data.floating = (float)(a.data.integer) - b.data.floating;
                        }
                        else if (b.type == l_basic_Int) {
                            res.data.floating = a.data.floating - (float)(b.data.integer);
                        }
                        else {
                            res.data.floating = a.data.floating - b.data.floating;
                        }
                    }
                    else {
                        res.data.integer = a.data.integer - b.data.integer;
                    }
                }

                safe_push(sys->eval_stack, sys->eval_stack_size, sys->eval_stack_capacity, res);
            } break;

            default: {
                assert(0 && "unknown instruction");
            }
        }
    }

    assert(sys->eval_stack_size >= 1);

    l_value_t res = sys->eval_stack[sys->eval_stack_size - 1];
    sys->eval_stack_size = 0;
    return res;
}


void l_system_update(l_system_t *sys)
{
    (void)sys;
}


void l_system_print(l_system_t *sys)
{
    (void)sys;
}

