#ifndef OBJ_PARSER_H
#define OBJ_PARSER_H

#include "res.h"

model_data_t load_obj_file(const char *path);

void model_data_export_to_obj_file(model_data_t data, const char *path);

#endif // OBJ_PARSER_H
