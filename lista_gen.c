#include "lista_gen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMPO_MAX 64

typedef struct no_gen NoGen;
struct no_gen {
    bool eh_sublista;
    union {
        ITEM *item;                       // átomo
        struct {                          // sublista
            char *chave;
            NoGen *filhos;
            NoGen *ultimo;
        } sub;
    } u;
    NoGen *prox;
};

struct ListaGen {
    NoGen raiz;                           // sublista raiz, sem chave
    size_t tamanho;
    char campos[MAX_NIVEIS][CAMPO_MAX];   // um campo por nível do GROUP
    int niveis;
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

// Anexa 'no' ao fim dos filhos de 'sub'
static void anexa(NoGen *sub, NoGen *no) {
    if (sub->u.sub.ultimo != NULL) sub->u.sub.ultimo->prox = no;
    else sub->u.sub.filhos = no;
    sub->u.sub.ultimo = no;
}

ListaGen *criar_lista_gen(void) {
    ListaGen *lista = calloc(1, sizeof(ListaGen));
    if (lista != NULL) {
        lista->raiz.eh_sublista = true;   // sem GROUP: itens ficam direto na raiz
    }
    return lista;
}

// Define os campos do GROUP (1 a MAX_NIVEIS). Só vale com a lista vazia.
bool lista_gen_configurar(ListaGen *lista, const char *const *campos, int n) {
    if (lista == NULL || campos == NULL || lista->tamanho > 0 || n < 0 || n > MAX_NIVEIS) {
        return false;
    }
    for (int i = 0; i < n; i++) {
        if (campos[i] == NULL || strlen(campos[i]) >= CAMPO_MAX) return false;
        strcpy(lista->campos[i], campos[i]);
    }
    lista->niveis = n;
    return true;
}

bool lista_gen_inserir(ListaGen *lista, ITEM *item) {
    NoGen *atual, *folha;
    if (lista == NULL || item == NULL) return false;

    atual = &lista->raiz;
    for (int nivel = 0; nivel < lista->niveis; nivel++) {
        char buf[48];
        const char *chave = chave_do_item(item, lista->campos[nivel], buf, sizeof(buf));
        NoGen *f = atual->u.sub.filhos;

        while (f != NULL && !(f->eh_sublista && strcmp(f->u.sub.chave, chave) == 0)) {
            f = f->prox;
        }
        if (f == NULL) {                  // cria a sublista deste nível
            f = calloc(1, sizeof(NoGen));
            if (f == NULL) return false;
            f->eh_sublista = true;
            f->u.sub.chave = duplica(chave);
            if (f->u.sub.chave == NULL) { free(f); return false; }
            anexa(atual, f);
        }
        atual = f;
    }

    folha = calloc(1, sizeof(NoGen));
    if (folha == NULL) return false;
    folha->u.item = item;
    anexa(atual, folha);
    lista->tamanho++;
    return true;
}

size_t lista_gen_tamanho(const ListaGen *lista) {
    return lista != NULL ? lista->tamanho : 0;
}

static bool percorre_sub(const NoGen *sub, VisitanteItem visitante, void *contexto) {
    for (const NoGen *f = sub->u.sub.filhos; f != NULL; f = f->prox) {
        if (f->eh_sublista) {
            if (!percorre_sub(f, visitante, contexto)) return false;
        } else if (!visitante(f->u.item, contexto)) {
            return false;
        }
    }
    return true;
}

bool lista_gen_percorrer(const ListaGen *lista, VisitanteItem visitante, void *contexto) {
    if (lista == NULL || visitante == NULL) return false;
    return percorre_sub(&lista->raiz, visitante, contexto);
}

static void libera_filhos(NoGen *sub) {
    NoGen *f = sub->u.sub.filhos;
    while (f != NULL) {
        NoGen *prox = f->prox;
        if (f->eh_sublista) {
            libera_filhos(f);
            free(f->u.sub.chave);
        }                                 // o item pertence ao main
        free(f);
        f = prox;
    }
}

void lista_gen_destruir(ListaGen *lista) {
    if (lista == NULL) return;
    libera_filhos(&lista->raiz);
    free(lista);
}