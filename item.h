#ifndef ITEM_H
#define ITEM_H

#include <stdbool.h>
#include <stdio.h>

/* TAD genérico: representa um registro (uma linha do CSV) sem conhecer o
   domínio. Os campos são acessados pelo nome, usando as funções fornecidas
   pelo leitor específico (ex.: reader_nasa). */
typedef struct item_ ITEM;

// Funções que cada domínio implementa sobre os seus dados.
// 'ok' indica se o campo existe, é do tipo pedido e não está ausente
typedef struct {
    double (*get_numero)(const void *dados, const char *campo, bool *ok);
    bool (*get_bool)(const void *dados, const char *campo, bool *ok);
    const char *(*get_texto)(const void *dados, const char *campo); // NULL se ausente
    void (*apagar)(void *dados);
} ITEM_OPS;

// Cria o item, que passa a ser dono de 'linha' e 'dados' (alocados com malloc)
ITEM *item_criar(char *linha, void *dados, const ITEM_OPS *ops);

// Libera o item, a linha original e os dados do domínio
void item_apagar(ITEM **item);

// Acesso aos campos pelo nome da coluna
double item_get_numero(const ITEM *item, const char *campo, bool *ok);
bool item_get_bool(const ITEM *item, const char *campo, bool *ok);
const char *item_get_texto(const ITEM *item, const char *campo);

// Similaridade calculada pela consulta (não vai para o arquivo de saída)
void item_set_similaridade(ITEM *item, double similaridade);
double item_get_similaridade(const ITEM *item);

// Escreve a linha original seguida do fim de linha da entrada (reader_fim_de_linha)
void item_escrever(const ITEM *item, const char *fim_de_linha, FILE *saida);

// Imprime o item na saída padrão (depuração)
void item_imprimir(const ITEM *item);

#endif
