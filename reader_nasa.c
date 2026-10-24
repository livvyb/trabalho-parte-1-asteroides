#include "reader_nasa.h"
#include <stdlib.h>
#include <string.h>

// Posição de cada coluna no CSV
enum {
    COL_DESIGNATION,
    COL_FULLNAME,
    COL_DATE,
    COL_YEAR,
    COL_MONTH,
    COL_DISTANCE,
    COL_VELOCITY,
    COL_MAGNITUDE,
    COL_DAYS,
    COL_PAST,
    COL_FUTURE,
    COL_RISK,
    COL_PANIC,
    COL_CATEGORY,
    COL_VERDICT,
    COL_SENTRY,
    COL_IMPACT_PROB,
    COL_TORINO,
    COL_PALERMO,
    COL_DIAMETER,
    TOTAL_CAMPOS
};

// Cabeçalho esperado (também usado para achar um campo pelo nome)
static const char *COLUNAS[TOTAL_CAMPOS] = {
    "asteroid_designation",
    "asteroid_fullname",
    "close_approach_date",
    "year",
    "month",
    "distance_au",
    "velocity_km_s",
    "absolute_magnitude",
    "days_until_approach",
    "is_past_event",
    "is_future_event",
    "risk_score",
    "panic_level",
    "threat_category",
    "panic_verdict",
    "on_sentry_list",
    "sentry_impact_prob",
    "sentry_torino_scale",
    "sentry_palermo_scale",
    "sentry_diameter_km"
};

// Registro de asteroide, visível apenas neste arquivo
typedef struct {
    char asteroid_designation[64];
    char asteroid_fullname[128];
    char close_approach_date[32];
    int year;
    int month;
    double distance_au;
    double velocity_km_s;
    double absolute_magnitude;
    int days_until_approach;
    bool is_past_event;
    bool is_future_event;
    double risk_score;
    int panic_level;
    char threat_category[32];
    char panic_verdict[128];
    bool on_sentry_list;
    double sentry_impact_prob;
    double sentry_torino_scale;
    double sentry_palermo_scale;
    double sentry_diameter_km;
    bool presente[TOTAL_CAMPOS]; // false quando o campo veio vazio (ausente)
} Asteroid;

/* Funções de conversão: campo vazio é marcado como ausente e não é erro.
   Só retornam false quando o campo está preenchido com um valor inválido
   (ex.: "abc" em distance_au). */

static bool le_texto(const char *texto, char *destino, size_t tamanho, bool *presente) {
    while (*texto == ' ') {
        texto++;
    }
    *presente = texto[0] != '\0';
    if (strlen(texto) >= tamanho) {
        return false;
    }
    strcpy(destino, texto);
    return true;
}

static bool le_inteiro(const char *texto, int *destino, bool *presente) {
    char *fim;

    *presente = texto[0] != '\0';
    if (!*presente) {
        return true;
    }
    *destino = (int)strtol(texto, &fim, 10);
    return *fim == '\0'; // o texto inteiro precisa ser um número
}

static bool le_real(const char *texto, double *destino, bool *presente) {
    char *fim;

    *presente = texto[0] != '\0';
    if (!*presente) {
        return true;
    }
    *destino = strtod(texto, &fim);
    return *fim == '\0';
}

static bool le_bool(const char *texto, bool *destino, bool *presente) {
    *presente = texto[0] != '\0';
    if (!*presente) {
        return true;
    }
    if (strcmp(texto, "True") == 0 || strcmp(texto, "true") == 0) {
        *destino = true;
        return true;
    }
    if (strcmp(texto, "False") == 0 || strcmp(texto, "false") == 0) {
        *destino = false;
        return true;
    }
    return false;
}

