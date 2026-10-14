#include "item.h"
#include <stdlib.h>
#include <string.h>

void free_asteroid(ITEM *item) {
    if (item) free(item);
}

void print_asteroid_csv(const ITEM *item, FILE *out) {
    if (!item || !out) return;
    const Asteroid *a = (const Asteroid *)item;

    fprintf(out, "%s,%s,%s,%d,%d,%.16g,%.16g,", 
            a->asteroid_designation, a->asteroid_fullname, 
            a->close_approach_date, a->year, a->month, 
            a->distance_au, a->velocity_km_s);

    if (a->has_absolute_magnitude) fprintf(out, "%.16g,", a->absolute_magnitude);
    else fprintf(out, ",");

    fprintf(out, "%d,%s,%s,%.16g,%d,%s,%s,%s,",
            a->days_until_approach,
            a->is_past_event ? "True" : "False",
            a->is_future_event ? "True" : "False",
            a->risk_score, a->panic_level,
            a->threat_category, a->panic_verdict,
            a->on_sentry_list ? "True" : "False");

    if (a->has_sentry_data) {
        fprintf(out, "%.16g,%.16g,%.16g,%.16g\n",
                a->sentry_impact_prob, a->sentry_torino_scale,
                a->sentry_palermo_scale, a->sentry_diameter_km);
    } else {
        fprintf(out, ",,,\n");
    }
}

double asteroid_get_numeric_field(const Asteroid *ast, const char *field, bool *valid) {
    *valid = true;
    if (strcmp(field, "distance_au") == 0) return ast->distance_au;
    if (strcmp(field, "velocity_km_s") == 0) return ast->velocity_km_s;
    if (strcmp(field, "absolute_magnitude") == 0) {
        *valid = ast->has_absolute_magnitude;
        return ast->absolute_magnitude;
    }
    if (strcmp(field, "risk_score") == 0) return ast->risk_score;
    if (strcmp(field, "year") == 0) return (double)ast->year;
    if (strcmp(field, "month") == 0) return (double)ast->month;
    if (strcmp(field, "panic_level") == 0) return (double)ast->panic_level;
    
    *valid = false;
    return 0.0;
}

bool asteroid_get_bool_field(const Asteroid *ast, const char *field, bool *valid) {
    *valid = true;
    if (strcmp(field, "is_past_event") == 0) return ast->is_past_event;
    if (strcmp(field, "is_future_event") == 0) return ast->is_future_event;
    if (strcmp(field, "on_sentry_list") == 0) return ast->on_sentry_list;
    
    *valid = false;
    return false;
}

const char* asteroid_get_string_field(const Asteroid *ast, const char *field) {
    if (strcmp(field, "asteroid_designation") == 0) return ast->asteroid_designation;
    if (strcmp(field, "threat_category") == 0) return ast->threat_category;
    if (strcmp(field, "panic_verdict") == 0) return ast->panic_verdict;
    return "";
}