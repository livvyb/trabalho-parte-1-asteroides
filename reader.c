#include "reader.h"
#include "reader_nasa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct reader_ {
    FILE *arquivo;
    const LEITOR *leitor;          // leitor escolhido em --reader
    const char *fim_de_linha;      // "\r\n" ou "\n", igual ao da entrada
    long numero_linha;             // usado nos avisos (cabeçalho = linha 1)
    long linhas_puladas;
    char cabecalho[READER_MAX_LINHA];
    char linha[READER_MAX_LINHA];  // linha original, que vai para o item
    char campos[READER_MAX_LINHA]; // cópia da linha, separada em campos
};

// Leitores disponíveis em --reader (na Parte 2, basta acrescentar o novo aqui)
static const LEITOR *LEITORES[] = { &LEITOR_NASA };

/* Separa o texto em campos trocando o delimitador por '\0'. Entre aspas o
   delimitador é ignorado (ex.: "Worth a tweet, not worth a bunker.") e as
   aspas são removidas. Campos vazios são mantidos.
   Retorna a quantidade de campos, ou -1 se as aspas não fecharem. */
static int separa_campos(char *texto, char delimitador, char **campos, int maximo) {
    char *escrita = texto; // nunca passa da leitura, então dá para reaproveitar o texto
    bool entre_aspas = false;
    int quantidade = 0;

    campos[quantidade++] = texto;
    for (char *leitura = texto; *leitura != '\0'; leitura++) {
        if (*leitura == '"') {
            entre_aspas = !entre_aspas;
        } else if (*leitura == delimitador && !entre_aspas) {
            *escrita++ = '\0'; // fecha o campo atual
            if (quantidade == maximo) {
                return -1;
            }
            campos[quantidade++] = escrita;
        } else {
            *escrita++ = *leitura;
        }
    }
    *escrita = '\0';

    return entre_aspas ? -1 : quantidade;
}

// Lê uma linha e remove o "\r\n" (ou "\n"). Retorna false no fim do arquivo.
// Se 'tinha_cr' não for NULL, informa se a linha terminava com '\r'
static bool le_linha(FILE *arquivo, char *destino, bool *tinha_cr) {
    size_t fim;

    if (fgets(destino, READER_MAX_LINHA, arquivo) == NULL) {
        return false;
    }
    fim = strcspn(destino, "\r\n");
    if (tinha_cr != NULL) {
        *tinha_cr = destino[fim] == '\r';
    }
    destino[fim] = '\0';
    return true;
}

// Mostra o erro, libera o que já foi aberto e retorna NULL
static READER *falha_ao_abrir(READER *reader, const char *mensagem, const char *detalhe) {
    fprintf(stderr, "erro: %s '%s'\n", mensagem, detalhe);
    reader_fechar(&reader);
    return NULL;
}

READER *reader_abrir(const char *nome, const char *caminho) {
    READER *reader;
    char *colunas[READER_MAX_COLUNAS];
    bool tinha_cr;
    int quantidade;

    if (nome == NULL || caminho == NULL) {
        return NULL;
    }
    reader = calloc(1, sizeof(READER));
    if (reader == NULL) {
        return NULL;
    }

    // procura o leitor pelo nome
    for (size_t i = 0; i < sizeof(LEITORES) / sizeof(LEITORES[0]); i++) {
        if (strcmp(LEITORES[i]->nome, nome) == 0) {
            reader->leitor = LEITORES[i];
        }
    }
    if (reader->leitor == NULL) {
        return falha_ao_abrir(reader, "leitor desconhecido", nome);
    }

    reader->arquivo = fopen(caminho, "r");
    if (reader->arquivo == NULL) {
        return falha_ao_abrir(reader, "nao foi possivel abrir", caminho);
    }

    if (!le_linha(reader->arquivo, reader->cabecalho, &tinha_cr)) {
        return falha_ao_abrir(reader, "arquivo vazio", caminho);
    }
    reader->fim_de_linha = tinha_cr ? "\r\n" : "\n";
    reader->numero_linha = 1;

    // o cabeçalho precisa ter as colunas esperadas pelo leitor, na mesma ordem
    strcpy(reader->campos, reader->cabecalho);
    quantidade = separa_campos(reader->campos, reader->leitor->delimitador,
                               colunas, READER_MAX_COLUNAS);
    if (quantidade != reader->leitor->quantidade_colunas) {
        return falha_ao_abrir(reader, "numero de colunas inesperado em", caminho);
    }
    for (int i = 0; i < quantidade; i++) {
        if (strcmp(colunas[i], reader->leitor->colunas[i]) != 0) {
            return falha_ao_abrir(reader, "coluna esperada no cabecalho:",
                                  reader->leitor->colunas[i]);
        }
    }

    return reader;
}

// Conta a linha pulada e avisa no stderr
static void avisa_linha_invalida(READER *reader, const char *motivo) {
    reader->linhas_puladas++;
    fprintf(stderr, "aviso: linha %ld ignorada (%s)\n", reader->numero_linha, motivo);
}

ITEM *reader_proximo(READER *reader) {
    char *campos[READER_MAX_COLUNAS];

    if (reader == NULL) {
        return NULL;
    }

    // repete até achar uma linha válida ou chegar ao fim do arquivo
    while (le_linha(reader->arquivo, reader->linha, NULL)) {
        void *dados;
        char *copia;
        ITEM *item;

        reader->numero_linha++;
        if (reader->linha[0] == '\0') {
            continue; // linha em branco
        }

        strcpy(reader->campos, reader->linha);
        if (separa_campos(reader->campos, reader->leitor->delimitador, campos,
                          READER_MAX_COLUNAS) != reader->leitor->quantidade_colunas) {
            avisa_linha_invalida(reader, "numero de colunas ou aspas incorretos");
            continue;
        }

        dados = reader->leitor->converter(campos);
        if (dados == NULL) {
            avisa_linha_invalida(reader, "valor invalido");
            continue;
        }

        // o item guarda uma cópia da linha original, para a saída ficar idêntica
        copia = malloc(strlen(reader->linha) + 1);
        item = (copia != NULL) ? item_criar(strcpy(copia, reader->linha), dados, reader->leitor->ops) : NULL;
        if (item == NULL) {
            free(copia);
            reader->leitor->ops->apagar(dados);
            fprintf(stderr, "erro: memoria insuficiente\n");
            return NULL;
        }
        return item;
    }

    return NULL;
}

const char *reader_cabecalho(const READER *reader) {
    return reader != NULL ? reader->cabecalho : NULL;
}

const char *reader_fim_de_linha(const READER *reader) {
    return reader != NULL ? reader->fim_de_linha : "\n";
}

long reader_linhas_puladas(const READER *reader) {
    return reader != NULL ? reader->linhas_puladas : 0;
}

void reader_fechar(READER **reader) {
    if (reader != NULL && *reader != NULL) {
        if ((*reader)->arquivo != NULL) {
            fclose((*reader)->arquivo);
        }
        free(*reader);
        *reader = NULL;
    }
}
