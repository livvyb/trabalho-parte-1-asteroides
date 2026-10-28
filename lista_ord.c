#include "lista_ord.h"
#include <stdlib.h>
#include <string.h>

#define CAMPO_MAX 64

typedef struct no_ord {
    ITEM *item;
    double chave;       // calculada uma única vez, na inserção
    bool tem_chave;     // false se o campo está ausente no item
    struct no_ord *prox;
} NoOrd;

struct ListaOrd {
    NoOrd *inicio;
    NoOrd *fim;
    size_t tamanho;
    char campo[CAMPO_MAX];  // chave do comando ORDER
    bool usa_similaridade;  // campo == "similarity" (não é coluna do CSV)
    bool desc;
};

ListaOrd *criar_lista_ord(void) {
    ListaOrd *lista = calloc(1, sizeof(ListaOrd));
    if (lista != NULL) {
        // padrão quando a query não tem ORDER
        strcpy(lista->campo, "similarity");
        lista->usa_similaridade = true;
        lista->desc = true;
    }
    return lista;
}

// Define a chave de ordenação (comando ORDER). Só vale com a lista vazia.
bool lista_ord_configurar(ListaOrd *lista, const char *campo, bool desc) {
    if (lista == NULL || campo == NULL || lista->tamanho > 0 || strlen(campo) >= CAMPO_MAX) {
        return false;
    }
    strcpy(lista->campo, campo);
    lista->usa_similaridade = strcmp(campo, "similarity") == 0;
    lista->desc = desc;
    return true;
}

// Itens sem a chave ficam sempre no final, em qualquer direção
static bool vem_antes(const ListaOrd *lista, const NoOrd *a, const NoOrd *b) {
    if (!a->tem_chave) return false;
    if (!b->tem_chave) return true;
    return lista->desc ? a->chave > b->chave : a->chave < b->chave;
}

bool lista_ord_inserir(ListaOrd *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    NoOrd *novo = malloc(sizeof(NoOrd));
    if (novo == NULL) return false;
    novo->item = item;
    novo->prox = NULL;
    if (lista->usa_similaridade) {
        novo->chave = item_get_similaridade(item);
        novo->tem_chave = true;
    } else {
        novo->chave = item_get_numero(item, lista->campo, &novo->tem_chave);
    }

    // A ordem é mantida durante a inserção (inserção ordenada estável)
    if (lista->inicio == NULL) {
        lista->inicio = lista->fim = novo;
    } else if (!vem_antes(lista, novo, lista->fim)) {
        // não vem antes do último: entra no final (caso rápido, O(1))
        lista->fim->prox = novo;
        lista->fim = novo;
    } else if (vem_antes(lista, novo, lista->inicio)) {
        novo->prox = lista->inicio;
        lista->inicio = novo;
    } else {
        NoOrd *atual = lista->inicio;
        while (!vem_antes(lista, novo, atual->prox)) {
            atual = atual->prox;
        }
        novo->prox = atual->prox;
        atual->prox = novo;
    }

    lista->tamanho++;
    return true;
}

size_t lista_ord_tamanho(const ListaOrd *lista) {
    return lista != NULL ? lista->tamanho : 0;
}

bool lista_ord_percorrer(const ListaOrd *lista, VisitanteItem visitante, void *contexto) {
    if (lista == NULL || visitante == NULL) return false;

    for (NoOrd *atual = lista->inicio; atual != NULL; atual = atual->prox) {
        if (!visitante(atual->item, contexto)) {
            return false;
        }
    }
    return true;
}

void lista_ord_destruir(ListaOrd *lista) {
    if (lista != NULL) {
        NoOrd *atual = lista->inicio;
        while (atual != NULL) {
            NoOrd *prox = atual->prox;
            free(atual); // o item pertence ao main
            atual = prox;
        }
        free(lista);
    }
}