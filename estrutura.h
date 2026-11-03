
#ifndef ESTRUTURA_H
#define ESTRUTURA_H
#include "query.h"
#include "item.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    ESTRUTURA_PILHA,
    ESTRUTURA_FILA,
    ESTRUTURA_LISTA_SEQ,
    ESTRUTURA_LISTA_ENC,
    ESTRUTURA_LISTA_CABECA,
    ESTRUTURA_LISTA_ORD,
    ESTRUTURA_LISTA_GEN,
    ESTRUTURA_LISTA_CRUZ
} TipoEstrutura;

typedef struct Estrutura Estrutura;

Estrutura *estrutura_criar(TipoEstrutura tipo, const Query *query);

bool estrutura_inserir(Estrutura *estrutura, ITEM *item);

size_t estrutura_tamanho(const Estrutura *estrutura);

bool estrutura_percorrer(const Estrutura *estrutura, VisitanteItem visitante, void *contexto);

bool estrutura_percorrer_alt(const Estrutura *estrutura, VisitanteItem visitante, void *contexto);

void estrutura_destruir(Estrutura *estrutura);

#endif
