#include "lista_gen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMPO_MAX 64

typedef struct no_item {
    ITEM *item;
    struct no_item *prox;
} NoItem;

typedef struct no_subgrupo {
    char *chave;
    NoItem *inicio;
    NoItem *fim;
    struct no_subgrupo *prox;
} NoSubgrupo;

typedef struct no_grupo {
    char *chave;
    NoSubgrupo *subgrupos;
    NoSubgrupo *ultimo_sub;
    struct no_grupo *prox;
} NoGrupo;

struct ListaGen {
    NoGrupo *grupos;
    NoGrupo *ultimo_grupo;
    size_t tamanho;
    char campo_grupo[CAMPO_MAX];     // 1º campo do GROUP
    char campo_subgrupo[CAMPO_MAX];  // 2º campo do GROUP
};

static char *duplica(const char *s) {
    size_t n = strlen(s) + 1;
    char *c = malloc(n);
    if (c != NULL) memcpy(c, s, n);
    return c;
}

// Valor do campo como texto (texto, número ou booleano). 'buf' guarda a conversão.
static const char *chave_do_item(const ITEM *item, const char *campo, char *buf, size_t n) {
    bool ok;
    const char *t = item_get_texto(item, campo);
    if (t != NULL && t[0] != '\0') return t;
    double v = item_get_numero(item, campo, &ok);
    if (ok) {
        snprintf(buf, n, "%.15g", v);
        return buf;
    }
    bool b = item_get_bool(item, campo, &ok);
    if (ok) return b ? "true" : "false";
    return "N/A";
}

ListaGen *criar_lista_gen(void) {
    ListaGen *lista = calloc(1, sizeof(ListaGen));
    if (lista != NULL) {
        strcpy(lista->campo_grupo, "threat_category");
        strcpy(lista->campo_subgrupo, "year");
    }
    return lista;
}

// Define os campos do comando GROUP. Só vale com a lista vazia.
bool lista_gen_configurar(ListaGen *lista, const char *campo_grupo, const char *campo_subgrupo) {
    if (lista == NULL || campo_grupo == NULL || campo_subgrupo == NULL || lista->tamanho > 0 ||
        strlen(campo_grupo) >= CAMPO_MAX || strlen(campo_subgrupo) >= CAMPO_MAX) {
        return false;
    }
    strcpy(lista->campo_grupo, campo_grupo);
    strcpy(lista->campo_subgrupo, campo_subgrupo);
    return true;
}

bool lista_gen_inserir(ListaGen *lista, ITEM *item) {
    char buf1[48], buf2[48];
    if (!lista || !item) return false;

    const char *chave_g = chave_do_item(item, lista->campo_grupo, buf1, sizeof(buf1));
    const char *chave_s = chave_do_item(item, lista->campo_subgrupo, buf2, sizeof(buf2));

    // Grupo (nível 1): busca ou cria
    NoGrupo *g = lista->grupos;
    while (g && strcmp(g->chave, chave_g) != 0) g = g->prox;
    if (!g) {
        g = calloc(1, sizeof(NoGrupo));
        if (!g) return false;
        g->chave = duplica(chave_g);
        if (!g->chave) { free(g); return false; }
        if (lista->ultimo_grupo) lista->ultimo_grupo->prox = g; else lista->grupos = g;
        lista->ultimo_grupo = g;
    }

    // Subgrupo (nível 2): busca ou cria
    NoSubgrupo *s = g->subgrupos;
    while (s && strcmp(s->chave, chave_s) != 0) s = s->prox;
    if (!s) {
        s = calloc(1, sizeof(NoSubgrupo));
        if (!s) return false;
        s->chave = duplica(chave_s);
        if (!s->chave) { free(s); return false; }
        if (g->ultimo_sub) g->ultimo_sub->prox = s; else g->subgrupos = s;
        g->ultimo_sub = s;
    }

    // Item no fim do subgrupo (mantém a ordem de chegada)
    NoItem *ni = malloc(sizeof(NoItem));
    if (!ni) return false;
    ni->item = item;
    ni->prox = NULL;
    if (s->fim) s->fim->prox = ni; else s->inicio = ni;
    s->fim = ni;

    lista->tamanho++;
    return true;
}

size_t lista_gen_tamanho(const ListaGen *lista) {
    return lista ? lista->tamanho : 0;
}

bool lista_gen_percorrer(const ListaGen *lista, VisitanteItem visitante, void *contexto) {
    if (!lista || !visitante) return false;
    for (NoGrupo *g = lista->grupos; g; g = g->prox) {
        for (NoSubgrupo *s = g->subgrupos; s; s = s->prox) {
            for (NoItem *ni = s->inicio; ni; ni = ni->prox) {
                if (!visitante(ni->item, contexto)) return false;
            }
        }
    }
    return true;
}

void lista_gen_destruir(ListaGen *lista) {
    if (!lista) return;
    NoGrupo *g = lista->grupos;
    while (g) {
        NoSubgrupo *s = g->subgrupos;
        while (s) {
            NoItem *ni = s->inicio;
            while (ni) {
                NoItem *prox_ni = ni->prox;
                free(ni);
                ni = prox_ni;
            }
            NoSubgrupo *prox_s = s->prox;
            free(s->chave);
            free(s);
            s = prox_s;
        }
        NoGrupo *prox_g = g->prox;
        free(g->chave);
        free(g);
        g = prox_g;
    }
    free(lista);
}