#include "reader_patrimonio.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Delimitador do CSV (o arquivo usa vírgula, com todos os campos entre aspas)
#define DELIMITADOR_PATRIMONIO ','

// Posição de cada coluna no CSV
enum {
    COL_BEM,
    COL_TIPO_PROTECAO,
    COL_PROCESSO,
    COL_MUNICIPIO,
    COL_CIDADES,
    COL_REGIAO,
    COL_RES_TOMBAMENTO,
    COL_TIPO_ENVOLTORIA,
    COL_RES_ENVOLTORIA,
    COL_PROC_VINCULADOS,
    COL_ANO,
    COL_LIVRO,
    COL_DOE,
    COL_OBSERVACOES,
    COL_ATA,
    TOTAL_CAMPOS
};

/* Cabeçalho do arquivo. A validação em reader.c ignora maiúsculas, espaços,
   pontuação e acentos, então espaços sobrando ("Ano Tombamento "), BOM ou
   diferenças de codificação não impedem a leitura. */
static const char *COLUNAS[TOTAL_CAMPOS] = {
    "Bens",
    "Tipo de proteção",
    "Nº do Processo",
    "Município",
    "Quais cidades?",
    "Região Administrativa",
    "Resolução de Tombamento",
    "Tipo de área envoltória",
    "Resolução de área envoltória",
    "Processos vinculados: regulamentação de área envoltória",
    "Ano Tombamento",
    "Livro do Tombo",
    "Publicação no DOE",
    "Observações",
    "Nº da ata e data da S.O."
};

/* Nomes usados nas consultas (FILTER, SIM, ORDER, GROUP, CROSS).
   São ASCII e sem espaços, para não depender de acentos nem de codificação. */
static const char *APELIDOS[TOTAL_CAMPOS] = {
    "bem",
    "tipo_protecao",
    "processo",
    "municipio",
    "cidades",
    "regiao_administrativa",
    "resolucao_tombamento",
    "tipo_area_envoltoria",
    "resolucao_area_envoltoria",
    "processos_vinculados",
    "ano_tombamento",
    "livro_tombo",
    "publicacao_doe",
    "observacoes",
    "ata_data_so"
};

// Registro de bem tombado, visível apenas neste arquivo.
// Textos são alocados com o tamanho exato (NULL = campo ausente).
typedef struct {
    char *texto[TOTAL_CAMPOS];
    int ano;
    bool ano_presente; // false se vazio ou não numérico (ex.: "1974/1985")
} Bem;

/* Copia o texto sem espaços nas pontas. Retorna NULL se ficar vazio
   (campo ausente) ou se faltar memória (*erro = true). */
static char *copia_aparada(const char *s, bool *erro) {
    const char *fim;
    char *copia;
    size_t n;

    while (isspace((unsigned char)*s)) {
        s++;
    }
    fim = s + strlen(s);
    while (fim > s && isspace((unsigned char)fim[-1])) {
        fim--;
    }
    n = (size_t)(fim - s);
    if (n == 0) {
        return NULL;
    }
    copia = malloc(n + 1);
    if (copia == NULL) {
        *erro = true;
        return NULL;
    }
    memcpy(copia, s, n);
    copia[n] = '\0';
    return copia;
}

static void patrimonio_apagar(void *dados) {
    Bem *b = dados;

    if (b != NULL) {
        for (int i = 0; i < TOTAL_CAMPOS; i++) {
            free(b->texto[i]);
        }
        free(b);
    }
}

/* Converte os campos de uma linha. Campo vazio ou ano inválido são tratados
   como ausentes e não descartam o registro; só a falta de memória falha. */
static void *patrimonio_converter(char **campos) {
    Bem *b = calloc(1, sizeof(Bem));
    bool erro = false;

    if (b == NULL) {
        return NULL;
    }
    for (int i = 0; i < TOTAL_CAMPOS && !erro; i++) {
        char *copia = copia_aparada(campos[i], &erro);

        if (i == COL_ANO) {
            if (copia != NULL) {
                char *fim;
                long v = strtol(copia, &fim, 10);
                if (fim != copia && *fim == '\0') {
                    b->ano = (int)v;
                    b->ano_presente = true;
                }
                free(copia);
            }
        } else {
            b->texto[i] = copia;
        }
    }
    if (erro) {
        patrimonio_apagar(b);
        return NULL;
    }
    return b;
}

// Retorna a coluna com esse apelido, ou -1 se não existir
static int procura_campo(const char *nome) {
    for (int i = 0; i < TOTAL_CAMPOS; i++) {
        if (APELIDOS[i][0] == nome[0] && strcmp(APELIDOS[i], nome) == 0) {
            return i;
        }
    }
    return -1;
}

// Único campo numérico: ano_tombamento
static double patrimonio_get_numero(const void *dados, const char *campo, bool *ok) {
    const Bem *b = dados;

    *ok = procura_campo(campo) == COL_ANO && b->ano_presente;
    return *ok ? b->ano : 0.0;
}

// Este dataset não tem campos booleanos
static bool patrimonio_get_bool(const void *dados, const char *campo, bool *ok) {
    (void)dados;
    (void)campo;
    *ok = false;
    return false;
}

static const char *patrimonio_get_texto(const void *dados, const char *campo) {
    const Bem *b = dados;
    int coluna = procura_campo(campo);

    if (coluna < 0 || coluna == COL_ANO) {
        return NULL;
    }
    return b->texto[coluna]; // NULL quando ausente
}

static const ITEM_OPS PATRIMONIO_OPS = {
    patrimonio_get_numero,
    patrimonio_get_bool,
    patrimonio_get_texto,
    patrimonio_apagar
};

const LEITOR LEITOR_PATRIMONIO = {
    "patrimonio",
    DELIMITADOR_PATRIMONIO,
    COLUNAS,
    TOTAL_CAMPOS,
    patrimonio_converter,
    &PATRIMONIO_OPS
};