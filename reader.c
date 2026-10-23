#include "reader.h"

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

/* Retira espaços do começo e do final. */
static char *cortar(char *texto) {
    char *fim;
    while (isspace((unsigned char)*texto)) {
        texto++;
    }
    if (*texto == '\0') {
        return texto;
    }
    fim = texto + strlen(texto) - 1;
    while (fim > texto && isspace((unsigned char)*fim)) {
        *fim = '\0';
        fim--;
    }
    return texto;
}

/* Converte texto para inteiro. */
static bool le_int(const char *texto, int *resultado) {
    char *fim;
    long valor;

    if (texto == NULL || *texto == '\0') {
        return false;
    }
    valor = strtol(texto, &fim, 10);
    while (isspace((unsigned char)*fim)) {
        fim++;
    }
    if (*fim != '\0') {
        return false;
    }
    *resultado = (int)valor;
    return true;
}

/* Converte texto para double. */
static bool le_double(const char *texto, double *resultado) {
    char *fim;
    double valor;
    if (texto == NULL || *texto == '\0') {
        return false;
    }
    valor = strtod(texto, &fim);
    while (isspace((unsigned char)*fim)) {
        fim++;
    }
    if (*fim != '\0') {
        return false;
    }
    *resultado = valor;
    return true;
}

/* Converte texto para booleano. */
static bool le_bool(const char *texto, bool *resultado) {
    if (strcmp(texto, "true") == 0 ||
        strcmp(texto, "True") == 0 ||
        strcmp(texto, "TRUE") == 0 ||
        strcmp(texto, "1") == 0) {
        *resultado = true;
        return true;
    }

    if (strcmp(texto, "false") == 0 ||
        strcmp(texto, "False") == 0 ||
        strcmp(texto, "FALSE") == 0 ||
        strcmp(texto, "0") == 0) {
        *resultado = false;
        return true;
    }

    return false;
}

/* Copia um texto sem ultrapassar o tamanho do destino. */
static void copia(char *destino, size_t tamanho,
                  const char *origem) {
    if (tamanho == 0) {
        return;
    }
    strncpy(destino, origem, tamanho - 1);
    destino[tamanho - 1] = '\0';
}

/* Separa uma linha CSV simples em campos.
   Esta versão inicial não trata vírgulas dentro de aspas. */
static int separa_csv(char *linha, char *campos[], int capacidade, char delimitador) {
    int quantidade = 0;
    char separador[2] = {delimitador, '\0'};
    char *token = strtok(linha, separador);

    while (token != NULL && quantidade < capacidade) {
        campos[quantidade++] = trim(token);
        token = strtok(NULL, separador);
    }

    return quantidade;
}

/* Procura uma coluna pelo nome no cabeçalho. */
static int procura_coluna(const Reader *reader, const char *nome) {
    for (int i = 0; i < reader->quantidade_colunas; i++) {
        if (strcmp(reader->nomes_colunas[i], nome) == 0) {
            return i;
        }
    }

    return -1;
}

/* Abre o CSV e identifica as colunas. */
bool reader_abre(Reader *reader, const char *caminho) {
    char linha[READER_MAX_LINHA];
    char *campos[READER_MAX_COLUNAS];
    if (reader == NULL || caminho == NULL) {
        return false;
    }
    memset(reader, 0, sizeof(*reader));
    reader->arquivo = fopen(caminho, "r");
    if (reader->arquivo == NULL) {
        return false;
    }
    if (fgets(linha, sizeof(linha), reader->arquivo) == NULL) {
        reader_fecha(reader);
        return false;
    }

    copia(reader->cabecalho, sizeof(reader->cabecalho), linha);

    if (strchr(linha, ';') != NULL &&
        strchr(linha, ',') == NULL) {
        reader->delimitador = ';';
    } else {
        reader->delimitador = ',';
    }

    linha[strcspn(linha, "\r\n")] = '\0';

    reader->quantidade_colunas = separa_csv(linha, campos, READER_MAX_COLUNAS, reader->delimitador);

    if (reader->quantidade_colunas <= 0) {
        reader_fecha(reader);
        return false;
    }

    for (int i = 0; i < reader->quantidade_colunas; i++) {
        copia(reader->nomes_colunas[i],
              sizeof(reader->nomes_colunas[i]), campos[i]);
    }

    for (int i = 0; i < TOTAL_CAMPOS; i++) {
        reader->indices[i] =
            procura_coluna(reader, campos_esperados[i]);

        /* magnitude e dados Sentry podem estar vazios
           em determinados registros, mas a coluna precisa
           existir no cabeçalho esperado. */
        if (reader->indices[i] == -1) {
            reader_fecha(reader);
            return false;
        }
    }

    return true;
}

