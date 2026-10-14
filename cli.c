#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool parse_cli_args(int argc, char *argv[], ARGS *args) {
    // Valores padrão
    memset(args, 0, sizeof(ARGS));
    args->top_k = 0;   // maximo de registros exportados -> 0 = todos
    args->limit = 0;   // maximo de registros carregados -> 0 = toda a base

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--reader") == 0 && i + 1 < argc) {
            strncpy(args->reader, argv[++i], sizeof(args->reader) - 1);
        } else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            strncpy(args->input_path, argv[++i], sizeof(args->input_path) - 1);
        } else if (strcmp(argv[i], "--base") == 0 && i + 1 < argc) {
            strncpy(args->base_struct, argv[++i], sizeof(args->base_struct) - 1);
        } else if (strcmp(argv[i], "--result") == 0 && i + 1 < argc) {
            strncpy(args->result_struct, argv[++i], sizeof(args->result_struct) - 1);
        } else if (strcmp(argv[i], "--query") == 0 && i + 1 < argc) {
            strncpy(args->query_path, argv[++i], sizeof(args->query_path) - 1);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            strncpy(args->output_path, argv[++i], sizeof(args->output_path) - 1);
        } else if (strcmp(argv[i], "--top-k") == 0 && i + 1 < argc) {
            args->top_k = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            args->limit = atoi(argv[++i]);
        }
    }

    if (strlen(args->input_path) == 0 || strlen(args->query_path) == 0) {
        return false;
    }
    return true;
}