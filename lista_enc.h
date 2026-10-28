#ifndef LISTA_ENC_H
#define LISTA_ENC_H

#include "item.h"
#include "estrutura.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct ListaEnc ListaEnc;

ListaEnc *criar_lista_enc(void);
bool lista_enc_inserir(ListaEnc *lista, ITEM *item);
size_t lista_enc_tamanho(const ListaEnc *lista);
bool lista_enc_percorrer(const ListaEnc *lista, VisitanteItem visitante, void *contexto);
void lista_enc_destruir(ListaEnc *lista);

#endif