#include "build.h"

/* shared */

const char *output = "program";

const char *source_files[] = {
#ifndef _WIN32
    "src/linux/bag_x11.c",
#else
    "src/windows/bag_win32.c",
    "src/windows/time_win32.c",
#endif

    "src/glad/gl.c",

    "src/main.c",
    "src/utils.c",
    "src/res.c",
    "src/core.c",
    "src/gui.c",
    "src/editor.c",
    "src/obj_parser.c",
    "src/generator.c",
    "src/l_system.c",
    "src/parser.c",

    NULL
};

const char *includes[] = {
    "src",
    "src/glad/include",
    NULL
};

const char *defines[] = {
#ifndef _WIN32
    "_POSIX_C_SOURCE=200809L",
#else
    "_CRT_SECURE_NO_WARNINGS",
#endif
    NULL
};

const char *libs[] = {
#ifndef _WIN32
    "GL",
    "X11",
    "Xi",
    "dl",
    "m",
#else
    "User32.lib",
    "Gdi32.lib",
    "Opengl32.lib",
    "Ole32.lib",
    "ksuser.lib",
#endif
    NULL
};

/* debug */

const char *debug_defines[] = {
    "_DEBUG",
    NULL
};

const char *debug_raw[] = {
#ifndef _WIN32
    "-fno-omit-frame-pointer",
#endif
    NULL
};


int main(int argc, char *argv[])
{
    const char *build_files[] = { "build.c", NULL };
    int res = try_rebuild_self(build_files, argc, argv);
    if (res != -1)
        return res;

    int debug = contains("debug", argc, argv);

    res = compile_w((compile_info_t) {
        .output = output,
        .std = "c17",
        .optimisations = debug ? DEBUG_FLAG : RELEASE_FLAG,

        .source_files = source_files,
        .includes = includes,
        .defines = debug ? merge(defines, debug_defines)
                         : defines,
        .libs = libs,
        .raw_params = debug ? debug_raw : NULL,

        .warnings = nice_warnings,
        .warnings_off = nice_warnings_off,
    });

    if (res)
        return res;

    if (contains("run", argc, argv)) {
        printf("./%s:\n", output);
#ifndef _WIN32
        return execute_w("./%s", output);
#else
        return execute_w("%s.exe", output);
#endif
    }

    return 0;
}

