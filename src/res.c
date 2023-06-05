#include "res.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


uint8_t *load_image(const char *path, int *width, int *height, int *channels, bool flip)
{
    stbi_set_flip_vertically_on_load(flip);

    uint8_t *image = stbi_load(path, width, height, channels, STBI_rgb_alpha);
    if (!image) {
        fprintf(stderr, "Failed to load image \"%s\"\n", path);
        exit(1);
    }

    return image;
}


texture_data_t load_texture_data(const char *path)
{
    texture_data_t res = {0};
    int channelCount;

    res.data = (unsigned *)load_image(path, &res.width, &res.height, &channelCount, true);

    return res;
}


model_data_t load_model_data(const char *path)
{
    model_data_t model_data;
    FILE *file = fopen(path, "rb");
    file_check(file, path);

    safe_read(&model_data.vertex_count, sizeof(int), 1, file);
    safe_read(&model_data.index_count,  sizeof(int), 1, file);

    model_data.vertices = malloc(sizeof(vertex_t) * model_data.vertex_count);
    malloc_check(model_data.vertices);

    model_data.indices = malloc(sizeof(unsigned) * model_data.index_count);
    malloc_check(model_data.indices);

    safe_read(model_data.vertices, sizeof(vertex_t), model_data.vertex_count, file);
    safe_read(model_data.indices,  sizeof(unsigned), model_data.index_count,  file);

    eof_check(file);

    fclose(file);
    return model_data;
}


void modelPrint(const model_data_t *model_data)
{
    printf("vertex count: %d\n", model_data->vertex_count);
    printf("index count:  %d\n", model_data->index_count);

    for (int i = 0; i < model_data->vertex_count; ++i) {
        printf("[%f, %f, %f], [%f, %f], [%f, %f, %f]\n",
                model_data->vertices[i].positions[0],
                model_data->vertices[i].positions[1],
                model_data->vertices[i].positions[2],

                model_data->vertices[i].textures[0],
                model_data->vertices[i].textures[1],

                model_data->vertices[i].normals[0],
                model_data->vertices[i].normals[1],
                model_data->vertices[i].normals[2]
        );
    }

    for (int i = 0; i < model_data->index_count; i += 3) {
        printf("[%u, %u, %u]\n",
                model_data->indices[i],
                model_data->indices[i + 1],
                model_data->indices[i + 2]
        );
    }
}


animated_data_t load_animated_data(const char *path)
{
    animated_data_t animated_data;

    FILE *file = fopen(path, "rb");
    file_check(file, path);

    int vertex_count, index_count, bone_count;
    safe_read(&vertex_count, sizeof(int), 1, file);
    safe_read(&index_count,  sizeof(int), 1, file);
    safe_read(&bone_count,   sizeof(int), 1, file);
    animated_data.model.vertex_count  = vertex_count;
    animated_data.model.index_count   = index_count;
    animated_data.armature.bone_count = bone_count;

    animated_data.model.vertices = malloc(sizeof(vertex_t) * vertex_count);
    animated_data.model.indices  = malloc(sizeof(unsigned) * index_count);
    malloc_check(animated_data.model.vertices);
    malloc_check(animated_data.model.indices);
    safe_read(animated_data.model.vertices, sizeof(vertex_t), vertex_count, file);
    safe_read(animated_data.model.indices, sizeof(unsigned), index_count, file);

    animated_data.vertex_weights = malloc(sizeof(vertex_weight_t) * vertex_count);
    animated_data.armature.ibms = malloc(sizeof(matrix_t) * bone_count);
    malloc_check(animated_data.vertex_weights);
    malloc_check(animated_data.armature.ibms);
    safe_read(animated_data.vertex_weights, sizeof(vertex_weight_t), vertex_count, file);
    safe_read(animated_data.armature.ibms, sizeof(matrix_t), bone_count, file);

    animated_data.armature.frame_counts = malloc(sizeof(unsigned) * bone_count);
    animated_data.armature.frame_offsets = malloc(sizeof(unsigned) * bone_count);
    malloc_check(animated_data.armature.frame_counts);
    malloc_check(animated_data.armature.frame_offsets);
    safe_read(animated_data.armature.frame_counts, sizeof(unsigned), bone_count, file);

    int frame_count = 0;
    for (int i = 0; i < bone_count; ++i) {
        animated_data.armature.frame_offsets[i] = frame_count;
        frame_count += animated_data.armature.frame_counts[i];
    }

    animated_data.armature.time_stamps = malloc(sizeof(float) * frame_count);
    animated_data.armature.transforms = malloc(sizeof(joint_transform_t) * frame_count);
    malloc_check(animated_data.armature.time_stamps);
    malloc_check(animated_data.armature.transforms);
    safe_read(animated_data.armature.time_stamps, sizeof(float), frame_count, file);
    safe_read(animated_data.armature.transforms, sizeof(joint_transform_t), frame_count, file);

    animated_data.armature.child_counts = malloc(sizeof(unsigned) * bone_count);
    animated_data.armature.child_offsets = malloc(sizeof(unsigned) * bone_count);
    malloc_check(animated_data.armature.child_counts);
    malloc_check(animated_data.armature.child_offsets);
    safe_read(animated_data.armature.child_counts, sizeof(unsigned), bone_count, file);

    int children_count = 0;
    for (int i = 0; i < bone_count; ++i) {
        animated_data.armature.child_offsets[i] = children_count;
        children_count += animated_data.armature.child_counts[i];
    }

    animated_data.armature.hierarchy = malloc(sizeof(unsigned) * children_count);
    malloc_check(animated_data.armature.hierarchy);
    safe_read(animated_data.armature.hierarchy, sizeof(unsigned), children_count, file);

    eof_check(file);

    fclose(file);
    return animated_data;
}

