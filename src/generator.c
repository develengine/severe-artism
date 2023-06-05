#include "generator.h"

#include "utils.h"


static inline void stretch(model_builder_t *builder, int vertex_count, int index_count)
{
    int new_count = builder->data.vertex_count + vertex_count;

    if (new_count > builder->vertex_capacity) {
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

    new_count = builder->data.index_count + index_count;

    if (new_count > builder->index_capacity) {
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
        vertex_t vert = data.vertices[i];

        vector_t p = {
            vert.positions[0],
            vert.positions[1],
            vert.positions[2],
            1.0f
        };

        p = vector_transform(p, transform);

        vert.positions[0] = p.x;
        vert.positions[1] = p.y;
        vert.positions[2] = p.z;

        builder->data.vertices[vertex_count + i] = vert;
    }

    builder->data.vertex_count += data.vertex_count;


    int index_count = builder->data.index_count;

    for (int i = 0; i < data.index_count; ++i) {
        builder->data.indices[index_count + i] = vertex_count + data.indices[i];
    }

    builder->data.index_count += data.index_count;
}


model_data_t generate_cylinder(int n, frect_t view)
{
    assert(n > 1);

    int vertex_count = 2            // central vertices
                     + (n + 1) * 4; // edges

    int index_count = n * 6  // sides
                    + n * 6; // top and bottom slices

    vertex_t *vertices = malloc(vertex_count * sizeof(vertex_t));
    malloc_check(vertices);

    unsigned *indices = malloc(index_count * sizeof(unsigned));
    malloc_check(indices);

    unsigned vpos = 0;
    unsigned ipos = 0;

    float angle = (float)((2.0 * M_PI) / n);

    /* sides */
    {
        unsigned top_prev = vpos;
        vertices[vpos++] = (vertex_t) {
            .positions = { 1.0f, 1.0f, 0.0f },
            .textures  = { view.x, view.y + view.h },
            .normals   = { 1.0f, 0.0f, 0.0f },
        };

        unsigned bottom_prev = vpos;
        vertices[vpos++] = (vertex_t) {
            .positions = { 1.0f,-1.0f, 0.0f },
            .textures  = { view.x, view.y },
            .normals   = { 1.0f, 0.0f, 0.0f },
        };

        for (int i = 1; i <= n; ++i) {
            float x = cosf(angle * i);
            float y = sinf(angle * i);

            unsigned top = vpos;
            vertices[vpos++] = (vertex_t) {
                .positions = { x, 1.0f, y },
                .textures  = { view.x + view.w * ((float)i / n),
                               view.y + view.h },
                .normals   = { x, 0.0f, y },
            };

            unsigned bottom = vpos;
            vertices[vpos++] = (vertex_t) {
                .positions = { x,-1.0f, y },
                .textures  = { view.x + view.w * ((float)i / n),
                               view.y },
                .normals   = { x, 0.0f, y },
            };

            indices[ipos++] = top;
            indices[ipos++] = bottom;
            indices[ipos++] = top_prev;

            indices[ipos++] = bottom;
            indices[ipos++] = bottom_prev;
            indices[ipos++] = top_prev;

            top_prev    = top;
            bottom_prev = bottom;
        }
    }

    /* top and bottom */
    {
        unsigned top_center = vpos;
        vertices[vpos++] = (vertex_t) {
            .positions = { 0.0f, 1.0f, 0.0f },
            .textures  = { view.x + view.w * 0.5f,
                           view.y + view.h * 0.5f },
            .normals   = { 0.0f, 1.0f, 0.0f },
        };

        unsigned bottom_center = vpos;
        vertices[vpos++] = (vertex_t) {
            .positions = { 0.0f,-1.0f, 0.0f },
            .textures  = { view.x + view.w * 0.5f,
                           view.y + view.h * 0.5f },
            .normals   = { 0.0f,-1.0f, 0.0f },
        };

        unsigned top_prev = vpos;
        vertices[vpos++] = (vertex_t) {
            .positions = { 1.0f, 1.0f, 0.0f },
            .textures  = { view.x + view.w,
                           view.y + view.h * 0.5f },
            .normals   = { 0.0f, 1.0f, 0.0f },
        };

        unsigned bottom_prev = vpos;
        vertices[vpos++] = (vertex_t) {
            .positions = { 1.0f,-1.0f, 0.0f },
            .textures  = { view.x + view.w,
                           view.y + view.h * 0.5f },
            .normals   = { 0.0f,-1.0f, 0.0f },
        };

        for (int i = 1; i <= n; ++i) {
            float x = cosf(angle * i);
            float y = sinf(angle * i);

            unsigned top = vpos;
            vertices[vpos++] = (vertex_t) {
                .positions = { x, 1.0f, y },
                .textures  = { view.x + view.w * (x * 0.5f + 0.5f),
                               view.y + view.h * (y * 0.5f + 0.5f) },
                .normals   = { 0.0f, 1.0f, 0.0f },
            };

            unsigned bottom = vpos;
            vertices[vpos++] = (vertex_t) {
                .positions = { x,-1.0f, y },
                .textures  = { view.x + view.w * (x * 0.5f + 0.5f),
                               view.y + view.h * (y * 0.5f + 0.5f) },
                .normals   = { 0.0f,-1.0f, 0.0f },
            };

            indices[ipos++] = top_center;
            indices[ipos++] = top;
            indices[ipos++] = top_prev;

            indices[ipos++] = bottom_center;
            indices[ipos++] = bottom_prev;
            indices[ipos++] = bottom;

            top_prev    = top;
            bottom_prev = bottom;
        }
    }

    return (model_data_t) {
        .vertex_count = vpos,
        .index_count  = ipos,
        .vertices = vertices,
        .indices  = indices,
    };
}


model_data_t generate_quad_sphere(int n, frect_t view)
{
    assert(n >= 0);

    int vertex_count = (n + 2) * (n + 2) // one side
                     * 6;                // side count

    int index_count = (n + 1) * (n + 1) // quads per face
                    * 6                 // verts per quad
                    * 6;                // side count

    vertex_t *vertices = malloc(vertex_count * sizeof(vertex_t));
    malloc_check(vertices);

    unsigned *indices = malloc(index_count * sizeof(unsigned));
    malloc_check(indices);

    unsigned vpos = 0;
    unsigned ipos = 0;

#define _z_init { .x = ((float)j / (n + 1)) * 2.0f - 1.0f,      \
                  .y = ((float)i / (n + 1)) * 2.0f - 1.0f,      \
                  .z = 1.0f }

#define _nz_init { .x = -(((float)j / (n + 1)) * 2.0f - 1.0f),  \
                   .y =   ((float)i / (n + 1)) * 2.0f - 1.0f,   \
                   .z = -1.0f }

