#ifndef RES_H
#define RES_H

#include "linalg.h"
#include "utils.h"

#include <stdlib.h>
#include <stdint.h>


typedef struct
{
    float positions[3];
    float textures[2];
    float normals[3];
} vertex_t;


typedef struct
{
    int vertex_count;
    int index_count;
    vertex_t *vertices;
    unsigned *indices;
} model_data_t;

static inline void free_model_data(model_data_t model_data)
{
    free(model_data.vertices);
    free(model_data.indices);
}

static model_data_t copy_model_data(model_data_t model_data)
{
    vertex_t *new_vertices = malloc(model_data.vertex_count * sizeof(vertex_t));
    malloc_check(new_vertices);

    unsigned *new_indices = malloc(model_data.index_count * sizeof(unsigned));
    malloc_check(new_indices);

    memcpy(new_vertices, model_data.vertices, model_data.vertex_count * sizeof(vertex_t));
    memcpy(new_indices,  model_data.indices,  model_data.index_count  * sizeof(unsigned));

    return (model_data_t) {
        .vertex_count = model_data.vertex_count,
        .index_count  = model_data.index_count,
        .vertices = new_vertices,
        .indices  = new_indices,
    };
}


typedef struct
{
    unsigned ids[4];
    float weights[4];
} vertex_weight_t;


typedef struct
{
    float position[4];
    quaternion_t rotation;
} joint_transform_t;


typedef struct
{
    int bone_count;

    matrix_t *ibms;

    unsigned *frame_counts;
    unsigned *frame_offsets;
    float *time_stamps;
    joint_transform_t *transforms;

    unsigned *child_counts;
    unsigned *child_offsets;
    unsigned *hierarchy;
} armature_t;

static inline void free_armature(armature_t armature)
{
    free(armature.frame_counts);
    free(armature.time_stamps);
    free(armature.transforms);
    free(armature.child_counts);
    free(armature.hierarchy);
}


typedef struct
{
    model_data_t model;
    armature_t armature;
    vertex_weight_t *vertex_weights;
} animated_data_t;

static inline void free_animated_data(animated_data_t animated)
{
    free_model_data(animated.model);
    free(animated.vertex_weights);
    free(animated.armature.ibms);
    free_armature(animated.armature);
}


typedef struct
{
    unsigned *data;
    int width, height;
} texture_data_t;

static inline void free_texture_data(texture_data_t texture)
{
    free(texture.data);
}


uint8_t *load_image(const char *path, int *width, int *height, int *channels, bool flip);

texture_data_t load_texture_data(const char *path);

model_data_t load_model_data(const char *path);
void print_model_data(const model_data_t *model);

animated_data_t load_animated_data(const char *path);


#endif
