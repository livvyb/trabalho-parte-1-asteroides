#ifndef READER_H
#define READER_H

#include "item.h"
#include <stdbool.h>
#include <stdio.h>

#define READER_MAX_COLUNAS 64
#define READER_MAX_LINHA 8192
#define READER_MAX_CAMPO 512

typedef struct {
    FILE *arquivo;
    char cabecalho[READER_MAX_LINHA];
    char delimitador;
    int quantidade_colunas;
    char nomes_colunas[READER_MAX_COLUNAS][READER_MAX_CAMPO];
    int indices[20];
} Reader;

/* Campos do Asteroid, na ordem usada por item.h. */
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

/* Nomes das colunas que o leitor espera encontrar. */
static const char *campos_esperados[TOTAL_CAMPOS] = {
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

/* Abre o CSV e identifica as colunas. */
bool reader_abre(Reader *reader, const char *caminho);

/* Lê um registro e o converte para Asteroid. */
bool reader_proximo(Reader *reader, Asteroid *asteroid);

/* Fecha o arquivo aberto. */
void reader_fecha(Reader *reader);


#endif
