#ifndef L_SYSTEM
#define L_SYSTEM

#include "utils.h"
#include "linalg.h"
#include "res.h"
#include "core.h"


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
    l_inst_CastBool,

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

        case l_inst_Add: { fprintf(file, "add\n"); } break;
        case l_inst_Sub: { fprintf(file, "sub\n"); } break;
        case l_inst_Mul: { fprintf(file, "mul\n"); } break;
        case l_inst_Div: { fprintf(file, "div\n"); } break;
        case l_inst_Mod: { fprintf(file, "mod\n"); } break;
        case l_inst_Neg: { fprintf(file, "neg\n"); } break;
        case l_inst_Less: { fprintf(file, "less\n"); } break;
        case l_inst_More: { fprintf(file, "more\n"); } break;
        case l_inst_LessEq: { fprintf(file, "less eq\n"); } break;
        case l_inst_MoreEq: { fprintf(file, "more eq\n"); } break;
        case l_inst_Equal: { fprintf(file, "equal\n"); } break;
        case l_inst_NotEqual: { fprintf(file, "not equal\n"); } break;
        case l_inst_And: { fprintf(file, "and\n"); } break;
        case l_inst_Or: { fprintf(file, "or\n"); } break;
        case l_inst_Not: { fprintf(file, "not\n"); } break;
        case l_inst_CastInt: { fprintf(file, "cast int\n"); } break;
        case l_inst_CastFloat: { fprintf(file, "cast float\n"); } break;
        case l_inst_CastBool: { fprintf(file, "cast bool\n"); } break;
        case l_inst_Rotation: { fprintf(file, "rotation\n"); } break;
        case l_inst_Scale: { fprintf(file, "scale\n"); } break;
        case l_inst_Position: { fprintf(file, "position\n"); } break;
        case l_inst_Noop: { fprintf(file, "noop\n"); } break;

        default:
            assert(0 && "unknown value");
    }
}

typedef struct
{
    unsigned index, count;
} l_expr_t;

typedef struct
{
    unsigned resource_index;
    l_expr_t expr;
} l_type_load_t;

typedef struct
{
    unsigned params_index, params_count;
    unsigned load_index, load_count;
} l_type_t;

typedef struct
{
    unsigned type;
    unsigned data_index;
} l_symbol_t;

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
    model_data_t model;
    unsigned texture_index;
} l_resource_t;

typedef struct
{
    dck_stretchy_t (l_value_t,  unsigned) values [2];
    dck_stretchy_t (l_symbol_t, unsigned) symbols[2];

    unsigned id;

    /* code */
    dck_stretchy_t (l_instruction_t, unsigned) instructions;

    /* types */
    dck_stretchy_t (l_type_load_t, unsigned) type_loads;
    dck_stretchy_t (l_basic_t,     unsigned) param_types;
    dck_stretchy_t (l_type_t,      unsigned) types;

    /* rules */
    dck_stretchy_t (l_expr_t,   unsigned) params;
    dck_stretchy_t (l_result_t, unsigned) results;
    dck_stretchy_t (l_rule_t,   unsigned) rules;
    // TODO: Optimize type matching. We can make an offset list on top for individual type indices.

    /* evaluation stack */
    dck_stretchy_t (l_value_t, unsigned) eval_stack;

    /* resources */
    dck_stretchy_t (texture_data_t, unsigned) textures;
    dck_stretchy_t (model_data_t,   unsigned) models;
    dck_stretchy_t (l_resource_t,   unsigned) resources;

    /* atlas stuff */
    dck_stretchy_t (rect_t, unsigned) views;
    texture_data_t atlas;
    unsigned atlas_texture;
} l_system_t;

static inline void l_system_reset(l_system_t *sys)
{
    sys->instructions.count = 0;
    sys->type_loads.count   = 0;
    sys->param_types.count  = 0;
    sys->types.count        = 0;
    sys->params.count       = 0;
    sys->results.count      = 0;
    sys->rules.count        = 0;
    sys->eval_stack.count   = 0;
    sys->views.count        = 0;

    for (unsigned i = 0; i < sys->textures.count; ++i) {
        free_texture_data(sys->textures.data[i]);
    }

    for (unsigned i = 0; i < sys->models.count; ++i) {
        free_model_data(sys->models.data[i]);
    }

    for (unsigned i = 0; i < sys->resources.count; ++i) {
        free_model_data(sys->resources.data[i].model);
    }

    sys->models.count    = 0;
    sys->textures.count  = 0;
    sys->resources.count = 0;

    free_texture_data(sys->atlas);
    sys->atlas.data = NULL;

    glDeleteTextures(1, &sys->atlas_texture);
    sys->atlas_texture = 0;
}

static inline void l_system_empty(l_system_t *sys)
{
    sys->values[0].count = 0;
    sys->values[1].count = 0;

    sys->symbols[0].count = 0;
    sys->symbols[1].count = 0;

    sys->id = 0;
}



unsigned l_system_add_type(l_system_t *sys,
                           l_basic_t     *types, unsigned type_count,
                           l_type_load_t *loads, unsigned load_count);

l_expr_t l_system_add_code(l_system_t *sys, l_instruction_t *instructions, int count);

void l_system_add_rule(l_system_t *sys,
                       l_match_t left,
                       unsigned *right_types,
                       l_expr_t *params,
                       int right_size);

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

l_eval_res_t l_evaluate(l_system_t *sys,
                        l_expr_t expr,
                        bool has_params,
                        unsigned type_index,
                        unsigned data_index,
                        bool compute);

char *l_system_update(l_system_t *sys);
void l_system_print(l_system_t *sys);


typedef struct
{
    model_object_t object;
    char *error;
} l_build_t;

l_build_t l_system_build(l_system_t *sys);

#endif // L_SYSTEM
