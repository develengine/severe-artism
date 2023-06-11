#include "obj_parser.h"

#include "utils.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct
{
    int vertex_count;
    int texture_count;
    int normal_count;
    int index_count;
} obj_info_t;


static obj_info_t load_info(const char *text)
{
    obj_info_t info = {0};

    const char *at = text;
    while (at != NULL) {
        if (at[1] == '\0')
            break;

        if (at[1] == 'v') {
            if (at[2] == ' ')
                info.vertex_count++;
            else if (at[2] == 't')
                info.texture_count++;
            else if (at[2] == 'n')
                info.normal_count++;
        } else if (at[1] == 'f') {
            int ic = 0;
            for (at += 2; *at != '\0' && *at != '\n'; ++at) {
                if (*at == ' ')
                    ++ic;
            }

            info.index_count += 3 * (ic - 2);

            continue;
        }

        at = strchr(at + 1, '\n');
    }

    return info;
}


typedef struct
{
    int pos_id, tex_id, norm_id;
    int index;
} index_t;


static bool index_eq(const index_t *i1, const index_t *i2)
{
    return i1->pos_id  == i2->pos_id
        && i1->tex_id  == i2->tex_id
        && i1->norm_id == i2->norm_id;
}


model_data_t load_obj_file(const char *path)
{
    char *text = read_file(path);
    if (!text)
        return (model_data_t) {0};

    obj_info_t info = load_info(text);

    float *positions = malloc(
            sizeof(float) * (info.vertex_count * 3 +
                             info.texture_count * 2 +
                             info.normal_count * 3)
    );
    malloc_check(positions);

    float *textures = positions + 3 * info.vertex_count;
    float *normals  = textures  + 2 * info.texture_count;

    index_t *indices = malloc(sizeof(index_t) * info.index_count);
    malloc_check(indices);

    vertex_t *vertices = malloc(sizeof(vertex_t) * info.index_count);
    malloc_check(vertices);

    unsigned *index_data = malloc(sizeof(unsigned) * info.index_count);
    malloc_check(index_data);

    int pos_offset  = 0;
    int tex_offset  = 0;
    int norm_offset = 0;

    int vertex_count = 0;
    int index_count  = 0;

    const char *at = text;
    char *end;
    while (*at != '\0') {
        if (at[1] == '\0')
            break;

        if (at[1] == 'v') {
            if (at[2] == ' ') {
                positions[pos_offset++] = strtof(at + 3, &end);
                at = end;
                positions[pos_offset++] = strtof(at, &end);
                at = end;
                positions[pos_offset++] = strtof(at, &end);
                at = end;
            }
            else if (at[2] == 't') {
                textures[tex_offset++] = strtof(at + 4, &end);
                at = end;
                textures[tex_offset++] = strtof(at, &end);
                at = end;
            }
            else if (at[2] == 'n') {
                normals[norm_offset++] = strtof(at + 4, &end);
                at = end;
                normals[norm_offset++] = strtof(at, &end);
                at = end;
                normals[norm_offset++] = strtof(at, &end);
                at = end;
            }
        }
        else if (at[1] == 'f') {
            index_t inds[3];
            int ic = 0;

            ++at;
            while (*at != '\0' && *at != '\n') {
                if (ic == 3) {
                    inds[1] = inds[2];
                    --ic;
                }

                inds[ic].pos_id  = strtol(++at, &end, 10) - 1;
                at = end;
                inds[ic].tex_id  = strtol(++at, &end, 10) - 1;
                at = end;
                inds[ic].norm_id = strtol(++at, &end, 10) - 1;
                at = end;

                if (++ic == 3) {
                    for (int i = 0; i < 3; ++i) {
                        bool found = false;

                        for (int j = 0; j < vertex_count; ++j) {
                            if (index_eq(inds + i, indices + j)) {
                                index_data[index_count++] = indices[j].index;
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            vertex_t vert;
                            int offset = 3 * inds[i].pos_id;
                            vert.positions[0] = positions[offset];
                            vert.positions[1] = positions[offset + 1];
                            vert.positions[2] = positions[offset + 2];
                            offset = 2 * inds[i].tex_id;
                            vert.textures[0] = textures[offset];
                            vert.textures[1] = textures[offset + 1];
                            offset = 3 * inds[i].norm_id;
                            vert.normals[0] = normals[offset];
                            vert.normals[1] = normals[offset + 1];
                            vert.normals[2] = normals[offset + 2];

                            vertices[vertex_count] = vert;

                            index_t ind = {
                                    inds[i].pos_id,
                                    inds[i].tex_id,
                                    inds[i].norm_id,
                                    vertex_count
                            };

                            indices[vertex_count] = ind;

                            index_data[index_count++] = vertex_count++;
                        }
                    }
                }
            }
        }
        else {
            at = strchr(at + 1, '\n');
        }
    }

    free(positions);
    free(text);

    return (model_data_t) {
        .vertex_count = vertex_count,
        .index_count  = index_count,
        .vertices     = vertices,
        .indices      = index_data,
    };
}

