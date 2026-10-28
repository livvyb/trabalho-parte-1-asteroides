#include "lista_cabeca.h"
#include <stdlib.h>

typedef struct no_cabeca {
    ITEM *item;
    struct no_cabeca *prox;
} NoCabeca;

struct ListaCabeca {
    NoCabeca *sentinela; 
    NoCabeca *fim;
    size_t tamanho;
};

ListaCabeca *criar_lista_cabeca(void) {
    ListaCabeca *lista = malloc(sizeof(ListaCabeca));
    if (lista != NULL) {
        lista->sentinela = malloc(sizeof(NoCabeca));
        if (lista->sentinela != NULL) {
            lista->sentinela->item = NULL; 
            lista->sentinela->prox = NULL;
            lista->fim = lista->sentinela;
            lista->tamanho = 0;
        } else {
            free(lista);
            return NULL;
        }
    }
    return lista;
}

bool lista_cabeca_inserir(ListaCabeca *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    NoCabeca *novo = malloc(sizeof(NoCabeca));
    if (novo == NULL) return false;
    novo->item = item;
    novo->prox = NULL;
    lista->fim->prox = novo;
    lista->fim = novo;
    lista->tamanho++;

    return true;
}

size_t lista_cabeca_tamanho(const ListaCabeca *lista) {
    return lista != NULL ? lista->tamanho : 0;
}

bool lista_cabeca_percorrer(const ListaCabeca *lista, VisitanteItem visitante, void *contexto) {
    if (lista == NULL || visitante == NULL) return false;

    for (NoCabeca *atual = lista->sentinela->prox; atual != NULL; atual = atual->prox) {
        if (!visitante(atual->item, contexto)) {
            return false;
        }
    }
    return true;
}

void lista_cabeca_destruir(ListaCabeca *lista) {
    if (lista != NULL) {
        NoCabeca *atual = lista->sentinela;
        while (atual != NULL) {
            NoCabeca *prox = atual->prox;
            free(atual);
            atual = prox;
        }
        free(lista);
    }
}