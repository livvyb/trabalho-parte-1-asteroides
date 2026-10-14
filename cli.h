#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

typedef struct {
    char reader[64];
    char input_path[256];
    char base_struct[64];
    char result_struct[64];
    char query_path[256];
    char output_path[256];
    int top_k;
    int limit;
} ARGS;

bool parse_cli_args(int argc, char *argv[], ARGS *args);

#endif