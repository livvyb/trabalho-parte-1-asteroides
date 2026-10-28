#include "lista_enc.h"
#include <stdlib.h>

typedef struct no_lista {
    ITEM *item;
    struct no_lista *prox;
} NoLista;

struct ListaEnc {
    NoLista *inicio;
    NoLista *fim;
    size_t tamanho;
};

ListaEnc *criar_lista_enc(void) {
    ListaEnc *lista = malloc(sizeof(ListaEnc));
    if (lista != NULL) {
        lista->inicio = NULL;
        lista->fim = NULL;
        lista->tamanho = 0;
    }
    return lista;
}

bool lista_enc_inserir(ListaEnc *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    NoLista *novo = malloc(sizeof(NoLista));
    if (novo == NULL) return false;

    novo->item = item;
    novo->prox = NULL;

    if (lista->fim == NULL) {
        lista->inicio = novo;
    } else {
        lista->fim->prox = novo;
    }
    lista->fim = novo;
    lista->tamanho++;

    return true;
}

size_t lista_enc_tamanho(const ListaEnc *lista) {
    return lista != NULL ? lista->tamanho : 0;
}

bool lista_enc_percorrer(const ListaEnc *lista, VisitanteItem visitante, void *contexto) {
    if (lista == NULL || visitante == NULL) return false;

    for (NoLista *atual = lista->inicio; atual != NULL; atual = atual->prox) {
        if (!visitante(atual->item, contexto)) {
            return false;
        }
    }
    return true;
}

void lista_enc_destruir(ListaEnc *lista) {
    if (lista != NULL) {
        NoLista *atual = lista->inicio;
        while (atual != NULL) {
            NoLista *prox = atual->prox;
            // Seguindo o modelo da sua lista_seq.c, nós apenas liberamos os nós estruturais. 
            // A liberação dos 'ITEM*' (item_apagar) deve ser feita pela estrutura 'base' ou no main 
            // para evitar double free, já que 'base' e 'result' compartilham os mesmos itens.
            free(atual);
            atual = prox;
        }
        free(lista);
    }
}