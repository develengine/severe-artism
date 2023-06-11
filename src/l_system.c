#include "l_system.h"

#include "generator.h"

#include <stdio.h>


unsigned l_system_add_type(l_system_t *sys,
                           l_basic_t     *types, unsigned type_count,
                           l_type_load_t *loads, unsigned load_count)
// unsigned l_system_add_type(l_system_t *sys, l_basic_t *types, int count)
{
    safe_reserve(sys->param_types, sys->param_type_count, sys->param_type_capacity, type_count);
    safe_reserve(sys->type_loads, sys->type_load_count, sys->type_load_capacity, load_count);

    unsigned id = sys->type_count;

    unsigned params_index = sys->param_type_count;

    for (unsigned i = 0; i < type_count; ++i) {
        sys->param_types[params_index + i] = types[i];
    }

    sys->param_type_count += type_count;

    unsigned load_index = sys->type_load_count;

    for (unsigned i = 0; i < load_count; ++i) {
        sys->type_loads[load_index + i] = loads[i];
    }

    sys->type_load_count += load_count;

    l_type_t type = {
        .params_index = params_index, .params_count = type_count,
        .load_index   = load_index,   .load_count   = load_count,
    };

    safe_push(sys->types, sys->type_count, sys->type_capacity, type);

    return id;
}


l_expr_t l_system_add_code(l_system_t *sys, l_instruction_t *instructions, int count)
{
    safe_reserve(sys->instructions, sys->instruction_count, sys->instruction_capacity, count);

    unsigned index = sys->instruction_count;

    for (int i = 0; i < count; ++i) {
        sys->instructions[index + i] = instructions[i];
    }

    sys->instruction_count += count;


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

    safe_reserve(sys->params, sys->param_count, sys->param_capacity, acc_param_count);

    safe_reserve(sys->results, sys->result_count, sys->result_capacity, right_size);

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

    sys->result_count += right_size;


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

    safe_reserve(sys->data_buffers[sys->id],
                 sys->data_lengths[sys->id],
                 sys->data_capacities[sys->id],
                 param_count);

    for (unsigned i = 0; i < param_count; ++i) {
        sys->data_buffers[sys->id][data_index + i] = params[i];
    }

    sys->data_lengths[sys->id] += param_count;


    l_symbol_t symbol = {
        .type = type,
        .data_index = data_index,
    };

    safe_push(sys->buffers[sys->id], sys->lengths[sys->id], sys->capacities[sys->id], symbol);
}


