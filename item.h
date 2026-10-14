#ifndef ITEM_H
#define ITEM_H

#include <stdbool.h>
#include <stdio.h>

typedef void ITEM;
// todos os campos do .csv 
typedef struct {
    char asteroid_designation[64];
    char asteroid_fullname[128];
    char close_approach_date[64];
    int year;
    int month;
    double distance_au;
    double velocity_km_s;
    double absolute_magnitude;
    bool has_absolute_magnitude;
    int days_until_approach;
    bool is_past_event;
    bool is_future_event;
    double risk_score;
    int panic_level;
    char threat_category[64];
    char panic_verdict[128];
    bool on_sentry_list;
    
    // Dados opcionais
    bool has_sentry_data;
    double sentry_impact_prob;
    double sentry_torino_scale;
    double sentry_palermo_scale;
    double sentry_diameter_km;
} Asteroid;

void free_asteroid(ITEM *item);
void print_asteroid_csv(const ITEM *item, FILE *out);

// Acessando conteúdo pelo nome do campo
double asteroid_get_numeric_field(const Asteroid *ast, const char *field, bool *valid);
bool asteroid_get_bool_field(const Asteroid *ast, const char *field, bool *valid);
const char* asteroid_get_string_field(const Asteroid *ast, const char *field);

#endif