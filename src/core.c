#include "core.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


void GLAPIENTRY opengl_callback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
) {
    (void)source; (void)id; (void)length; (void)userParam;

    int error = type == GL_DEBUG_TYPE_ERROR;

    if (!error)
        return;

    fprintf(stderr, "[%s] type: %d, severity: %d\n%s\n",
            error ? "\033[1;31mERROR\033[0m" : "\033[1mINFO\033[0m",
            type, severity, message
    );
}


void print_context_info(void)
{
    printf("Adaptive vsync: %d\n", bagE_isAdaptiveVsyncAvailable());

    const char *vendor       = (const char*)glGetString(GL_VENDOR);
    const char *renderer     = (const char*)glGetString(GL_RENDERER);
    const char *version      = (const char*)glGetString(GL_VERSION);
    const char *glsl_version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("Vendor: %s\nRenderer: %s\nVersion: %s\nShading Language version: %s\n",
            vendor, renderer, version, glsl_version);
}


static const float cube_vertices[] = {
    // front face
    -1.0f, 1.0f, 1.0f,  // 0      3
    -1.0f,-1.0f, 1.0f,  // 
     1.0f,-1.0f, 1.0f,  // 
     1.0f, 1.0f, 1.0f,  // 1      2
    // back face
    -1.0f, 1.0f,-1.0f,  // 4      7
    -1.0f,-1.0f,-1.0f,  // 
     1.0f,-1.0f,-1.0f,  // 
     1.0f, 1.0f,-1.0f,  // 5      6
};

static const unsigned cube_indices[] = {
    // front face
    0, 1, 2,
    2, 3, 0,
    // right face
    3, 2, 6,
    6, 7, 3,
    // left face
    4, 5, 1,
    1, 0, 4,
    // back face
    7, 6, 5,
    5, 4, 7,
    // top face
    3, 7, 4,
    4, 0, 3,
    // bottom face
    1, 5, 6,
    6, 2, 1,
};

static const unsigned box_indices[] = {
    2, 1, 0,
    0, 3, 2,
    6, 2, 3,
    3, 7, 6,
    1, 5, 4,
    4, 0, 1,
    5, 6, 7,
    7, 4, 5,
    4, 7, 3,
    3, 0, 4,
    6, 5, 1,
    1, 2, 6,
};


model_object_t create_cube_model_object(void)
{
    model_object_t object;

    glCreateBuffers(1, &object.vbo);
    glNamedBufferStorage(object.vbo, sizeof(cube_vertices), cube_vertices, 0);

    glCreateBuffers(1, &object.ebo);
    glNamedBufferStorage(object.ebo, sizeof(cube_indices), cube_indices, 0);

    glCreateVertexArrays(1, &object.vao);
    glVertexArrayVertexBuffer(object.vao, 0, object.vbo, 0, sizeof(float) * 3);
    glEnableVertexArrayAttrib(object.vao, 0);
    glVertexArrayAttribFormat(object.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(object.vao, 0, 0);

    glVertexArrayElementBuffer(object.vao, object.ebo);

    return object;
}


model_object_t create_box_model_object(void)
{
    model_object_t object;

    glCreateBuffers(1, &object.vbo);
    glNamedBufferStorage(object.vbo, sizeof(cube_vertices), cube_vertices, 0);

    glCreateBuffers(1, &object.ebo);
    glNamedBufferStorage(object.ebo, sizeof(box_indices), box_indices, 0);

    glCreateVertexArrays(1, &object.vao);
    glVertexArrayVertexBuffer(object.vao, 0, object.vbo, 0, sizeof(float) * 3);
    glEnableVertexArrayAttrib(object.vao, 0);
    glVertexArrayAttribFormat(object.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(object.vao, 0, 0);

    glVertexArrayElementBuffer(object.vao, object.ebo);

    return object;
}


int load_shader(const char *path, GLenum type)
{
    char *source = read_file(path);
    if (!source)
        exit(1);

    int shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char * const*)&source, NULL);
    glCompileShader(shader);

    int success;
    char info_log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        fprintf(stderr, "[\033[1;31mERROR\033[0m] Failed to compile shader '%s'!\n"
                        "%s", path, info_log);
        exit(1);
    }

    free(source);
    return shader;
}


int load_program(const char *vertex_path, const char *fragment_path)
{
    int vertex_shader   = load_shader(vertex_path,   GL_VERTEX_SHADER);
    int fragment_shader = load_shader(fragment_path, GL_FRAGMENT_SHADER);
    int program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    int success;
    char info_log[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, info_log);
        fprintf(stderr, "[\033[1;31mERROR\033[0m] Failed to link shader program!\n"
                        "%s", info_log);
        exit(1);
    }

    glDetachShader(program, vertex_shader);
    glDeleteShader(vertex_shader);
    glDetachShader(program, fragment_shader);
    glDeleteShader(fragment_shader);

    return program;
}


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


unsigned load_texture(const char *path)
{
    int width, height, channelCount;
    uint8_t *image = load_image(path, &width, &height, &channelCount, true);

    unsigned texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTextureStorage2D(texture, (int)log2(width), GL_RGBA8, width, height);
    glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glGenerateTextureMipmap(texture);

    free(image);

    return texture;
}


