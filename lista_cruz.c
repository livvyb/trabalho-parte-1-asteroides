#include "lista_cruz.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMPO_MAX 64

// Cada item tem UM único nó, que participa das duas organizações ao mesmo tempo
typedef struct no_cruzado {
    ITEM *item;
    struct no_cruzado *prox_linha;  // próximo item com a mesma chave de linha
    struct no_cruzado *prox_coluna; // próximo item com a mesma chave de coluna
} NoCruzado;

typedef struct descritor {
    char *chave;
    NoCruzado *inicio;
    NoCruzado *fim;
    struct descritor *prox;
} Descritor;

struct ListaCruz {
    Descritor *linhas;   // organização 1 (1º campo do CROSS)
    Descritor *ultima_linha;
    Descritor *colunas;  // organização 2 (2º campo do CROSS)
    Descritor *ultima_coluna;
    size_t tamanho;
    char campo_linha[CAMPO_MAX];
    char campo_coluna[CAMPO_MAX];
};

static char *duplica(const char *s) {
    size_t n = strlen(s) + 1;
    char *c = malloc(n);
    if (c != NULL) memcpy(c, s, n);
    return c;
}

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

// Busca o descritor com essa chave; cria no fim da lista se não existir
static Descritor *acha_ou_cria(Descritor **inicio, Descritor **ultimo, const char *chave) {
    Descritor *d = *inicio;
    while (d && strcmp(d->chave, chave) != 0) d = d->prox;
    if (d) return d;

    d = calloc(1, sizeof(Descritor));
    if (!d) return NULL;
    d->chave = duplica(chave);
    if (!d->chave) { free(d); return NULL; }
    if (*ultimo) (*ultimo)->prox = d; else *inicio = d;
    *ultimo = d;
    return d;
}

ListaCruz *criar_lista_cruz(void) {
    ListaCruz *lista = calloc(1, sizeof(ListaCruz));
    if (lista != NULL) {
        // sem CROSS na query, todos os itens ficam numa única linha/coluna ("N/A")
        lista->campo_linha[0] = '\0';
        lista->campo_coluna[0] = '\0';
    }
    return lista;
}

// Define os campos do comando CROSS. Só vale com a lista vazia.
bool lista_cruz_configurar(ListaCruz *lista, const char *campo_linha, const char *campo_coluna) {
    if (lista == NULL || campo_linha == NULL || campo_coluna == NULL || lista->tamanho > 0 ||
        strlen(campo_linha) >= CAMPO_MAX || strlen(campo_coluna) >= CAMPO_MAX) {
        return false;
    }
    strcpy(lista->campo_linha, campo_linha);
    strcpy(lista->campo_coluna, campo_coluna);
    return true;
}

bool lista_cruz_inserir(ListaCruz *lista, ITEM *item) {
    char buf1[48], buf2[48];
    if (!lista || !item) return false;

    Descritor *l = acha_ou_cria(&lista->linhas, &lista->ultima_linha,
                                chave_do_item(item, lista->campo_linha, buf1, sizeof(buf1)));
    if (!l) return false;
    Descritor *c = acha_ou_cria(&lista->colunas, &lista->ultima_coluna,
                                chave_do_item(item, lista->campo_coluna, buf2, sizeof(buf2)));
    if (!c) return false;

    NoCruzado *novo = malloc(sizeof(NoCruzado));
    if (!novo) return false;
    novo->item = item;
    novo->prox_linha = NULL;
    novo->prox_coluna = NULL;

    // mesmo nó encadeado nas duas organizações, sem duplicar o registro
    if (l->fim) l->fim->prox_linha = novo; else l->inicio = novo;
    l->fim = novo;
    if (c->fim) c->fim->prox_coluna = novo; else c->inicio = novo;
    c->fim = novo;

    lista->tamanho++;
    return true;
}

size_t lista_cruz_tamanho(const ListaCruz *lista) {
    return lista ? lista->tamanho : 0;
}

// Percurso pela 1ª organização (linhas, ex.: threat_category). É o usado pela Estrutura.
bool lista_cruz_percorrer(const ListaCruz *lista, VisitanteItem visitante, void *contexto) {
    if (!lista || !visitante) return false;
    for (Descritor *l = lista->linhas; l; l = l->prox) {
        for (NoCruzado *n = l->inicio; n; n = n->prox_linha) {
            if (!visitante(n->item, contexto)) return false;
        }
    }
    return true;
}

// Percurso pela 2ª organização (colunas, ex.: year), sobre os mesmos nós
bool lista_cruz_percorrer_por_coluna(const ListaCruz *lista, VisitanteItem visitante, void *contexto) {
    if (!lista || !visitante) return false;
    for (Descritor *c = lista->colunas; c; c = c->prox) {
        for (NoCruzado *n = c->inicio; n; n = n->prox_coluna) {
            if (!visitante(n->item, contexto)) return false;
        }
    }
    return true;
}

void lista_cruz_destruir(ListaCruz *lista) {
    if (!lista) return;

    // Os nós são liberados por uma só dimensão para evitar double free
    Descritor *l = lista->linhas;
    while (l) {
        NoCruzado *n = l->inicio;
        while (n) {
            NoCruzado *prox_n = n->prox_linha;
            free(n);
            n = prox_n;
        }
        Descritor *prox_l = l->prox;
        free(l->chave);
        free(l);
        l = prox_l;
    }
    Descritor *c = lista->colunas;
    while (c) {
        Descritor *prox_c = c->prox;
        free(c->chave);
        free(c);
        c = prox_c;
    }
    free(lista);
}