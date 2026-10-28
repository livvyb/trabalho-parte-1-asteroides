#ifndef PILHA_H
#define PILHA_H

#include "item.h"
#include "estrutura.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct pilha PILHA;

PILHA *pilha_criar(void);
bool pilha_inserir(PILHA *p, ITEM *item);
size_t pilha_tamanho(const PILHA *p);
bool pilha_percorrer(const PILHA *p, VisitanteItem visitante, void *contexto);
void pilha_destruir(PILHA **p);

#endif