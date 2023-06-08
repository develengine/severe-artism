#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


#define length(x) (sizeof(x)/sizeof(*x))

#define NOT_IMPLEMENTED 0

#ifdef _DEBUG
    #define unreachable()                                                           \
    do {                                                                            \
        fprintf(stderr, "%s:%d: Unreachable line reached.\n", __FILE__, __LINE__);  \
        exit(666);                                                                  \
    } while (0)
#else
    #ifdef _WIN32
        #define unreachable()   assume(0)
    #else
        #define unreachable()   __builtin_unreachable()
    #endif
#endif

#define safe_read(buffer, size, count, stream)                              \
do {                                                                        \
    if (fread(buffer, size, count, stream) != (size_t)count) {              \
        fprintf(stderr, "%s:%d: fread failure! %s. exiting...\n",           \
                __FILE__, __LINE__, feof(stream) ? "EOF reached" : "Error");\
        exit(666);                                                          \
    }                                                                       \
} while (0)

#define safe_write(buffer, size, count, stream)                                     \
do {                                                                                \
    if (fwrite(buffer, size, count, stream) != (size_t)count) {                     \
        fprintf(stderr, "%s:%d: fwrite failure! exiting...\n", __FILE__, __LINE__); \
        exit(666);                                                                  \
    }                                                                               \
} while (0)

#define malloc_check(ptr)                                                           \
do {                                                                                \
    if (!(ptr)) {                                                                   \
        fprintf(stderr, "%s:%d: malloc failure! exiting...\n", __FILE__, __LINE__); \
        exit(666);                                                                  \
    }                                                                               \
} while (0)

#define file_check(file, path)                                          \
do {                                                                    \
    if (!(file)) {                                                      \
        fprintf(stderr, "%s:%d: fopen failure! path: \"%s\".\n",        \
                __FILE__, __LINE__, path);                              \
        exit(666);                                                      \
    }                                                                   \
} while (0)

#define eof_check(file)                                                             \
do {                                                                                \
    if (fgetc(file) != EOF) {                                                       \
        fprintf(stderr, "%s:%d: expected EOF! exiting...\n", __FILE__, __LINE__);   \
        exit(666);                                                                  \
    }                                                                               \
} while (0)

/* arguments must be lvalues, except for `item` */
#define safe_push(buffer, size, capacity, item)                             \
do {                                                                        \
    if ((size) == (capacity)) {                                             \
        capacity = (capacity) ? (capacity) * 2 : 1;                         \
        void *new_ptr = realloc(buffer, sizeof(*(buffer)) * (capacity));    \
        if (!new_ptr) {                                                     \
            new_ptr = malloc(sizeof(*(buffer)) * (capacity));               \
            malloc_check(new_ptr);                                          \
            memcpy(new_ptr, buffer, sizeof(*(buffer)) * (size));            \
            free(buffer);                                                   \
        }                                                                   \
        buffer = new_ptr;                                                   \
    }                                                                       \
    buffer[size] = item;                                                    \
    ++(size);                                                               \
} while (0)

/* arguments must be lvalues, except for `amount` */
#define safe_expand(buffer, size, capacity, amount)                         \
do {                                                                        \
    capacity = (capacity) + (amount);                                       \
    void *new_ptr = realloc(buffer, sizeof(*(buffer)) * (capacity));        \
    if (!new_ptr) {                                                         \
        new_ptr = malloc(sizeof(*(buffer)) * (capacity));                   \
        malloc_check(new_ptr);                                              \
        memcpy(new_ptr, buffer, sizeof(*(buffer)) * (size));                \
        free(buffer);                                                       \
    }                                                                       \
    buffer = new_ptr;                                                       \
} while (0)

/* arguments must be lvalues, except for `amount` */
#define queue_init(queue, capacity, start, end, amount) \
do {                                                    \
    assert(amount > 0);                                 \
    queue = malloc(sizeof(*(queue)) * amount);          \
    malloc_check(queue);                                \
    capacity = amount;                                  \
    start = 0;                                          \
    end = 0;                                            \
} while (0)

/* arguments must be lvalues, except for `item` */
#define queue_push(queue, capacity, start, end, item)                             \
do {                                                                                    \
    queue[end] = item;                                                                  \
    if (++(end) == (capacity))                                                          \
        end = 0;                                                                        \
    if ((end) == (start)) {                                                             \
        int type_size = sizeof(__typeof__(*(queue)));                                   \
        int new_capacity = (capacity) * 2;                                              \
        __typeof__(queue) new_ptr = realloc(queue, type_size * new_capacity);           \
        if (!new_ptr) {                                                                 \
            new_ptr = malloc(type_size * new_capacity);                                 \
            malloc_check(new_ptr);                                                      \
            memcpy(new_ptr, (queue) + (end), type_size * ((capacity) - (end)));         \
            memcpy(new_ptr + (capacity) - (end), queue, type_size * (end));             \
            free(queue);                                                                \
            start = 0;                                                                  \
            end = capacity;                                                             \
        } else {                                                                        \
            memcpy(new_ptr + (capacity), new_ptr, type_size * (end));                   \
            end = (capacity) + (end);                                                   \
        }                                                                               \
        capacity = new_capacity;                                                        \
        queue = new_ptr;                                                                \
    }                                                                                   \
} while (0)

/* arguments must be lvalues */
#define queue_pop(queue, capacity, start, end)  \
do {                                            \
    if (++(start) == (capacity))                \
        start = 0;                              \
} while (0)


char *read_file(const char *name);

#endif
