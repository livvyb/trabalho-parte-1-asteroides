#ifndef LISTA_GEN_H
#define LISTA_GEN_H

#include "item.h"
#include "estrutura.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct ListaGen ListaGen;

ListaGen *criar_lista_gen(void);
bool lista_gen_configurar(ListaGen *lista, const char *campo_grupo, const char *campo_subgrupo);
bool lista_gen_inserir(ListaGen *lista, ITEM *item);
size_t lista_gen_tamanho(const ListaGen *lista);
bool lista_gen_percorrer(const ListaGen *lista, VisitanteItem visitante, void *contexto);
void lista_gen_destruir(ListaGen *lista);

#endif