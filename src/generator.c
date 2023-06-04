#include "generator.h"

#include "utils.h"


static inline stretch(model_builder_t *builder,
                      int vertex_count, int index_count)
{
    if (builder->data.vertex_count + vertex_count > builder->vertex_capacity) {
        int capacity = builder->vertex_capacity;

        capacity = capacity ? capacity * 2
                            : (4096 / sizeof(vertex_t));

        while (new_count > capacity) {
            capacity *= 2;
        }

        builder->data.vertices = realloc(builder->data.vertices,
                                         capacity * sizeof(vertex_t));
        malloc_check(builder->data.vertices);

        builder->vertex_capacity = capacity;
    }

    if (builder->data.index_count + index_count > builder->index_capacity) {
        int capacity = builder->index_capacity;

        capacity = capacity ? capacity * 2
                            : (4096 / sizeof(unsigned));

        while (new_count > capacity) {
            capacity *= 2;
        }

        builder->data.indices = realloc(builder->data.indices,
                                        capacity * sizeof(unsigned));
        malloc_check(builder->data.indices);

        builder->index_capacity = capacity;
    }
}


void model_builder_push(model_builder_t *builder, model_data_t data)
{
    stretch(builder, data.vertex_count, data.index_count);

    memcpy(builder->data.vertices + builder->data.vertex_count,
           data.vertices,
           data.vertex_count * sizeof(vertex_t));

    builder->data.vertex_count += data.vertex_count;

    int index_count = builder->data.index_count;

    for (int i = 0; i < data.index_count; ++i) {
        builder->data.indices[index_count + i] = index_count + data.indices[i];
    }

    builder->data.index_count += data.index_count;
}

void model_builder_merge(model_builder_t *builder,
                         model_data_t data,
                         matrix_t transform)
{
    stretch(builder, data.vertex_count, data.index_count);

    int vertex_count = builder->data.vertex_count;

    for (int i = 0; i < data.vertex_count; ++i) {
        vector_t v = vector_transform(data.vertices[i], transform);
        builder->data.vertices[vertex_count + i] = v;
    }

    builder->data.vertex_count += data.vertex_count;

    int index_count = builder->data.index_count;

    for (int i = 0; i < data.index_count; ++i) {
        builder->data.indices[index_count + i] = index_count + data.indices[i];
    }

    builder->data.index_count += data.index_count;
}

