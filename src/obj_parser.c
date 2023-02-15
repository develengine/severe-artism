#include "utils.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct
{
    int vertexCount;
    int textureCount;
    int normalCount;
    int indexCount;
} ObjInfo;


static ObjInfo loadInfo(const char *text)
{
    ObjInfo info = {0};

    const char *at = text;
    while (at != NULL) {
        if (at[1] == '\0')
            break;

        if (at[1] == 'v') {
            if (at[2] == ' ')
                info.vertexCount++;
            else if (at[2] == 't')
                info.textureCount++;
            else if (at[2] == 'n')
                info.normalCount++;
        } else if (at[1] == 'f') {
            int ic = 0;
            for (at += 2; *at != '\0' && *at != '\n'; ++at) {
                if (*at == ' ')
                    ++ic;
            }

            info.indexCount += 3 * (ic - 2);

            continue;
        }

        at = strchr(at + 1, '\n');
    }

    return info;
}


typedef struct
{
    int posID, texID, normID;
    int index;
} Index;


typedef struct
{
    float positions[3];
    float textures[2];
    float normals[3];
} Vertex;


static bool indexEq(const Index *i1, const Index *i2)
{
    return i1->posID  == i2->posID
        && i1->texID  == i2->texID
        && i1->normID == i2->normID;
}


static void parse(const ObjInfo *info, const char *text, FILE *out)
{
    float *positions, *textures, *normals;

    positions = malloc(
            sizeof(float) * (info->vertexCount * 3 + info->textureCount * 2 + info->normalCount * 3)
    );
    malloc_check(positions);

    textures = positions + 3 * info->vertexCount;
    normals  = textures  + 2 * info->textureCount;

    Index *indices = malloc(sizeof(Index) * info->indexCount);
    malloc_check(indices);

    Vertex *vertices = malloc(sizeof(Vertex) * info->indexCount);
    malloc_check(vertices);

    unsigned *indexData = malloc(sizeof(unsigned) * info->indexCount);
    malloc_check(indexData);

    int posOffset  = 0;
    int texOffset  = 0;
    int normOffset = 0;

    int vertexCount = 0;
    int indexCount = 0;

    const char *at = text;
    char *end;
    while (*at != '\0') {
        if (at[1] == '\0')
            break;

        if (at[1] == 'v') {
            if (at[2] == ' ') {
                positions[posOffset++] = strtof(at + 3, &end);
                at = end;
                positions[posOffset++] = strtof(at, &end);
                at = end;
                positions[posOffset++] = strtof(at, &end);
                at = end;
            } else if (at[2] == 't') {
                textures[texOffset++] = strtof(at + 4, &end);
                at = end;
                textures[texOffset++] = strtof(at, &end);
                at = end;
            } else if (at[2] == 'n') {
                normals[normOffset++] = strtof(at + 4, &end);
                at = end;
                normals[normOffset++] = strtof(at, &end);
                at = end;
                normals[normOffset++] = strtof(at, &end);
                at = end;
            }
        } else if (at[1] == 'f') {
            Index inds[3];
            int ic = 0;

            ++at;
            while (*at != '\0' && *at != '\n') {
                if (ic == 3) {
                    inds[1] = inds[2];
                    --ic;
                }

                inds[ic].posID  = strtol(++at, &end, 10) - 1;
                at = end;
                inds[ic].texID  = strtol(++at, &end, 10) - 1;
                at = end;
                inds[ic].normID = strtol(++at, &end, 10) - 1;
                at = end;

                if (++ic == 3) {
                    for (int i = 0; i < 3; ++i) {
                        bool found = false;

                        // TODO heap search
                        for (int j = 0; j < vertexCount; ++j) {
                            if (indexEq(inds + i, indices + j)) {
                                indexData[indexCount++] = indices[j].index;
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            Vertex vert;
                            int offset = 3 * inds[i].posID;
                            vert.positions[0] = positions[offset];
                            vert.positions[1] = positions[offset + 1];
                            vert.positions[2] = positions[offset + 2];
                            offset = 2 * inds[i].texID;
                            vert.textures[0] = textures[offset];
                            vert.textures[1] = textures[offset + 1];
                            offset = 3 * inds[i].normID;
                            vert.normals[0] = normals[offset];
                            vert.normals[1] = normals[offset + 1];
                            vert.normals[2] = normals[offset + 2];

                            vertices[vertexCount] = vert;

                            Index ind = {
                                    inds[i].posID,
                                    inds[i].texID,
                                    inds[i].normID,
                                    vertexCount
                            };

                            indices[vertexCount] = ind;

                            indexData[indexCount++] = vertexCount++;
                        }
                    }
                }
            }
        } else {
            at = strchr(at + 1, '\n');
        }
    }

    // TODO FIXME write failure handling
    safe_write(&vertexCount, sizeof(int), 1, out);
    safe_write(&indexCount, sizeof(int), 1, out);
    safe_write(vertices, sizeof(Vertex), vertexCount, out);
    safe_write(indexData, sizeof(unsigned), indexCount, out);

    printf("v: %d\n", vertexCount);
    printf("i: %d\n", indexCount);

    free(positions);
    free(indices);
    free(vertices);
    free(indexData);
}


int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "use: obj_parser <input-file> <output-file>\n");
        exit(1);
    }

    char *text = read_file(argv[1]);

    ObjInfo info = loadInfo(text);

    FILE *out = fopen(argv[2], "wb");
    file_check(out, argv[2]);

    parse(&info, text, out);

    fclose(out);
    free(text);
    return 0;
}
