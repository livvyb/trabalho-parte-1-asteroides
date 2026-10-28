#include "pilha.h"
#include <stdlib.h>

typedef struct no_pilha {
    ITEM *item;
    struct no_pilha *prox;
} NoPilha;

struct pilha {
    NoPilha *topo;
    size_t tamanho;
};

PILHA *pilha_criar(void) {
    PILHA *p = malloc(sizeof(PILHA));
    if (p != NULL) {
        p->topo = NULL;
        p->tamanho = 0;
    }
    return p;
}

bool pilha_inserir(PILHA *p, ITEM *item) {
    if (p == NULL || item == NULL) return false;
    
    NoPilha *novo = malloc(sizeof(NoPilha));
    if (novo == NULL) return false;
    
    novo->item = item;
    novo->prox = p->topo;
    p->topo = novo;
    p->tamanho++;
    
    return true;
}

size_t pilha_tamanho(const PILHA *p) {
    return p != NULL ? p->tamanho : 0;
}

bool pilha_percorrer(const PILHA *p, VisitanteItem visitante, void *contexto) {
    if (p == NULL || visitante == NULL) return false;
    
    for (NoPilha *atual = p->topo; atual != NULL; atual = atual->prox) {
        if (!visitante(atual->item, contexto)) {
            return false;
        }
    }
    return true;
}

void pilha_destruir(PILHA **p) {
    if (p != NULL && *p != NULL) {
        NoPilha *atual = (*p)->topo;
        while (atual != NULL) {
            NoPilha *prox = atual->prox;
            item_apagar(&(atual->item));
            free(atual);
            atual = prox;
        }
        free(*p);
        *p = NULL;
    }
}