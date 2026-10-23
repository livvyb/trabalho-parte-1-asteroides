#ifndef LISTA_H
#define LISTA_H

#include <stdbool.h>
#include "item.h"

typedef struct lista_ LISTA;

LISTA *lista_criar(bool ordenada);
void lista_apagar(LISTA **lista);
bool lista_inserir(LISTA *lista, ITEM *item);
ITEM *lista_busca(const LISTA *lista, int chave);
ITEM *lista_remover(LISTA *lista, int chave);
int lista_tamanho(const LISTA *lista);
bool lista_vazia(const LISTA *lista);
bool lista_cheia(const LISTA *lista);
void lista_imprimir(const LISTA *lista);

#endif