// Converte os campos de uma linha em Asteroid. Retorna NULL se algum for inválido
static void *nasa_converter(char **campos) {
    Asteroid *a = malloc(sizeof(Asteroid));
    bool *p;

    if (a == NULL) {
        return NULL;
    }
    p = a->presente;

    if (le_texto(campos[COL_DESIGNATION], a->asteroid_designation, sizeof(a->asteroid_designation), &p[COL_DESIGNATION]) &&
        le_texto(campos[COL_FULLNAME], a->asteroid_fullname, sizeof(a->asteroid_fullname), &p[COL_FULLNAME]) &&
        le_texto(campos[COL_DATE], a->close_approach_date, sizeof(a->close_approach_date), &p[COL_DATE]) &&
        le_inteiro(campos[COL_YEAR], &a->year, &p[COL_YEAR]) &&
        le_inteiro(campos[COL_MONTH], &a->month, &p[COL_MONTH]) &&
        le_real(campos[COL_DISTANCE], &a->distance_au, &p[COL_DISTANCE]) &&
        le_real(campos[COL_VELOCITY], &a->velocity_km_s, &p[COL_VELOCITY]) &&
        le_real(campos[COL_MAGNITUDE], &a->absolute_magnitude, &p[COL_MAGNITUDE]) &&
        le_inteiro(campos[COL_DAYS], &a->days_until_approach, &p[COL_DAYS]) &&
        le_bool(campos[COL_PAST], &a->is_past_event, &p[COL_PAST]) &&
        le_bool(campos[COL_FUTURE], &a->is_future_event, &p[COL_FUTURE]) &&
        le_real(campos[COL_RISK], &a->risk_score, &p[COL_RISK]) &&
        le_inteiro(campos[COL_PANIC], &a->panic_level, &p[COL_PANIC]) &&
        le_texto(campos[COL_CATEGORY], a->threat_category, sizeof(a->threat_category), &p[COL_CATEGORY]) &&
        le_texto(campos[COL_VERDICT], a->panic_verdict, sizeof(a->panic_verdict), &p[COL_VERDICT]) &&
        le_bool(campos[COL_SENTRY], &a->on_sentry_list, &p[COL_SENTRY]) &&
        le_real(campos[COL_IMPACT_PROB], &a->sentry_impact_prob, &p[COL_IMPACT_PROB]) &&
        le_real(campos[COL_TORINO], &a->sentry_torino_scale, &p[COL_TORINO]) &&
        le_real(campos[COL_PALERMO], &a->sentry_palermo_scale, &p[COL_PALERMO]) &&
        le_real(campos[COL_DIAMETER], &a->sentry_diameter_km, &p[COL_DIAMETER])) {
        return a;
    }

    free(a);
    return NULL;
}

// Retorna a coluna com esse nome, ou -1 se não existir
static int procura_campo(const char *nome) {
    for (int i = 0; i < TOTAL_CAMPOS; i++) {
        if (strcmp(COLUNAS[i], nome) == 0) {
            return i;
        }
    }
    return -1;
}

// Getters usados pelo ITEM: 'ok' fica false se o campo não existir,
// estiver ausente ou não for do tipo pedido
static double nasa_get_numero(const void *dados, const char *campo, bool *ok) {
    const Asteroid *a = dados;
    int coluna = procura_campo(campo);

    *ok = coluna >= 0 && a->presente[coluna];
    if (!*ok) {
        return 0.0;
    }
    switch (coluna) {
        case COL_YEAR: return a->year;
        case COL_MONTH: return a->month;
        case COL_DISTANCE: return a->distance_au;
        case COL_VELOCITY: return a->velocity_km_s;
        case COL_MAGNITUDE: return a->absolute_magnitude;
        case COL_DAYS: return a->days_until_approach;
        case COL_RISK: return a->risk_score;
        case COL_PANIC: return a->panic_level;
        case COL_IMPACT_PROB: return a->sentry_impact_prob;
        case COL_TORINO: return a->sentry_torino_scale;
        case COL_PALERMO: return a->sentry_palermo_scale;
        case COL_DIAMETER: return a->sentry_diameter_km;
    }
    *ok = false; // o campo existe, mas não é numérico
    return 0.0;
}

static bool nasa_get_bool(const void *dados, const char *campo, bool *ok) {
    const Asteroid *a = dados;
    int coluna = procura_campo(campo);

    *ok = coluna >= 0 && a->presente[coluna];
    if (!*ok) {
        return false;
    }
    switch (coluna) {
        case COL_PAST: return a->is_past_event;
        case COL_FUTURE: return a->is_future_event;
        case COL_SENTRY: return a->on_sentry_list;
    }
    *ok = false;
    return false;
}

static const char *nasa_get_texto(const void *dados, const char *campo) {
    const Asteroid *a = dados;
    int coluna = procura_campo(campo);

    if (coluna < 0 || !a->presente[coluna]) {
        return NULL;
    }
    switch (coluna) {
        case COL_DESIGNATION: return a->asteroid_designation;
        case COL_FULLNAME: return a->asteroid_fullname;
        case COL_DATE: return a->close_approach_date;
        case COL_CATEGORY: return a->threat_category;
        case COL_VERDICT: return a->panic_verdict;
    }
    return NULL;
}

static void nasa_apagar(void *dados) {
    free(dados);
}

static const ITEM_OPS NASA_OPS = {
    nasa_get_numero,
    nasa_get_bool,
    nasa_get_texto,
    nasa_apagar
};

const LEITOR LEITOR_NASA = {
    "nasa",
    ',',
    COLUNAS,
    TOTAL_CAMPOS,
    nasa_converter,
    &NASA_OPS
};