/* Lê um registro e o converte para Asteroid. */
bool reader_proximo(Reader *reader, Asteroid *asteroid) {
    char linha[READER_MAX_LINHA];
    char copia_linha[READER_MAX_LINHA];
    char *campos[READER_MAX_COLUNAS];
    int quantidade;

    if (reader == NULL || reader->arquivo == NULL || asteroid == NULL) {
        return false;
    }

    while (fgets(linha, sizeof(linha), reader->arquivo) != NULL) {
        linha[strcspn(linha, "\r\n")] = '\0';

        if (linha[0] == '\0') {
            continue;
        }
        copia(copia_linha, sizeof(copia_linha), linha);
        quantidade = separa_csv( copia_linha, campos, READER_MAX_COLUNAS, reader->delimitador);
        if (quantidade != reader->quantidade_colunas) {
            return false;
        }

        memset(asteroid, 0, sizeof(*asteroid));

        copia(asteroid->asteroid_designation, sizeof(asteroid->asteroid_designation), campos[reader->indices[COL_DESIGNATION]]);
        copia(asteroid->asteroid_fullname, sizeof(asteroid->asteroid_fullname), campos[reader->indices[COL_FULLNAME]]);
        copia(asteroid->close_approach_date, sizeof(asteroid->close_approach_date), campos[reader->indices[COL_DATE]]);

        if (!le_int(campos[reader->indices[COL_YEAR]], &asteroid->year) ||
            !le_int(campos[reader->indices[COL_MONTH]], &asteroid->month) ||
            !le_double(campos[reader->indices[COL_DISTANCE]], &asteroid->distance_au) ||
            !le_double(campos[reader->indices[COL_VELOCITY]],  &asteroid->velocity_km_s) ||
            !le_int(campos[reader->indices[COL_DAYS]], &asteroid->days_until_approach) ||
            !le_double(campos[reader->indices[COL_RISK]], &asteroid->risk_score) ||
            !le_int(campos[reader->indices[COL_PANIC]], &asteroid->panic_level)) {
            return false;
        }

        asteroid->has_absolute_magnitude =
            le_double(campos[reader->indices[COL_MAGNITUDE]], &asteroid->absolute_magnitude);

        if (!le_bool(campos[reader->indices[COL_PAST]], &asteroid->is_past_event) ||
            !le_bool(campos[reader->indices[COL_FUTURE]], &asteroid->is_future_event) ||
            !le_bool(campos[reader->indices[COL_SENTRY]], &asteroid->on_sentry_list)) {
            return false;
        }

        copia(asteroid->threat_category, sizeof(asteroid->threat_category), campos[reader->indices[COL_CATEGORY]]);

        copia(asteroid->panic_verdict, sizeof(asteroid->panic_verdict), campos[reader->indices[COL_VERDICT]]);

        asteroid->has_sentry_data =
            le_double(campos[reader->indices[COL_IMPACT_PROB]], &asteroid->sentry_impact_prob) &&
            le_double(campos[reader->indices[COL_TORINO]], &asteroid->sentry_torino_scale) &&
            le_double(campos[reader->indices[COL_PALERMO]], &asteroid->sentry_palermo_scale) &&
            le_double(campos[reader->indices[COL_DIAMETER]], &asteroid->sentry_diameter_km);
        return true;
    }

    return false;
}

/* Fecha o arquivo aberto. */
void reader_fecha(Reader *reader) {
    if (reader != NULL && reader->arquivo != NULL) {
        fclose(reader->arquivo);
        reader->arquivo = NULL;
    }
}
