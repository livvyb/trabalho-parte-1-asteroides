#include "fila.h"
#include <stdlib.h>

typedef struct no_fila {
    ITEM *item;
    struct no_fila *prox;
} NoFila;

struct fila {
    NoFila *inicio;
    NoFila *fim;
    size_t tamanho;
};

FILA *fila_criar(void) {
    FILA *f = malloc(sizeof(FILA));
    if (f != NULL) {
        f->inicio = NULL;
        f->fim = NULL;
        f->tamanho = 0;
    }
    return f;
}

bool fila_inserir(FILA *f, ITEM *item) {
    if (f == NULL || item == NULL) return false;
    
    NoFila *novo = malloc(sizeof(NoFila));
    if (novo == NULL) return false;
    
    novo->item = item;
    novo->prox = NULL;
    
    if (f->fim == NULL) {
        f->inicio = novo;
    } else {
        f->fim->prox = novo;
    }
    f->fim = novo;
    f->tamanho++;
    
    return true;
}

size_t fila_tamanho(const FILA *f) {
    return f != NULL ? f->tamanho : 0;
}

bool fila_percorrer(const FILA *f, VisitanteItem visitante, void *contexto) {
    if (f == NULL || visitante == NULL) return false;
    
    for (NoFila *atual = f->inicio; atual != NULL; atual = atual->prox) {
        if (!visitante(atual->item, contexto)) {
            return false;
        }
    }
    return true;
}

void fila_destruir(FILA **f) {
    if (f != NULL && *f != NULL) {
        NoFila *atual = (*f)->inicio;
        while (atual != NULL) {
            NoFila *prox = atual->prox;
            item_apagar(&(atual->item));
            free(atual);
            atual = prox;
        }
        free(*f);
        *f = NULL;
    }
}