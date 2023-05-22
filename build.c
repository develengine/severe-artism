#include "build.h"

/* shared */

const char *output = "program";

const char *source_files[] = {
    "src/linux/bag_x11.c",
    "src/glad/gl.c",

    "src/main.c",
    "src/utils.c",
    "src/res.c",
    "src/core.c",
    "src/gui.c",

    NULL
};

const char *includes[] = {
    "src",
    "src/glad/include",
    NULL
};

const char *defines[] = {
    "_POSIX_C_SOURCE=200809L",
    NULL
};

const char *libs[] = {
    "GL",
    "X11",
    "Xi",
    "dl",
    "m",
    NULL
};

/* debug */

const char *debug_defines[] = {
    "_DEBUG",
    NULL
};

const char *debug_raw[] = {
    "-fno-omit-frame-pointer",
    NULL
};


int main(int argc, char *argv[])
{
    int debug = contains("debug", argc, argv);

    int res = compile_w((compile_info_t) {
        .output = output,
        .std = "c11",
        .optimisations = debug ? "g" : "2",

        .source_files = source_files,
        .includes = includes,
        .defines = debug ? merge(defines, debug_defines)
                         : defines,
        .libs = libs,
        .raw_params = debug ? debug_raw : NULL,
    });

    if (res)
        return res;

    if (contains("run", argc, argv)) {
        printf("./%s:\n", output);
        return execute_w("./%s", output);
    }

    return 0;
}
