#include "reader.h"
#include "reader_nasa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "reader_patrimonio.h"

struct reader_ {
    FILE *arquivo;
    const LEITOR *leitor;          // leitor escolhido em --reader
    const char *fim_de_linha;      // "\r\n" ou "\n", igual ao da entrada
    long numero_linha;             // usado nos avisos (cabeçalho = linha 1)
    long linhas_puladas;
    char cabecalho[READER_MAX_LINHA];
    char linha[READER_MAX_LINHA];  // linha original, que vai para o item
    char campos[READER_MAX_LINHA]; // cópia da linha, separada em campos
    char extra[READER_MAX_LINHA];  // linha física seguinte, quando o registro tem quebra de linha entre aspas
};

// Leitores disponíveis em --reader (na Parte 2, basta acrescentar o novo aqui)
static const LEITOR *LEITORES[] = { &LEITOR_NASA, &LEITOR_PATRIMONIO };

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

// Lê uma linha e remove o "\r\n" (ou "\n").
// Retorna 1 se leu, 0 no fim do arquivo, -1 se a linha excede READER_MAX_LINHA
// (nesse caso o resto da linha é descartado).
// Se 'tinha_cr' não for NULL, informa se a linha terminava com '\r'
static int le_linha(FILE *arquivo, char *destino, bool *tinha_cr) {
    size_t fim;

    if (fgets(destino, READER_MAX_LINHA, arquivo) == NULL) {
        return 0;
    }
    fim = strcspn(destino, "\r\n");
    if (destino[fim] == '\0' && fim == READER_MAX_LINHA - 1) {
        int c = getc(arquivo);
        if (c != EOF && c != '\n') {
            while (c != EOF && c != '\n') {
                c = getc(arquivo);
            }
            destino[0] = '\0';
            if (tinha_cr != NULL) {
                *tinha_cr = false;
            }
            return -1;
        }
    }
    if (tinha_cr != NULL) {
        *tinha_cr = destino[fim] == '\r';
    }
    destino[fim] = '\0';
    return 1;
}

// Pula o BOM UTF-8 (EF BB BF) do início do arquivo, se existir
static size_t salta_bom(const char *texto) {
    return strncmp(texto, "\xEF\xBB\xBF", 3) == 0 ? 3 : 0;
}

static bool caractere_util(unsigned char c) {
    return c < 128 && isalnum(c);
}

/* Compara nomes de colunas ignorando maiúsculas, espaços, pontuação e bytes
   não ASCII (acentos). Assim "Ano Tombamento " casa com "Ano Tombamento" e o
   cabeçalho é aceito em UTF-8 ou Latin-1. A ordem e a quantidade de colunas
   continuam sendo validadas. */
static bool nomes_iguais(const char *a, const char *b) {
    for (;;) {
        while (*a != '\0' && !caractere_util((unsigned char)*a)) a++;
        while (*b != '\0' && !caractere_util((unsigned char)*b)) b++;
        if (*a == '\0' || *b == '\0') {
            return *a == *b;
        }
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        a++;
        b++;
    }
}

static int conta_aspas(const char *texto) {
    int n = 0;
    for (; *texto != '\0'; texto++) {
        if (*texto == '"') n++;
    }
    return n;
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
            break;
        }
    }
    if (reader->leitor == NULL) {
        return falha_ao_abrir(reader, "leitor desconhecido", nome);
    }

    reader->arquivo = fopen(caminho, "r");
    if (reader->arquivo == NULL) {
        return falha_ao_abrir(reader, "nao foi possivel abrir", caminho);
    }

    if (le_linha(reader->arquivo, reader->cabecalho, &tinha_cr) <= 0) {
        return falha_ao_abrir(reader, "arquivo vazio", caminho);
    }
    reader->fim_de_linha = tinha_cr ? "\r\n" : "\n";
    reader->numero_linha = 1;

    // o cabeçalho precisa ter as colunas esperadas pelo leitor, na mesma ordem
    strcpy(reader->campos, reader->cabecalho + salta_bom(reader->cabecalho));
    quantidade = separa_campos(reader->campos, reader->leitor->delimitador,
                               colunas, READER_MAX_COLUNAS);
    if (quantidade != reader->leitor->quantidade_colunas) {
        return falha_ao_abrir(reader, "numero de colunas inesperado em", caminho);
    }
    for (int i = 0; i < quantidade; i++) {
        if (!nomes_iguais(colunas[i], reader->leitor->colunas[i])) {
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

/* Um campo entre aspas pode conter quebras de linha (ex.: Observações).
   Enquanto houver aspas abertas, junta as linhas físicas seguintes em
   reader->linha, mantendo o fim de linha original, para a saída ficar idêntica.
   Retorna 1 se o registro fechou, 0 se o arquivo acabou com aspas abertas e
   -1 se o registro não cabe no buffer. */
static int completa_registro(READER *reader) {
    bool aberta = conta_aspas(reader->linha) % 2 != 0;
    bool cabe = true;
    size_t tam_fim = strlen(reader->fim_de_linha);

    while (aberta) {
        int estado = le_linha(reader->arquivo, reader->extra, NULL);
        size_t usado = strlen(reader->linha);
        size_t novo = strlen(reader->extra);

        if (estado == 0) {
            return 0;
        }
        reader->numero_linha++;
        if (estado < 0) {
            return -1;
        }
        if (cabe && usado + tam_fim + novo < READER_MAX_LINHA) {
            memcpy(reader->linha + usado, reader->fim_de_linha, tam_fim);
            memcpy(reader->linha + usado + tam_fim, reader->extra, novo + 1);
        } else {
            cabe = false;
        }
        if (conta_aspas(reader->extra) % 2 != 0) {
            aberta = !aberta;
        }
    }
    return cabe ? 1 : -1;
}

ITEM *reader_proximo(READER *reader) {
    char *campos[READER_MAX_COLUNAS];
    int estado;

    if (reader == NULL) {
        return NULL;
    }

    // repete até achar uma linha válida ou chegar ao fim do arquivo
    while ((estado = le_linha(reader->arquivo, reader->linha, NULL)) != 0) {
        void *dados;
        char *copia;
        ITEM *item;

        reader->numero_linha++;
        if (estado < 0) {
            avisa_linha_invalida(reader, "linha maior que o limite");
            continue;
        }
        if (reader->linha[0] == '\0') {
            continue; // linha em branco
        }

        estado = completa_registro(reader);
        if (estado == 0) {
            avisa_linha_invalida(reader, "aspas nao fechadas ate o fim do arquivo");
            continue;
        }
        if (estado < 0) {
            avisa_linha_invalida(reader, "registro maior que o limite");
            continue;
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