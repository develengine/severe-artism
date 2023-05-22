#ifndef BUILD_H_
#define BUILD_H_

/* TODO:
 * [ ] file writing and loading
 * [ ] csv timestamp file for auto updating
 * */

#ifndef COMMAND_BUFFER_SIZE
#define COMMAND_BUFFER_SIZE 1024
#endif // COMMAND_BUFFER_SIZE

#ifndef EXECUTE_BUFFER_SIZE
#define EXECUTE_BUFFER_SIZE 1024
#endif // EXECUTE_BUFFER_SIZE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>

#ifndef NO_DATA

const char *nice_warnings[] = {
    "all",
    "extra",
    "pedantic",
    "no-deprecated-declarations",
    "no-missing-field-initializers",
    NULL
};

#endif // NO_DATA


typedef struct
{
    const char *output;
    const char *std;
    const char *optimisations;
    int pedantic;
    int keep_source_info;
    const char *compiler;

    const char **source_files;
    const char **includes;
    const char **defines;
    const char **libs;
    const char **warnings;
    const char **raw_params;
} compile_info_t;

static inline int buffer_append(char *buffer, int pos, const char *str)
{
    int str_len = 0;
    for (; str[str_len]; ++str_len)
        ;;

    if (pos + str_len >= COMMAND_BUFFER_SIZE) {
        fprintf(stderr, "build: COMMAND_BUFFER_SIZE '%d' too small!\n"
                        "       Increase the value of the macro in file '%s'.\n",
                        COMMAND_BUFFER_SIZE, __FILE__);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < str_len; ++i) {
        buffer[pos + i] = str[i];
    }

    return pos + str_len;
}

static inline pid_t compile(compile_info_t info)
{
    /* construct command */

    char buffer[COMMAND_BUFFER_SIZE] = {0};
    int pos = 0;

    if (!info.compiler) {
        info.compiler = "cc";
    }

    pos = buffer_append(buffer, pos, info.compiler);

    if (info.output) {
        pos = buffer_append(buffer, pos, " -o ");
        pos = buffer_append(buffer, pos, info.output);
    }

    if (info.std) {
        pos = buffer_append(buffer, pos, " -std=");
        pos = buffer_append(buffer, pos, info.std);
    }

    if (info.optimisations) {
        pos = buffer_append(buffer, pos, " -O");
        pos = buffer_append(buffer, pos, info.optimisations);
    }

    if (info.pedantic) {
        pos = buffer_append(buffer, pos, " -pedantic");
    }

    if (info.keep_source_info) {
        pos = buffer_append(buffer, pos, " -g");
    }

    if (info.source_files) {
        for (int i = 0; info.source_files[i]; ++i) {
            pos = buffer_append(buffer, pos, " ");
            pos = buffer_append(buffer, pos, info.source_files[i]);
        }
    }

    if (info.includes) {
        for (int i = 0; info.includes[i]; ++i) {
            pos = buffer_append(buffer, pos, " -I");
            pos = buffer_append(buffer, pos, info.includes[i]);
        }
    }

    if (info.defines) {
        for (int i = 0; info.defines[i]; ++i) {
            pos = buffer_append(buffer, pos, " -D");
            pos = buffer_append(buffer, pos, info.defines[i]);
        }
    }

    if (info.libs) {
        for (int i = 0; info.libs[i]; ++i) {
            pos = buffer_append(buffer, pos, " -l");
            pos = buffer_append(buffer, pos, info.libs[i]);
        }
    }

    if (info.warnings) {
        for (int i = 0; info.warnings[i]; ++i) {
            pos = buffer_append(buffer, pos, " -W");
            pos = buffer_append(buffer, pos, info.warnings[i]);
        }
    }

    if (info.raw_params) {
        for (int i = 0; info.raw_params[i]; ++i) {
            pos = buffer_append(buffer, pos, " ");
            pos = buffer_append(buffer, pos, info.raw_params[i]);
        }
    }

    /* execute command */

    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "fork(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
         execl("/bin/sh", "sh", "-c", buffer, NULL);

        fprintf(stderr, "execve(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return pid;
}

static inline int wait_on_exits(pid_t *pids, int count)
{
    siginfo_t info;

    for (int i = 0; i < count; ++i) {
        if (waitid(P_ALL, pids[i], &info, WEXITED) == -1) {
            fprintf(stderr, "waitid(): %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (info.si_status == 0)
            continue;

        return info.si_status;
    }

    return 0;
}

static inline int compile_w(compile_info_t info)
{
    pid_t pid = compile(info);
    return wait_on_exits(&pid, 1);
}

static inline pid_t execute_v(const char *fmt, va_list args)
{
    /* construct command */

    char buffer[EXECUTE_BUFFER_SIZE];

    va_list args_copy;
    va_copy(args_copy, args);

    int to_write = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    int written = vsnprintf(buffer, EXECUTE_BUFFER_SIZE, fmt, args_copy);
    va_end(args_copy);

    if (written != to_write) {
        fprintf(stderr, "build: EXECUTE_BUFFER_SIZE '%d' too small!\n"
                        "       Increase the value of the macro in file '%s'.\n",
                        EXECUTE_BUFFER_SIZE, __FILE__);
        exit(EXIT_FAILURE);
    }

    /* execute command */

    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "fork(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", buffer, NULL);

        fprintf(stderr, "execve(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return pid;
}

static inline pid_t execute(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pid_t pid = execute_v(fmt, args);

    va_end(args);

    return pid;
}

static inline int execute_w(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pid_t pid = execute_v(fmt, args);

    va_end(args);

    return wait_on_exits(&pid, 1);
}

static inline int str_eq(const char *str_a, const char *str_b)
{
    for (; *str_a && *str_b; ++str_a, ++str_b) {
        if (*str_a != *str_b)
            return 0;
    }

    return *str_a == *str_b;
}

static inline int contains(const char *str, int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (str_eq(argv[i], str))
            return 1;
    }

    return 0;
}

static inline const char **merge(const char *arr_1[], const char *arr_2[])
{
    int size = 1;

    for (int i = 0; arr_1[i]; ++i, ++size)
        ;;

    for (int i = 0; arr_2[i]; ++i, ++size)
        ;;

    const char **new_arr = malloc(size * sizeof(const char *));
    assert(new_arr);

    int pos = 0;

    for (int i = 0; arr_1[i]; ++i, ++pos) {
        new_arr[pos] = arr_1[i];
    }

    for (int i = 0; arr_2[i]; ++i, ++pos) {
        new_arr[pos] = arr_2[i];
    }

    new_arr[pos] = NULL;

    return new_arr;
}

#endif // BUILD_H_
