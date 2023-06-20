#include "l_system.h"

#include "generator.h"

#include <stdio.h>


unsigned l_system_add_type(l_system_t *sys,
                           l_basic_t     *types, unsigned type_count,
                           l_type_load_t *loads, unsigned load_count)
{
    dck_stretchy_reserve(sys->param_types, type_count);
    dck_stretchy_reserve(sys->type_loads,  load_count);

    unsigned id = sys->types.count;

    unsigned params_index = sys->param_types.count;

    for (unsigned i = 0; i < type_count; ++i) {
        sys->param_types.data[params_index + i] = types[i];
    }

    sys->param_types.count += type_count;

    unsigned load_index = sys->type_loads.count;

    for (unsigned i = 0; i < load_count; ++i) {
        sys->type_loads.data[load_index + i] = loads[i];
    }

    sys->type_loads.count += load_count;

    l_type_t type = {
        .params_index = params_index, .params_count = type_count,
        .load_index   = load_index,   .load_count   = load_count,
    };

    dck_stretchy_push(sys->types, type);

    return id;
}


l_expr_t l_system_add_code(l_system_t *sys, l_instruction_t *instructions, int count)
{
    dck_stretchy_reserve(sys->instructions, count);

    unsigned index = sys->instructions.count;

    for (int i = 0; i < count; ++i) {
        sys->instructions.data[index + i] = instructions[i];
    }

    sys->instructions.count += count;


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
        acc_param_count += sys->types.data[type].params_count;
    }

    dck_stretchy_reserve(sys->params, acc_param_count);
    dck_stretchy_reserve(sys->results, right_size);

    unsigned right_index = sys->results.count;
    unsigned param_pos = 0;

    for (int ri = 0; ri < right_size; ++ri) {
        unsigned type = right_types[ri];
        unsigned params_count = sys->types.data[type].params_count;
        unsigned params_index = sys->params.count;

        for (unsigned pi = 0; pi < params_count; ++pi) {
            sys->params.data[params_index + pi] = params[param_pos + pi];
        }

        sys->params.count += params_count;


        sys->results.data[right_index + ri] = (l_result_t) {
            .type = type,
            .params_index = params_index,
        };

        param_pos += params_count;
    }

    sys->results.count += right_size;


    l_rule_t rule = {
        .left = left,
        .right_index = right_index,
        .right_size = right_size,
    };

    dck_stretchy_push(sys->rules, rule);
}


void l_system_append(l_system_t *sys, unsigned type, l_value_t *params)
{
    unsigned param_count = sys->types.data[type].params_count;
    unsigned data_index = sys->values[sys->id].count;

    dck_stretchy_reserve(sys->values[sys->id], param_count);

    for (unsigned i = 0; i < param_count; ++i) {
        sys->values[sys->id].data[data_index + i] = params[i];
    }

    sys->values[sys->id].count += param_count;


    l_symbol_t symbol = {
        .type = type,
        .data_index = data_index,
    };

    dck_stretchy_push(sys->symbols[sys->id], symbol);
}


