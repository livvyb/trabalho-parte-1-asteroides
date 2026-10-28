#ifndef LISTA_SEQ_H
#define LISTA_SEQ_H

#include "item.h"
#include "estrutura.h" // para o tipo VisitanteItem
#include <stdbool.h>
#include <stddef.h>

typedef struct ListaSeq ListaSeq;

ListaSeq *criar_lista_seq(void);
bool lista_seq_inserir(ListaSeq *lista, ITEM *item);
ITEM *lista_seq_obter(const ListaSeq *lista, size_t posicao);
size_t lista_seq_tamanho(const ListaSeq *lista);
bool lista_seq_percorrer(const ListaSeq *lista, VisitanteItem visitante, void *contexto);
void lista_seq_destruir(ListaSeq *lista);

#endif