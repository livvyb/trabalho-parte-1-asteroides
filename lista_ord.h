#ifndef LISTA_ORD_H
#define LISTA_ORD_H

#include "item.h"
#include "estrutura.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct ListaOrd ListaOrd;

ListaOrd *criar_lista_ord(void);
bool lista_ord_configurar(ListaOrd *lista, const char *campo, bool desc);
bool lista_ord_inserir(ListaOrd *lista, ITEM *item);
size_t lista_ord_tamanho(const ListaOrd *lista);
bool lista_ord_percorrer(const ListaOrd *lista, VisitanteItem visitante, void *contexto);
void lista_ord_destruir(ListaOrd *lista);

#endif