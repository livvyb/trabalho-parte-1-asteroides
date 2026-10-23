#include "lista_seq.h"

#include <stdlib.h>
#include <stdint.h>

#define CAPACIDADE_INICIAL 10

struct ListaSeq {
    ITEM **itens;
    size_t tamanho;
    size_t capacidade;
};

ListaSeq *criar_lista_seq(void)
{
    ListaSeq *lista = malloc(sizeof(ListaSeq));
    if (lista == NULL) {
        return NULL;
    }

    lista->itens = malloc(CAPACIDADE_INICIAL * sizeof(ITEM *));

    if (lista->itens == NULL) {
        free(lista);
        return NULL;
    }
    lista->tamanho = 0;
    lista->capacidade = CAPACIDADE_INICIAL;

    return lista;
}

bool lista_seq_inserir(ListaSeq *lista, ITEM *item)
{
    if (lista == NULL || item == NULL) {
        return false;
    }
    /* Se o vetor estiver cheio, aumenta sua capacidade */
    if (lista->tamanho == lista->capacidade) {
        if (lista->capacidade > SIZE_MAX / 2) {
            return false;
        }
        size_t nova_capacidade = lista->capacidade * 2;

        if (nova_capacidade > SIZE_MAX / sizeof(ITEM *)) {
            return false;
        }
        ITEM **novo_vetor = realloc(
            lista->itens,
            nova_capacidade * sizeof(ITEM *)
        );
        if (novo_vetor == NULL) {
            return false;
        }
        lista->itens = novo_vetor;
        lista->capacidade = nova_capacidade;
    }
    lista->itens[lista->tamanho] = item;
    lista->tamanho++;
    return true;
}

ITEM *lista_seq_obter(const ListaSeq *lista, size_t posicao)
{
    if (lista == NULL || posicao >= lista->tamanho) {
        return NULL;
    }
    return lista->itens[posicao];
}

size_t lista_seq_tamanho(const ListaSeq *lista)
{
    if (lista == NULL) {
        return 0;
    }
    return lista->tamanho;
}

bool lista_seq_percorrer(
    const ListaSeq *lista,
    ListaSeqVisitante visitante,
    void *contexto
)
{
    if (lista == NULL || visitante == NULL) {
        return false;
    }
    for (size_t i = 0; i < lista->tamanho; i++) {
        if (!visitante(lista->itens[i], contexto)) {
            return false;
        }
    }
    return true;
}

void lista_seq_destruir(ListaSeq *lista)
{
    if (lista == NULL) {
        return;
    }
    free(lista->itens);
    free(lista);
}