#define _nx_init { .z = ((float)j / (n + 1)) * 2.0f - 1.0f,     \
                   .y = ((float)i / (n + 1)) * 2.0f - 1.0f,     \
                   .x = -1.0f }

#define _x_init { .z = -(((float)j / (n + 1)) * 2.0f - 1.0f),   \
                  .y =   ((float)i / (n + 1)) * 2.0f - 1.0f,    \
                  .x = 1.0f }

#define _y_init { .x = ((float)j / (n + 1)) * 2.0f - 1.0f,      \
                  .z = ((float)i / (n + 1)) * 2.0f - 1.0f,      \
                  .y = -1.0f }

#define _ny_init { .x = -(((float)j / (n + 1)) * 2.0f - 1.0f),  \
                   .z =   ((float)i / (n + 1)) * 2.0f - 1.0f,   \
                   .y = 1.0f }

#define _make_side(init)                                        \
do {                                                            \
    unsigned prev_column = vpos;                                \
                                                                \
    for (int j = 0; j <= n + 1; ++j) {                          \
        unsigned column = vpos;                                 \
                                                                \
        for (int i = 0; i <= n + 1; ++i) {                      \
            vec3_t v = init;                                    \
                                                                \
            v = vec3_normalize(v);                              \
                                                                \
            vertices[vpos++] = (vertex_t) {                     \
                .positions = { v.x, v.y, v.z },                 \
                .textures  = {                                  \
                    view.x + view.w * ((float)j / (n + 1)),     \
                    view.y + view.h * ((float)i / (n + 1))      \
                },                                              \
                .normals   = { v.x, v.y, v.z },                 \
            };                                                  \
        }                                                       \
                                                                \
        if (j == 0)                                             \
            continue;                                           \
                                                                \
        for (int i = 0; i < n + 1; ++i) {                       \
            unsigned tl = prev_column + i;                      \
            unsigned bl = prev_column + i + 1;                  \
            unsigned tr = column + i;                           \
            unsigned br = column + i + 1;                       \
                                                                \
            indices[ipos++] = tr;                               \
            indices[ipos++] = br;                               \
            indices[ipos++] = tl;                               \
                                                                \
            indices[ipos++] = br;                               \
            indices[ipos++] = bl;                               \
            indices[ipos++] = tl;                               \
        }                                                       \
                                                                \
        prev_column = column;                                   \
    }                                                           \
} while (0)

    _make_side(_z_init);
    _make_side(_nz_init);
    _make_side(_x_init);
    _make_side(_nx_init);
    _make_side(_y_init);
    _make_side(_ny_init);

#undef _z_init
#undef _nz_init
#undef _x_init
#undef _nx_init
#undef _y_init
#undef _ny_init

#undef _make_side

    return (model_data_t) {
        .vertex_count = vpos,
        .index_count  = ipos,
        .vertices = vertices,
        .indices  = indices,
    };
}


static inline ivec2_t pack_rects_shitly(rect_t *rects, int count)
{
    int width = 0;
    int height = 0;

    for (int i = 0; i < count; ++i) {
        rect_t rect = rects[i];

        rects[i].x = 0;
        rects[i].y = height;

        height += rect.h;

        if (rect.w > width) {
            width = rect.w;
        }
    }

    return (ivec2_t) { width, height };
}


texture_data_t create_texture_atlas(texture_data_t *textures, rect_t *views, int count)
{
    for (int i = 0; i < count; ++i) {
        views[i].w = textures[i].width;
        views[i].h = textures[i].height;
    }

    ivec2_t size = pack_rects_shitly(views, count);

    texture_data_t res = {
        .width = size.x,
        .height = size.y,
    };

    res.data = malloc(size.x * size.y * sizeof(unsigned));
    malloc_check(res.data);

    for (int tex_id = 0; tex_id < count; ++tex_id) {
        texture_data_t texture = textures[tex_id];
        rect_t view = views[tex_id];

        for (int y = 0; y < texture.height; ++y) {
            for (int x = 0; x < texture.width; ++x) {
                unsigned pixel = texture.data[x + y * texture.width];
                res.data[(view.x + x) + (view.y + y) * res.width] = pixel;
            }
        }
    }

    return res;
}

void model_map_textures_to_view(model_data_t *data, frect_t view)
{
    for (int i = 0; i < data->vertex_count; ++i) {
        data->vertices[i].textures[0] = view.x + view.w * data->vertices[i].textures[0];
        data->vertices[i].textures[1] = view.y + view.h * data->vertices[i].textures[1];
    }
}
