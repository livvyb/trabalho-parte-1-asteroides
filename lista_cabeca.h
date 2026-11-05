#ifndef LISTA_CABECA_H
#define LISTA_CABECA_H
#include "item.h"
#include "estrutura.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct ListaCabeca ListaCabeca;

ListaCabeca *criar_lista_cabeca(void);
bool lista_cabeca_inserir(ListaCabeca *lista, ITEM *item);
size_t lista_cabeca_tamanho(const ListaCabeca *lista);
bool lista_cabeca_percorrer(const ListaCabeca *lista, VisitanteItem visitante, void *contexto);
bool lista_cabeca_remover(ListaCabeca *lista, const ITEM *item);
const ITEM *lista_cabeca_buscar(const ListaCabeca *lista,
                                bool (*pred)(const ITEM *, void *), void *ctx);
void lista_cabeca_destruir(ListaCabeca *lista);

#endif