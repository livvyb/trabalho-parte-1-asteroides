#ifndef FILA_H
#define FILA_H

#include "item.h"
#include "estrutura.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct fila FILA;

FILA *fila_criar(void);
bool fila_inserir(FILA *f, ITEM *item);
size_t fila_tamanho(const FILA *f);
bool fila_percorrer(const FILA *f, VisitanteItem visitante, void *contexto);
void fila_destruir(FILA **f);

#endif