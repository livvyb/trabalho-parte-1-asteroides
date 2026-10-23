#include "lista.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct no_ NO;

struct no_ {
    ITEM *item;
    NO *proximo;
};

struct lista_ {
    NO *inicio;
    NO *fim;
    int tamanho;
    bool ordenada;
};

static NO *no_criar(ITEM *item) {
    NO *novo = malloc(sizeof(NO));
    if (novo != NULL) {
        novo->item = item;
        novo->proximo = NULL;
    }
    return novo;
}

static bool inserir_fim(LISTA *lista, ITEM *item) {
    NO *novo = no_criar(item);
    if (novo == NULL)
        return false;

    if (lista->inicio == NULL)
        lista->inicio = novo;
    else
        lista->fim->proximo = novo;

    lista->fim = novo;
    lista->tamanho++;
    return true;
}

static bool inserir_ordenado(LISTA *lista, ITEM *item) {
    NO *novo = no_criar(item);
    if (novo == NULL)
        return false;

    int chave = item_chave(item);
    NO *anterior = NULL;
    NO *atual = lista->inicio;

    while (atual != NULL && item_chave(atual->item) < chave) {
        anterior = atual;
        atual = atual->proximo;
    }

    novo->proximo = atual;

    if (anterior == NULL)
        lista->inicio = novo;
    else
        anterior->proximo = novo;

    if (atual == NULL)
        lista->fim = novo;

    if (lista->tamanho == 0)
        lista->fim = novo;

    lista->tamanho++;
    return true;
}

LISTA *lista_criar(bool ordenada) {
    LISTA *lista = malloc(sizeof(LISTA));
    if (lista == NULL)
        return NULL;

    lista->inicio = NULL;
    lista->fim = NULL;
    lista->tamanho = 0;
    lista->ordenada = ordenada;
    return lista;
}

void lista_apagar(LISTA **lista) {
    if (lista == NULL || *lista == NULL)
        return;

    NO *p = (*lista)->inicio;
    while (p != NULL) {
        NO *proximo = p->proximo;
        item_apagar(&p->item);
        free(p);
        p = proximo;
    }

    free(*lista);
    *lista = NULL;
}

bool lista_inserir(LISTA *lista, ITEM *item) {
    if (lista == NULL || item == NULL)
        return false;

    return lista->ordenada
        ? inserir_ordenado(lista, item)
        : inserir_fim(lista, item);
}

ITEM *lista_busca(const LISTA *lista, int chave) {
    if (lista == NULL)
        return NULL;

    const NO *p = lista->inicio;
    while (p != NULL) {
        int atual = item_chave(p->item);
        if (atual == chave)
            return p->item;
        if (lista->ordenada && atual > chave)
            return NULL;
        p = p->proximo;
    }
    return NULL;
}

ITEM *lista_remover(LISTA *lista, int chave) {
    if (lista == NULL || lista->inicio == NULL)
        return NULL;

    NO *anterior = NULL;
    NO *atual = lista->inicio;

    while (atual != NULL && item_chave(atual->item) != chave) {
        if (lista->ordenada && item_chave(atual->item) > chave)
            return NULL;
        anterior = atual;
        atual = atual->proximo;
    }

    if (atual == NULL)
        return NULL;

    if (anterior == NULL)
        lista->inicio = atual->proximo;
    else
        anterior->proximo = atual->proximo;

    if (atual == lista->fim)
        lista->fim = anterior;

    ITEM *item = atual->item;
    atual->proximo = NULL;
    free(atual);
    lista->tamanho--;

    if (lista->tamanho == 0)
        lista->fim = NULL;

    return item;
}

int lista_tamanho(const LISTA *lista) {
    return lista != NULL ? lista->tamanho : 0;
}

bool lista_vazia(const LISTA *lista) {
    return lista != NULL && lista->inicio == NULL;
}

bool lista_cheia(const LISTA *lista) {
    (void)lista;
    return false;
}

void lista_imprimir(const LISTA *lista) {
    if (lista == NULL) {
        printf("<lista inexistente>\n");
        return;
    }

    printf("inicio -> ");
    for (const NO *p = lista->inicio; p != NULL; p = p->proximo) {
        item_imprimir(p->item);
        printf(" -> ");
    }
    printf("NULL\n");
}