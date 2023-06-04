#ifndef GENERATOR_H
#define GENERATOR_H

#include "linalg.h"
#include "res.h"

typedef struct
{
    model_data_t data;

    int vertex_capacity, index_capacity;
} model_builder_t;


void model_builder_push(model_builder_t *builder, model_data_t data);

void model_builder_merge(model_builder_t *builder,
                         model_data_t data,
                         matrix_t transform);

#endif // GENERATOR_H
