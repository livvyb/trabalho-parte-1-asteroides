#include "item.h"
#include <stdlib.h>
#include <string.h>

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

// Item sem o campo vai para o final: 'a' ausente -> depois; 'b' ausente -> antes
static int ausente_por_ultimo(bool tem_a, bool tem_b) {
    if (tem_a == tem_b) {
        return 0; // os dois sem o campo: empate
    }
    return tem_a ? -1 : 1;
}

int item_comparar(const ITEM *a, const ITEM *b, const char *campo, bool decrescente) {
    int resultado;
    bool tem_a, tem_b;
    double numero_a, numero_b;
    const char *texto_a, *texto_b;
    bool bool_a, bool_b;

    if (a == NULL || b == NULL || campo == NULL) {
        return 0;
    }

    if (strcmp(campo, "similarity") == 0) {
        numero_a = a->similaridade;
        numero_b = b->similaridade;
        resultado = (numero_a > numero_b) - (numero_a < numero_b);
        return decrescente ? -resultado : resultado;
    }

    // tenta como número, depois como texto e por fim como booleano
    numero_a = item_get_numero(a, campo, &tem_a);
    numero_b = item_get_numero(b, campo, &tem_b);
    if (tem_a || tem_b) {
        if (!tem_a || !tem_b) {
            return ausente_por_ultimo(tem_a, tem_b);
        }
        resultado = (numero_a > numero_b) - (numero_a < numero_b);
        return decrescente ? -resultado : resultado;
    }

    texto_a = item_get_texto(a, campo);
    texto_b = item_get_texto(b, campo);
    if (texto_a != NULL || texto_b != NULL) {
        if (texto_a == NULL || texto_b == NULL) {
            return ausente_por_ultimo(texto_a != NULL, texto_b != NULL);
        }
        resultado = strcmp(texto_a, texto_b);
        return decrescente ? -resultado : resultado;
    }

    bool_a = item_get_bool(a, campo, &tem_a);
    bool_b = item_get_bool(b, campo, &tem_b);
    if (!tem_a || !tem_b) {
        return ausente_por_ultimo(tem_a, tem_b); // também cobre campo inexistente
    }
    resultado = (int)bool_a - (int)bool_b; // false antes de true
    return decrescente ? -resultado : resultado;
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