model_object_t create_model_object(model_data_t model_data)
{
    model_object_t object;

    glCreateBuffers(1, &object.vbo);
    glNamedBufferStorage(object.vbo, model_data.vertex_count * sizeof(vertex_t), model_data.vertices, 0);

    glCreateBuffers(1, &object.ebo);
    glNamedBufferStorage(object.ebo, model_data.index_count * sizeof(unsigned), model_data.indices, 0);

    glCreateVertexArrays(1, &object.vao);
    glVertexArrayVertexBuffer(object.vao, 0, object.vbo, 0, sizeof(vertex_t));

    glEnableVertexArrayAttrib(object.vao, 0);
    glEnableVertexArrayAttrib(object.vao, 1);
    glEnableVertexArrayAttrib(object.vao, 2);

    glVertexArrayAttribFormat(object.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(object.vao, 1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
    glVertexArrayAttribFormat(object.vao, 2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5);

    glVertexArrayAttribBinding(object.vao, 0, 0);
    glVertexArrayAttribBinding(object.vao, 1, 0);
    glVertexArrayAttribBinding(object.vao, 2, 0);

    glVertexArrayElementBuffer(object.vao, object.ebo);

    object.index_count = model_data.index_count;

    return object;
}


model_object_t load_model_object(const char *path)
{
    model_data_t model_data = load_model_data(path);
    model_object_t object = create_model_object(model_data);
    free_model_data(model_data);
    return object;
}


animated_object_t create_animated_object(animated_data_t animated_data)
{
    animated_object_t object;

    glCreateBuffers(1, &object.model.vbo);
    glNamedBufferStorage(
            object.model.vbo,
            animated_data.model.vertex_count * sizeof(vertex_t),
            animated_data.model.vertices,
            0
    );

    glCreateBuffers(1, &object.weights);
    glNamedBufferStorage(
            object.weights,
            animated_data.model.vertex_count * sizeof(vertex_weight_t),
            animated_data.vertex_weights,
            0
    );

    glCreateBuffers(1, &object.model.ebo);
    glNamedBufferStorage(
            object.model.ebo,
            animated_data.model.index_count * sizeof(unsigned),
            animated_data.model.indices,
            0
    );

    glCreateVertexArrays(1, &object.model.vao);
    glVertexArrayVertexBuffer(object.model.vao, 0, object.model.vbo, 0, sizeof(vertex_t));
    glVertexArrayVertexBuffer(object.model.vao, 1, object.weights, 0, sizeof(vertex_weight_t));

    glEnableVertexArrayAttrib(object.model.vao, 0);
    glEnableVertexArrayAttrib(object.model.vao, 1);
    glEnableVertexArrayAttrib(object.model.vao, 2);
    glEnableVertexArrayAttrib(object.model.vao, 3);
    glEnableVertexArrayAttrib(object.model.vao, 4);

    glVertexArrayAttribFormat(object.model.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(object.model.vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
    glVertexArrayAttribFormat(object.model.vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
    glVertexArrayAttribIFormat(object.model.vao, 3, 4, GL_UNSIGNED_INT, 0);
    glVertexArrayAttribFormat(object.model.vao, 4, 4, GL_FLOAT, GL_FALSE, sizeof(unsigned) * 4);

    glVertexArrayAttribBinding(object.model.vao, 0, 0);
    glVertexArrayAttribBinding(object.model.vao, 1, 0);
    glVertexArrayAttribBinding(object.model.vao, 2, 0);
    glVertexArrayAttribBinding(object.model.vao, 3, 1);
    glVertexArrayAttribBinding(object.model.vao, 4, 1);

    glVertexArrayElementBuffer(object.model.vao, object.model.ebo);

    object.model.index_count = animated_data.model.index_count;

    return object;
}

unsigned load_cube_texture(
        const char *px_path,
        const char *nx_path,
        const char *py_path,
        const char *ny_path,
        const char *pz_path,
        const char *nz_path
) {
    const char *paths[] = { px_path, nx_path, py_path, ny_path, pz_path, nz_path };

    int width, height, channels;
    uint8_t *images[6];

    images[0] = load_image(paths[0], &width, &height, &channels, false);

    assert(width == height);

    for (int i = 1; i < 6; ++i) {
        int w, h, c;
        images[i] = load_image(paths[i], &w, &h, &c, false);

        assert(w == width);
        assert(h == height);
        assert(c == channels);
    }

    unsigned texture;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);
    glTextureStorage2D(texture, (int)log2(width) + 1, GL_RGBA8, width, height);

    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    for (int i = 0; i < 6; ++i) {
        glTextureSubImage3D(
                texture,
                0,
                0, 0, i,
                width, height, 1,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                images[i]
        );

        free(images[i]);
    }

    glGenerateTextureMipmap(texture);

    return texture;
}


unsigned create_buffer_object(size_t size, void *data, unsigned flags)
{
    unsigned buffer;
    glCreateBuffers(1, &buffer);
    glNamedBufferStorage(buffer, size, data, flags);
    return buffer;
}

