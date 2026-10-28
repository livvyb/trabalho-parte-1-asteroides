#ifndef LISTA_CRUZ_H
#define LISTA_CRUZ_H

#include "item.h"
#include "estrutura.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct ListaCruz ListaCruz;

ListaCruz *criar_lista_cruz(void);
bool lista_cruz_inserir(ListaCruz *lista, ITEM *item);
bool lista_cruz_configurar(ListaCruz *lista, const char *campo_linha, const char *campo_coluna);
bool lista_cruz_percorrer_por_coluna(const ListaCruz *lista, VisitanteItem visitante, void *contexto);
size_t lista_cruz_tamanho(const ListaCruz *lista);
bool lista_cruz_percorrer(const ListaCruz *lista, VisitanteItem visitante, void *contexto);
void lista_cruz_destruir(ListaCruz *lista);

#endif