#include "item.h"
#include <stdlib.h>

struct item_ {
    char *linha;           // linha original do CSV, sem o fim de linha (saída fiel)
    void *dados;           // registro já convertido pelo leitor (ex.: Asteroid)
    const ITEM_OPS *ops;   // como ler os campos de dados
    double similaridade;   // preenchida pela consulta
};

ITEM *item_criar(char *linha, void *dados, const ITEM_OPS *ops) {
    ITEM *item;

    if (linha == NULL || dados == NULL || ops == NULL) {
        return NULL;
    }

    item = malloc(sizeof(ITEM));
    if (item == NULL) {
        return NULL;
    }

    item->linha = linha;
    item->dados = dados;
    item->ops = ops;
    item->similaridade = 0.0;

    return item;
}

void item_apagar(ITEM **item) {
    if (item != NULL && *item != NULL) {
        // os dados do domínio são liberados por quem sabe como foram criados
        if ((*item)->ops->apagar != NULL) {
            (*item)->ops->apagar((*item)->dados);
        }
        free((*item)->linha);
        free(*item);
        *item = NULL;
    }
}

double item_get_numero(const ITEM *item, const char *campo, bool *ok) {
    if (item == NULL || campo == NULL) {
        *ok = false;
        return 0.0;
    }
    return item->ops->get_numero(item->dados, campo, ok);
}

bool item_get_bool(const ITEM *item, const char *campo, bool *ok) {
    if (item == NULL || campo == NULL) {
        *ok = false;
        return false;
    }
    return item->ops->get_bool(item->dados, campo, ok);
}

const char *item_get_texto(const ITEM *item, const char *campo) {
    if (item == NULL || campo == NULL) {
        return NULL;
    }
    return item->ops->get_texto(item->dados, campo);
}

void item_set_similaridade(ITEM *item, double similaridade) {
    if (item != NULL) {
        item->similaridade = similaridade;
    }
}

double item_get_similaridade(const ITEM *item) {
    return item != NULL ? item->similaridade : 0.0;
}

void item_escrever(const ITEM *item, const char *fim_de_linha, FILE *saida) {
    if (item != NULL && fim_de_linha != NULL && saida != NULL) {
        fprintf(saida, "%s%s", item->linha, fim_de_linha);
    }
}

void item_imprimir(const ITEM *item) {
    if (item != NULL) {
        printf("[%.4f] %s\n", item->similaridade, item->linha);
    }
}
