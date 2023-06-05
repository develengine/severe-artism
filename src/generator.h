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

model_data_t generate_cylinder(int n, frect_t view);
model_data_t generate_quad_sphere(int n, frect_t view);

texture_data_t create_texture_atlas(texture_data_t *textures, rect_t *views, int count);

void model_map_textures_to_view(model_data_t *data, frect_t view);

#endif // GENERATOR_H
