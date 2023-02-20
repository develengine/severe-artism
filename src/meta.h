#ifndef META_H
#define META_H

#include <stdio.h>


#define meta_insert_header(file)                                      \
do {                                                                            \
    FILE *stream = (file);                                                      \
    fprintf(stream, "/* THIS IS A GENERATED FILE\n"                             \
                    " * \n"                                                     \
                    " * generated in: %s:%d\n"                                  \
                    " * \n"                                                     \
                    " * If you are interested in understanding or modifying\n"  \
                    " * this file, you should look into \"%s\" */\n\n",         \
                    __FILE__, __LINE__, __FILE__);                              \
} while(0)

#endif // META_H