char *l_system_update(l_system_t *sys)
{
    unsigned next_id = 1 - sys->id;

    sys->data_lengths[next_id] = 0;
    sys->lengths[next_id] = 0;

    for (unsigned sym_id = 0; sym_id < sys->lengths[sys->id]; ++sym_id) {
        l_symbol_t symbol = sys->buffers[sys->id][sym_id];

        for (unsigned rule_id = 0; rule_id < sys->rule_count; ++rule_id) {
            l_rule_t rule = sys->rules[rule_id];

            if (rule.left.type != symbol.type)
                continue;

            l_eval_res_t res = l_evaluate(sys,
                                          rule.left.predicate,
                                          true,
                                          rule.left.type,
                                          symbol.data_index,
                                          true);

            if (res.error)
                return res.error;

            assert(res.val.type == l_basic_Bool);

            if (!res.val.data.boolean)
                continue;

            unsigned acc_param_count = 0;

            for (unsigned i = 0; i < rule.right_size; ++i) {
                l_type_t type = sys->types[sys->results[rule.right_index + i].type];
                acc_param_count += type.params_count;
            }

            safe_reserve(sys->data_buffers[next_id],
                         sys->data_lengths[next_id],
                         sys->data_capacities[next_id],
                         acc_param_count);

            safe_reserve(sys->buffers[next_id],
                         sys->lengths[next_id],
                         sys->capacities[next_id],
                         rule.right_size);

            unsigned symbol_index = sys->lengths[next_id];

            for (unsigned ri = 0; ri < rule.right_size; ++ri) {
                unsigned data_index = sys->data_lengths[next_id];

                l_result_t result = sys->results[rule.right_index + ri];

                l_type_t type = sys->types[result.type];
                l_basic_t *param_types = sys->param_types + type.params_index;

                for (unsigned pi = 0; pi < type.params_count; ++pi) {
                    l_eval_res_t ret = l_evaluate(sys,
                                                  sys->params[result.params_index + pi],
                                                  true,
                                                  result.type,
                                                  symbol.data_index,
                                                  true);

                    if (ret.error)
                        return ret.error;

                    assert(ret.val.type == param_types[pi]);

                    sys->data_buffers[next_id][data_index + pi] = ret.val;
                }

                sys->data_lengths[next_id] += type.params_count;

                sys->buffers[next_id][symbol_index] = (l_symbol_t) {
                    .type = result.type,
                    .data_index = data_index
                };

                ++symbol_index;
            }

            sys->lengths[next_id] += rule.right_size;
        }
    }

    sys->id = next_id;
    return NULL;
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
l_eval_res_t l_evaluate_instruction(l_instruction_t inst,
                                    l_value_t *params, unsigned param_count,
                                    l_value_t *data_top, unsigned data_size,
                                    bool compute)
{
    switch (inst.id) {
        case l_inst_Value: {
            return (l_eval_res_t) { inst.op };
        } break;

        case l_inst_Param: {
            unsigned param_index = inst.op.data.integer;

            assert(param_index < param_count);

            l_value_t val = params[param_index];

            return (l_eval_res_t) { val };
        } break;

        case l_inst_Add: {
            assert(data_size >= 2);

            l_value_t a = data_top[-1];
            l_value_t b = data_top[ 0];

            if (a.type == l_basic_Bool || b.type == l_basic_Bool)
                return (l_eval_res_t) { .error = "Addition of booleans is not allowed!" };

            if (a.type == l_basic_Mat4 || b.type == l_basic_Mat4)
                return (l_eval_res_t) { .error = "Addition of matrices is not allowed!" };

            bool has_float = a.type == l_basic_Float || b.type == l_basic_Float;

            l_value_t res = {
                .type = has_float ? l_basic_Float : l_basic_Int,
            };

            if (compute) {
                if (has_float) {
                    float fa = a.data.floating;
                    float fb = b.data.floating;

                    if (a.type == l_basic_Int) {
                        fa = (float)(a.data.integer);
                    }
                    else if (b.type == l_basic_Int) {
                        fb = (float)(b.data.integer);
                    }

                    res.data.floating = fa + fb;
                }
                else {
                    res.data.integer = a.data.integer + b.data.integer;
                }
            }

            return (l_eval_res_t) { res, 2 };
        } break;

        case l_inst_Sub: {
            assert(data_size >= 2);

            l_value_t a = data_top[-1];
            l_value_t b = data_top[ 0];

            if (a.type == l_basic_Bool || b.type == l_basic_Bool)
                return (l_eval_res_t) { .error = "Difference of booleans is not allowed!" };

            if (a.type == l_basic_Mat4 || b.type == l_basic_Mat4)
                return (l_eval_res_t) { .error = "Difference of matrices is not allowed!" };

            bool has_float = a.type == l_basic_Float || b.type == l_basic_Float;

            l_value_t res = {
                .type = has_float ? l_basic_Float : l_basic_Int,
            };

            if (compute) {
                if (has_float) {
                    float fa = a.data.floating;
                    float fb = b.data.floating;

                    if (a.type == l_basic_Int) {
                        fa = (float)(a.data.integer);
                    }
                    else if (b.type == l_basic_Int) {
                        fb = (float)(b.data.integer);
                    }

                    res.data.floating = fa - fb;
                }
                else {
                    res.data.integer = a.data.integer - b.data.integer;
                }
            }

            return (l_eval_res_t) { res, 2 };
        } break;

        case l_inst_Mul: {
            assert(data_size >= 2);

            l_value_t a = data_top[-1];
            l_value_t b = data_top[ 0];

            if (a.type == l_basic_Bool || b.type == l_basic_Bool)
                return (l_eval_res_t) { .error = "Product of booleans is not allowed!" };

            if (a.type == l_basic_Mat4 || b.type == l_basic_Mat4) {
                if (a.type != l_basic_Mat4 || b.type != l_basic_Mat4)
                    return (l_eval_res_t) { .error = "Both values must be matrices for product!" };

                l_value_t res = { .type = l_basic_Mat4 };

                if (compute) {
                    res.data.matrix = matrix_multiply(a.data.matrix, b.data.matrix);
                }

                return (l_eval_res_t) { res, 2 };
            }

            bool has_float = a.type == l_basic_Float || b.type == l_basic_Float;

            l_value_t res = {
                .type = has_float ? l_basic_Float : l_basic_Int,
            };

            if (compute) {
                if (has_float) {
                    float fa = a.data.floating;
                    float fb = b.data.floating;

                    if (a.type == l_basic_Int) {
                        fa = (float)(a.data.integer);
                    }
                    else if (b.type == l_basic_Int) {
                        fb = (float)(b.data.integer);
                    }

                    res.data.floating = fa * fb;
                }
                else {
                    res.data.integer = a.data.integer * b.data.integer;
                }
            }

            return (l_eval_res_t) { res, 2 };
        } break;

        case l_inst_Div: {
            assert(data_size >= 2);

            l_value_t a = data_top[-1];
            l_value_t b = data_top[ 0];

            if (a.type == l_basic_Bool || b.type == l_basic_Bool)
                return (l_eval_res_t) { .error = "Division of booleans is not allowed!" };

            if (a.type == l_basic_Mat4 || b.type == l_basic_Mat4)
                return (l_eval_res_t) { .error = "Division of matrices is not allowed!" };

            bool has_float = a.type == l_basic_Float || b.type == l_basic_Float;

            l_value_t res = {
                .type = has_float ? l_basic_Float : l_basic_Int,
            };

            if (compute) {
                if (has_float) {
                    float fa = a.data.floating;
                    float fb = b.data.floating;

                    if (a.type == l_basic_Int) {
                        fa = (float)(a.data.integer);
                    }
                    else if (b.type == l_basic_Int) {
                        fb = (float)(b.data.integer);
                    }

                    res.data.floating = fa / fb;
                }
                else {
                    if (b.data.integer == 0)
                        return (l_eval_res_t) { .error = "Division by zero!" };

                    res.data.integer = a.data.integer / b.data.integer;
                }
            }

            return (l_eval_res_t) { res, 2 };
        } break;

        case l_inst_Mod: {
            assert(data_size >= 2);

            l_value_t a = data_top[-1];
            l_value_t b = data_top[ 0];

            if (a.type == l_basic_Bool || b.type == l_basic_Bool)
                return (l_eval_res_t) { .error = "Modulo of booleans is not allowed!" };

            if (a.type == l_basic_Float || b.type == l_basic_Float)
                return (l_eval_res_t) { .error = "Modulo of floats is not allowed!" };

            if (a.type == l_basic_Mat4 || b.type == l_basic_Mat4)
                return (l_eval_res_t) { .error = "Modulo of matrices is not allowed!" };

            l_value_t res = { .type = l_basic_Int };

            if (compute) {
                res.data.integer = a.data.integer % b.data.integer;
            }

            return (l_eval_res_t) { res, 2 };
        } break;

        case l_inst_Neg: {
            assert(data_size >= 1);

            l_value_t a = data_top[0];

            if (a.type == l_basic_Bool)
                return (l_eval_res_t) { .error = "Minus before boolean is not allowed!" };

            if (a.type == l_basic_Mat4)
                return (l_eval_res_t) { .error = "Minus before matrix is not allowed!" };

            l_value_t res = { .type = a.type };

            if (compute) {
                if (a.type == l_basic_Float) {
                    res.data.floating = -a.data.floating;
                }
                else {
                    res.data.integer = -a.data.integer;
                }
            }

            return (l_eval_res_t) { res, 1 };
        } break;

        case l_inst_Less:   /* fallthrough */
        case l_inst_More:   /* fallthrough */
        case l_inst_LessEq: /* fallthrough */
        case l_inst_MoreEq: /* fallthrough */
        case l_inst_Equal:  /* fallthrough */
        case l_inst_NotEqual:
        {
            assert(data_size >= 2);

            l_value_t a = data_top[-1];
            l_value_t b = data_top[ 0];

            if (a.type == l_basic_Bool || b.type == l_basic_Bool)
                return (l_eval_res_t) { .error = "Comparison of booleans is not allowed!" };

            if (a.type == l_basic_Mat4 || b.type == l_basic_Mat4)
                return (l_eval_res_t) { .error = "Comparison of matrices is not allowed!" };

            bool has_float = a.type == l_basic_Float || b.type == l_basic_Float;

            l_value_t res = { .type = l_basic_Bool };

            if (compute) {
                if (has_float) {
                    float fa = a.data.floating;
                    float fb = b.data.floating;

                    if (a.type == l_basic_Int) {
                        fa = (float)(a.data.integer);
                    }
                    else if (b.type == l_basic_Int) {
                        fb = (float)(b.data.integer);
                    }

                    if      (inst.id == l_inst_Less)   res.data.boolean = fa <  fb;
                    else if (inst.id == l_inst_More)   res.data.boolean = fa >  fb;
                    else if (inst.id == l_inst_LessEq) res.data.boolean = fa <= fb;
                    else if (inst.id == l_inst_MoreEq) res.data.boolean = fa >= fb;
                    else if (inst.id == l_inst_Equal)  res.data.boolean = fa == fb;
                    else                               res.data.boolean = fa != fb;
                }
                else {
                    int ia = a.data.integer;
                    int ib = b.data.integer;

                    if      (inst.id == l_inst_Less)   res.data.boolean = ia <  ib;
                    else if (inst.id == l_inst_More)   res.data.boolean = ia >  ib;
                    else if (inst.id == l_inst_LessEq) res.data.boolean = ia <= ib;
                    else if (inst.id == l_inst_MoreEq) res.data.boolean = ia >= ib;
                    else if (inst.id == l_inst_Equal)  res.data.boolean = ia == ib;
                    else                               res.data.boolean = ia != ib;
                }
            }

            return (l_eval_res_t) { res, 2 };
        } break;

        case l_inst_And: /* fallthrough */
        case l_inst_Or:  /* fallthrough */
        case l_inst_Not:
        {
            assert(data_size >= 2);

            l_value_t a = data_top[-1];
            l_value_t b = data_top[ 0];

            l_value_t res = { .type = l_basic_Bool };

            if (a.type != l_basic_Bool || b.type != l_basic_Bool)
                return (l_eval_res_t) { .error = "Logical operators take only booleans!" };

            if (compute) {
                bool ba = a.data.boolean;
                bool bb = b.data.boolean;

                if      (inst.id == l_inst_And) res.data.boolean = ba <  bb;
                else if (inst.id == l_inst_Or)  res.data.boolean = ba >  bb;
                else                            res.data.boolean = ba != bb;
            }

            return (l_eval_res_t) { res, 2 };
        } break;

        case l_inst_CastInt:   /* fallthrough */
        case l_inst_CastFloat: /* fallthrough */
        case l_inst_CastBool:
        {
            assert(data_size >= 1);

            l_value_t a = data_top[0];

            if (a.type == l_basic_Mat4)
                return (l_eval_res_t) { .error = "Can't cast matrix to another type!" };

            l_value_t res = {0};

            if      (inst.id == l_inst_CastInt)   res.type = l_basic_Int;
            else if (inst.id == l_inst_CastFloat) res.type = l_basic_Float;
            else                                  res.type = l_basic_Bool;

            if (compute) {
                if (inst.id == l_inst_CastInt) {
                    if      (a.type == l_basic_Float) res.data.integer = (int)a.data.floating;
                    else if (a.type == l_basic_Bool)  res.data.integer = (int)a.data.boolean;
                    else                              res.data.integer = a.data.integer;
                }
                else if (inst.id == l_inst_CastFloat) {
                    if      (a.type == l_basic_Int)  res.data.floating = (float)a.data.integer;
                    else if (a.type == l_basic_Bool) res.data.floating = (float)a.data.boolean;
                    else                             res.data.floating = a.data.floating;
                }
                else {
                    if      (a.type == l_basic_Int)   res.data.boolean = (bool)a.data.integer;
                    else if (a.type == l_basic_Float) res.data.boolean = (bool)a.data.floating;
                    else                              res.data.boolean = a.data.boolean;
                }
            }

            return (l_eval_res_t) { res, 1 };
        } break;

        case l_inst_Rotation: /* fallthrough */
        case l_inst_Scale:    /* fallthrough */
        case l_inst_Position:
        {
            assert(data_size >= 3);

            l_value_t x = data_top[-2];
            l_value_t y = data_top[-1];
            l_value_t z = data_top[ 0];

            if (x.type != l_basic_Float || y.type != l_basic_Float || z.type != l_basic_Float)
                return (l_eval_res_t) { .error = "Matrix functions can take only float values!" };

            l_value_t res = { .type = l_basic_Mat4 };

            if (compute) {
                if (inst.id == l_inst_Position) {
                    res.data.matrix = matrix_translation(x.data.floating,
                                                         y.data.floating,
                                                         z.data.floating);
                }
                else if (inst.id == l_inst_Scale) {
                    res.data.matrix = matrix_scale(x.data.floating,
                                                   y.data.floating,
                                                   z.data.floating);
                }
                else {
                    res.data.matrix = matrix_multiply(matrix_rotation_z(z.data.floating),
                                          matrix_multiply(matrix_rotation_y(y.data.floating),
                                              matrix_rotation_x(x.data.floating)));
                }
            }

            return (l_eval_res_t) { res, 3 };
        } break;

        case l_inst_Noop: {
            assert(data_size >= 1);

            l_value_t a = data_top[0];

            return (l_eval_res_t) { a, 1 };
        } break;

        default: {
            return (l_eval_res_t) { .error = "Unimplemented instruction!" };
        }
    }
}


l_eval_res_t l_evaluate(l_system_t *sys,
                        l_expr_t expr,
                        bool has_params,
                        unsigned type_index,
                        unsigned data_index,
                        bool compute)
{
    l_value_t *params = sys->data_buffers[sys->id] + data_index;
    l_instruction_t *code = sys->instructions + expr.index;

    unsigned param_count = 0;

    if (has_params) {
        l_type_t type = sys->types[type_index];
        param_count = type.params_count;
    }


    for (unsigned i = 0; i < expr.count; ++i) {
        l_instruction_t inst = code[i];

        l_eval_res_t res = l_evaluate_instruction(inst, params, param_count,
                                                  sys->eval_stack + sys->eval_stack_size - 1,
                                                  sys->eval_stack_size,
                                                  compute);

        if (res.error)
            return res;

        sys->eval_stack_size -= res.eaten;

        safe_push(sys->eval_stack, sys->eval_stack_size, sys->eval_stack_capacity, res.val);
    }

    assert(sys->eval_stack_size == 1);

    l_value_t res = sys->eval_stack[sys->eval_stack_size - 1];
    sys->eval_stack_size = 0;

    return (l_eval_res_t) { res };
}


l_build_t l_system_build(l_system_t *sys)
{
    if (sys->texture_count == 0)
        return (l_build_t) { .error = "No textures loaded!" };

    rect_t *views = malloc(sys->texture_count * sizeof(rect_t));;

    texture_data_t atlas_data = create_texture_atlas(sys->textures, views, sys->texture_count);
    unsigned atlas_texture = create_texture_object(atlas_data);

    for (unsigned i = 0; i < sys->resource_count; ++i) {
        l_resource_t *res = sys->resources + i;
        model_map_textures_to_view(&(res->model),
                                   frect_make(views[res->texture_index],
                                              atlas_data.width, atlas_data.height));
    }


    model_builder_t builder = {0};

    for (unsigned i = 0; i < sys->lengths[sys->id]; ++i) {
        l_symbol_t sym = sys->buffers[sys->id][i];
        l_type_t type = sys->types[sym.type];

        for (unsigned lid = 0; lid < type.load_count; ++lid) {
            l_type_load_t load = sys->type_loads[type.load_index + lid];
            
            l_eval_res_t res = l_evaluate(sys, load.expr, true, sym.type, sym.data_index, true);

            if (res.error)
                return (l_build_t) { .error = res.error };

            assert(res.val.type == l_basic_Mat4);

            model_builder_merge(&builder,
                                sys->resources[load.resource_index].model,
                                res.val.data.matrix);
        }
    }

    if (builder.data.index_count == 0)
        return (l_build_t) { .error = "Empty object!" };

    model_object_t object = create_model_object(builder.data);

    free_model_data(builder.data);

    return (l_build_t) {
        .atlas = atlas_texture,
        .object = object,
    };
}
