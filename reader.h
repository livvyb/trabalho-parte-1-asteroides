#ifndef READER_H
#define READER_H

#include "item.h"
#include <stdbool.h>

/* Leitor genérico de CSV: abre o arquivo, confere o cabeçalho e separa os
   campos de cada linha. A conversão dos campos em ITEM fica a cargo do
   leitor específico do domínio (ex.: reader_nasa). */

#define READER_MAX_LINHA 8192
#define READER_MAX_COLUNAS 128

typedef struct reader_ READER;

// O que cada leitor específico precisa informar (ex.: LEITOR_NASA)
typedef struct {
    const char *nome;                  // nome usado em --reader
    char delimitador;                  // ',' ou ';'
    const char **colunas;              // cabeçalho esperado, na ordem
    int quantidade_colunas;
    void *(*converter)(char **campos); // devolve NULL se a linha for inválida
    const ITEM_OPS *ops;               // acesso aos campos dos dados convertidos
} LEITOR;

// Abre o arquivo com o leitor 'nome'. Retorna NULL em caso de erro
READER *reader_abrir(const char *nome, const char *caminho);

// Lê o próximo registro válido. Linhas inválidas são puladas com aviso
// no stderr. Retorna NULL no fim do arquivo
ITEM *reader_proximo(READER *reader);

// Cabeçalho e fim de linha ("\r\n" ou "\n") originais, para a saída
const char *reader_cabecalho(const READER *reader);
const char *reader_fim_de_linha(const READER *reader);

long reader_linhas_puladas(const READER *reader);

void reader_fechar(READER **reader);

#endif
