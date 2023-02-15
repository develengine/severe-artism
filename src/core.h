#ifndef CORE_H
#define CORE_H

#include "bag_engine.h"
#include "res.h"

#include <stdbool.h>

typedef struct
{
    unsigned vao;
    unsigned vbo;
    unsigned ebo;
    unsigned index_count;
} model_object_t;

static inline void free_model_object(model_object_t object)
{
    glDeleteVertexArrays(1, &object.vao);
    glDeleteBuffers(1, &object.vbo);
    glDeleteBuffers(1, &object.ebo);
}


typedef struct
{
    model_object_t model;
    unsigned weights;
} animated_object_t;

static inline void free_animated_object(animated_object_t object)
{
    free_model_object(object.model);
    glDeleteBuffers(1, &object.weights);
}


void GLAPIENTRY opengl_callback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
);

void print_context_info(void);

int load_shader(const char *path, GLenum type);
int load_program(const char *vertex_path, const char *fragment_path);

uint8_t *load_image(const char *path, int *width, int *height, int *channels, bool flip);
unsigned load_texture(const char *path);

model_object_t create_model_object(model_data_t model);
model_object_t load_model_object(const char *path);

animated_object_t create_animated_object(animated_data_t animated);

unsigned create_buffer_object(size_t size, void *data, unsigned flags);

unsigned load_cube_texture(
        const char *px_path,
        const char *nx_path,
        const char *py_path,
        const char *ny_path,
        const char *pz_path,
        const char *nz_path
);

model_object_t create_cube_model_object(void);
model_object_t create_box_model_object(void);


#define BOX_INDEX_COUNT 36

#endif
