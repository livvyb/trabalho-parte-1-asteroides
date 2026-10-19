#ifndef QUERY_H
#define QUERY_H

#include "item.h"
#include <stdbool.h>

#define MAX_FILTERS 16
#define MAX_SIMS 16

// arquivo contendo os critérios da busca

typedef enum { OP_EQ, OP_NEQ, OP_LT, OP_LE, OP_GT, OP_GE } OpType;

typedef struct {
    char field[64];
    OpType op;
    char target_str[64];
    double target_num;
    bool is_bool;
    bool target_bool;
} Filter;

typedef struct {
    char field[64];
    double target;
    double tolerance;
    double weight;
} Sim;

typedef struct {
    Filter filters[MAX_FILTERS];
    int filter_count;
    
    Sim sims[MAX_SIMS];
    int sim_count;

    double accept_threshold; // ACCEPT
    char order_key[64];      // ORDER
    bool order_desc;
    
    char group_fields[2][64];// GROUP
    int group_count;
    
    char cross_fields[2][64];// CROSS
    int cross_count;
} Query;

bool carrega_query(const char *filepath, Query *q);
bool avalia_query(const Query *q, const Asteroid *ast, double *out_similarity);

#endif