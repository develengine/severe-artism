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


void l_system_update(l_system_t *sys)
{
    unsigned next_id = !sys->id;

    sys->data_lengths[next_id] = 0;
    sys->lengths[next_id] = 0;

    for (unsigned sym_id = 0; sym_id < sys->lengths[sys->id]; ++sym_id) {
        l_symbol_t symbol = sys->buffers[sys->id][sym_id];

        for (unsigned rule_id = 0; rule_id < sys->rule_count; ++rule_id) {
            l_rule_t rule = sys->rules[rule_id];

            if (rule.left.type != symbol.type)
                continue;

            l_value_t res = l_evaluate(sys,
                                       rule.left.predicate,
                                       rule.left.type,
                                       symbol.data_index,
                                       true);

            assert(res.type == l_basic_Bool);

            if (!res.data.boolean)
                continue;

            unsigned acc_param_count = 0;

            for (unsigned i = 0; i < rule.right_size; ++i) {
                l_type_t type = sys->types[sys->results[rule.right_index + i].type];
                acc_param_count += type.params_count;
            }

            unsigned new_data_length = sys->data_lengths[next_id] + acc_param_count;

            if (new_data_length > sys->data_capacities[next_id]) {
                unsigned capacity = sys->data_capacities[next_id];

                if (capacity == 0) {
                    capacity = 4096 / sizeof(l_value_t);
                }

                while (new_data_length > capacity) {
                    capacity *= 2;
                }

                sys->data_buffers[next_id] = realloc(sys->data_buffers[next_id],
                                                     capacity * sizeof(l_value_t));

                sys->data_capacities[next_id] = capacity;
            }

            unsigned new_length = sys->lengths[next_id] + rule.right_size;

            if (new_length > sys->capacities[next_id]) {
                unsigned capacity = sys->capacities[next_id];

                if (capacity == 0) {
                    capacity = 4096 / sizeof(l_symbol_t);
                }

                while (new_length > capacity) {
                    capacity *= 2;
                }

                sys->buffers[next_id] = realloc(sys->buffers[next_id],
                                                capacity * sizeof(l_symbol_t));

                sys->capacities[next_id] = capacity;
            }

            unsigned symbol_index = sys->lengths[next_id];

            for (unsigned ri = 0; ri < rule.right_size; ++ri) {
                unsigned data_index = sys->data_lengths[next_id];

                l_result_t result = sys->results[rule.right_index + ri];

                l_type_t type = sys->types[result.type];
                l_basic_t *param_types = sys->param_types + type.params_index;

                for (unsigned pi = 0; pi < type.params_count; ++pi) {
                    l_value_t value = l_evaluate(sys,
                                                 sys->params[result.params_index + pi],
                                                 result.type,
                                                 symbol.data_index,
                                                 true);

                    assert(value.type == param_types[pi]);

                    sys->data_buffers[next_id][data_index + pi] = value;
                }

                sys->data_lengths[next_id] += type.params_count;

                sys->buffers[next_id][symbol_index] = (l_symbol_t) {
                    .type = result.type,
                    .data_index = data_index
                };

                ++symbol_index;
            }

            sys->lengths[next_id] = new_length;
        }
    }

    sys->id = next_id;
}


void l_system_print(l_system_t *sys)
{
    for (unsigned i = 0; i < sys->lengths[sys->id]; ++i) {
        l_symbol_t sym = sys->buffers[sys->id][i];

        l_type_t type = sys->types[sym.type];
        l_basic_t *param_types = sys->param_types + type.params_index;
        unsigned param_count = type.params_count;

        l_value_t *data = sys->data_buffers[sys->id] + sym.data_index;

        printf("%d (", sym.type);

        for (unsigned ti = 0; ti < param_count; ++ti) {
            l_basic_t p_type = param_types[ti];
            l_value_t value = data[ti];

            assert(value.type == p_type);

            switch (p_type) {
                case l_basic_Int: {
                    printf("%d: ", value.data.integer);
                } break;

                case l_basic_Float: {
                    printf("%f: ", value.data.floating);
                } break;

                case l_basic_Bool: {
                    printf("%s: ", value.data.boolean ? "true" : "false");
                } break;

                case l_basic_Mat4: {
                    printf("<mat4>: ");
                } break;

                default: unreachable();
            }

            printf("%s, ", l_basic_name(p_type));
        }

        printf(")\n");
    }

    printf("\n");
}


// NOTE: `compute` flag determines if we also compute the values or not.
//       If `compute` is 'false', only the `type` attribute is valid.
//       If it is 'true' the `data` is valid as well.
//
//       This is useful for example to avoid divide by zero on integer values
//       when doing type checking.
l_value_t l_evaluate(l_system_t *sys,
                     l_expr_t expr,
                     unsigned type_index,
                     unsigned data_index,
                     bool compute)
{
    l_value_t *params = sys->data_buffers[sys->id] + data_index;
    l_instruction_t *code = sys->instructions + expr.index;

    l_type_t type = sys->types[type_index];
    l_basic_t *param_types = sys->param_types + type.params_index;

    for (unsigned i = 0; i < expr.count; ++i) {
        l_instruction_t inst = code[i];

        switch (inst.id) {
            case l_inst_Value: {
                safe_push(sys->eval_stack, sys->eval_stack_size, sys->eval_stack_capacity, inst.op);
            } break;

            case l_inst_Param: {
                unsigned param_index = inst.op.data.integer;

                assert(param_index < type.params_count);

                l_value_t val = params[param_index];

                assert(val.type == param_types[param_index]);

                safe_push(sys->eval_stack, sys->eval_stack_size, sys->eval_stack_capacity, val);
            } break;

            case l_inst_Add: {
                assert(sys->eval_stack_size >= 2);

                l_value_t b = sys->eval_stack[--(sys->eval_stack_size)];
                l_value_t a = sys->eval_stack[--(sys->eval_stack_size)];

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
                        if (a.type == l_basic_Int) {
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

                l_value_t b = sys->eval_stack[--(sys->eval_stack_size)];
                l_value_t a = sys->eval_stack[--(sys->eval_stack_size)];

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
                        if (a.type == l_basic_Int) {
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