char *l_system_update(l_system_t *sys)
{
    unsigned next_id = 1 - sys->id;

    sys->values [next_id].count = 0;
    sys->symbols[next_id].count = 0;

    for (unsigned sym_id = 0; sym_id < sys->symbols[sys->id].count; ++sym_id) {
        l_symbol_t symbol = sys->symbols[sys->id].data[sym_id];

        for (unsigned rule_id = 0; rule_id < sys->rules.count; ++rule_id) {
            l_rule_t rule = sys->rules.data[rule_id];

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
                l_type_t type = sys->types.data[sys->results.data[rule.right_index + i].type];
                acc_param_count += type.params_count;
            }

            dck_stretchy_reserve(sys->values [next_id], acc_param_count);
            dck_stretchy_reserve(sys->symbols[next_id], rule.right_size);

            unsigned symbol_index = sys->symbols[next_id].count;

            for (unsigned ri = 0; ri < rule.right_size; ++ri) {
                unsigned data_index = sys->values[next_id].count;

                l_result_t result = sys->results.data[rule.right_index + ri];

                l_type_t type = sys->types.data[result.type];
                l_basic_t *param_types = sys->param_types.data + type.params_index;

                for (unsigned pi = 0; pi < type.params_count; ++pi) {
                    l_eval_res_t ret = l_evaluate(sys,
                                                  sys->params.data[result.params_index + pi],
                                                  true,
                                                  result.type,
                                                  symbol.data_index,
                                                  true);

                    if (ret.error)
                        return ret.error;

                    assert(ret.val.type == param_types[pi]);

                    sys->values[next_id].data[data_index + pi] = ret.val;
                }

                sys->values[next_id].count += type.params_count;

                sys->symbols[next_id].data[symbol_index] = (l_symbol_t) {
                    .type = result.type,
                    .data_index = data_index
                };

                ++symbol_index;
            }

            sys->symbols[next_id].count += rule.right_size;
        }
    }

    sys->id = next_id;
    return NULL;
}


void l_system_print(l_system_t *sys)
{
    for (unsigned i = 0; i < sys->symbols[sys->id].count; ++i) {
        l_symbol_t sym = sys->symbols[sys->id].data[i];

        l_type_t type = sys->types.data[sym.type];
        l_basic_t *param_types = sys->param_types.data + type.params_index;
        unsigned param_count = type.params_count;

        l_value_t *data = sys->values[sys->id].data + sym.data_index;

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
        case l_inst_Stretch:  /* fallthrough */
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
                else if (inst.id == l_inst_Stretch) {
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

        case l_inst_Scale: {
            assert(data_size >= 1);

            l_value_t x = data_top[0];

            if (x.type != l_basic_Float)
                return (l_eval_res_t) { .error = "Matrix functions can take only float values!" };

            l_value_t res = { .type = l_basic_Mat4 };

            if (compute) {
                res.data.matrix = matrix_scale(x.data.floating, x.data.floating, x.data.floating);
            }

            return (l_eval_res_t) { res, 1 };
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
    l_value_t *params = sys->values[sys->id].data + data_index;
    l_instruction_t *code = sys->instructions.data + expr.index;

    unsigned param_count = 0;

    if (has_params) {
        l_type_t type = sys->types.data[type_index];
        param_count = type.params_count;
    }


    for (unsigned i = 0; i < expr.count; ++i) {
        l_instruction_t inst = code[i];

        l_eval_res_t res = l_evaluate_instruction(inst, params, param_count,
                                                  sys->eval_stack.data + sys->eval_stack.count - 1,
                                                  sys->eval_stack.count,
                                                  compute);
        if (res.error)
            return res;

        sys->eval_stack.count -= res.eaten;

        dck_stretchy_push(sys->eval_stack, res.val);
    }

    assert(sys->eval_stack.count == 1);

    l_value_t res = sys->eval_stack.data[sys->eval_stack.count - 1];
    sys->eval_stack.count = 0;

    return (l_eval_res_t) { res };
}


l_build_t l_system_build(l_system_t *sys, model_builder_t *builder)
{
    for (unsigned i = 0; i < sys->symbols[sys->id].count; ++i) {
        l_symbol_t sym = sys->symbols[sys->id].data[i];
        l_type_t type = sys->types.data[sym.type];

        for (unsigned lid = 0; lid < type.load_count; ++lid) {
            l_type_load_t load = sys->type_loads.data[type.load_index + lid];
            
            l_eval_res_t res = l_evaluate(sys, load.expr, true, sym.type, sym.data_index, true);

            if (res.error)
                return (l_build_t) { .error = res.error };

            assert(res.val.type == l_basic_Mat4);

            model_builder_merge(builder,
                                sys->resources.data[load.resource_index].model,
                                res.val.data.matrix);
        }
    }

    if (builder->data.index_count == 0)
        return (l_build_t) { .error = "Empty object!" };

    return (l_build_t) {0};
}

