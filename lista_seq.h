
#ifndef LISTA_SEQ_H
#define LISTA_SEQ_H

#include "item.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct ListaSeq ListaSeq;

/* Função chamada para cada item durante a percorrida */
typedef bool (*ListaSeqVisitante)(const ITEM *item, void *contexto);

/* Cria uma lista vazia */
ListaSeq *criar_lista_seq(void);

/* Insere um item no final da lista */
bool lista_seq_inserir(ListaSeq *lista, ITEM *item);

/* Retorna o item na posição indicada */
ITEM *lista_seq_obter(const ListaSeq *lista, size_t posicao);

/* Retorna a quantidade de itens */
size_t lista_seq_tamanho(const ListaSeq *lista);

/* Percorre todos os itens da lista */
bool lista_seq_percorrer(
    const ListaSeq *lista,
    ListaSeqVisitante visitante,
    void *contexto
);

/* Libera a lista, sem liberar os itens armazenados */
void lista_seq_destruir(ListaSeq *lista);

#endif
