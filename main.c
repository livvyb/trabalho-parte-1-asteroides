#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cli.h"
#include "reader.h"
#include "query.h"
#include "estrutura.h"
#include "item.h"

// Contexto do visitante de busca
typedef struct {
    Query *query;
    Estrutura *resultado;
    long combinacoes;
    double tempo_insercao_ms; // tempo gasto só inserindo em --result
    bool erro;
} ContextoBusca;

// Contexto do visitante de escrita
typedef struct {
    FILE *arquivo_saida;
    const char *fim_linha;
    int impressos;
    int limite_top_k;
} ContextoEscrita;

// Dono dos itens: guarda cada ITEM* uma única vez, para liberar no final
// sem depender da ordem de destruição das estruturas.
typedef struct {
    ITEM **itens;
    size_t quantidade;
    size_t capacidade;
} Donos;

static double agora_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static bool donos_adicionar(Donos *d, ITEM *item) {
    if (d->quantidade == d->capacidade) {
        size_t nova = d->capacidade ? d->capacidade * 2 : 1024;
        ITEM **novo = realloc(d->itens, nova * sizeof(ITEM *));
        if (novo == NULL) {
            return false;
        }
        d->itens = novo;
        d->capacidade = nova;
    }
    d->itens[d->quantidade++] = item;
    return true;
}

// Converte a string do CLI para o tipo da Estrutura. Retorna false se inválida.
static bool mapear_tipo_estrutura(const char *nome, TipoEstrutura *tipo) {
    static const struct { const char *nome; TipoEstrutura tipo; } TABELA[] = {
        {"pilha", ESTRUTURA_PILHA},
        {"fila", ESTRUTURA_FILA},
        {"lista_seq", ESTRUTURA_LISTA_SEQ},
        {"lista_enc", ESTRUTURA_LISTA_ENC},
        {"lista_cabeca", ESTRUTURA_LISTA_CABECA},
        {"lista_ord", ESTRUTURA_LISTA_ORD},
        {"lista_gen", ESTRUTURA_LISTA_GEN},
        {"lista_cruz", ESTRUTURA_LISTA_CRUZ},
    };
    for (size_t i = 0; i < sizeof(TABELA) / sizeof(TABELA[0]); i++) {
        if (nome != NULL && strcmp(nome, TABELA[i].nome) == 0) {
            *tipo = TABELA[i].tipo;
            return true;
        }
    }
    return false;
}

// Visitante 1: avalia cada item da base; se aprovado, insere nos resultados.
static bool visitante_busca(const ITEM *c_item, void *ctx) {
    ContextoBusca *contexto = (ContextoBusca *)ctx;
    ITEM *item = (ITEM *)c_item; // o item é mutável (similaridade)
    double similaridade;

    if (avalia_query(contexto->query, item, &similaridade)) {
        double t0;
        bool ok;

        item_set_similaridade(item, similaridade);
        t0 = agora_ms();
        ok = estrutura_inserir(contexto->resultado, item);
        contexto->tempo_insercao_ms += agora_ms() - t0;
        if (!ok) {
            contexto->erro = true;
            return false;
        }
        contexto->combinacoes++;
    }
    return true;
}

// Visitante 2: escreve o item na saída; para ao atingir o top-k.
static bool visitante_escrita(const ITEM *item, void *ctx) {
    ContextoEscrita *contexto = (ContextoEscrita *)ctx;

    if (contexto->limite_top_k > 0 && contexto->impressos >= contexto->limite_top_k) {
        return false;
    }
    item_escrever(item, contexto->fim_linha, contexto->arquivo_saida);
    contexto->impressos++;
    return true;
}

int main(int argc, char *argv[]) {
    int status = EXIT_FAILURE;
    ARGS args;
    TipoEstrutura tipo_base, tipo_result;
    Estrutura *base = NULL;
    Estrutura *resultado = NULL;
    READER *reader = NULL;
    FILE *out = NULL;
    Donos donos = {NULL, 0, 0};
    Query query;
    long registros_lidos = 0;
    ITEM *item_atual;
    double t0, load_ms, search_ms, insert_ms, write_ms;

    if (!parse_cli_args(argc, argv, &args)) {
        fprintf(stderr, "Erro ao processar argumentos.\n");
        goto fim;
    }
    if (!mapear_tipo_estrutura(args.base_struct, &tipo_base)) {
        fprintf(stderr, "Erro: --base invalido '%s'\n", args.base_struct);
        goto fim;
    }
    if (!mapear_tipo_estrutura(args.result_struct, &tipo_result)) {
        fprintf(stderr, "Erro: --result invalido '%s'\n", args.result_struct);
        goto fim;
    }

    // A query é carregada antes: ORDER/GROUP/CROSS configuram as estruturas
    if (!carrega_query(args.query_path, &query)) {
        fprintf(stderr, "Erro ao carregar arquivo de query '%s'.\n", args.query_path);
        goto fim;
    }

    base = estrutura_criar(tipo_base, &query);
    resultado = estrutura_criar(tipo_result, &query);
    if (base == NULL || resultado == NULL) {
        fprintf(stderr, "Erro ao criar estruturas de dados.\n");
        goto fim;
    }

    reader = reader_abrir(args.reader, args.input_path);
    if (reader == NULL) {
        goto fim;
    }

    // Carga: lê os registros e insere na estrutura base
    t0 = agora_ms();
    while ((item_atual = reader_proximo(reader)) != NULL) {
        if (!donos_adicionar(&donos, item_atual)) {
            item_apagar(&item_atual);
            fprintf(stderr, "Erro: memoria insuficiente.\n");
            goto fim;
        }
        if (!estrutura_inserir(base, item_atual)) {
            fprintf(stderr, "Erro ao inserir na estrutura base.\n");
            goto fim;
        }
        registros_lidos++;
        if (args.limit > 0 && registros_lidos >= args.limit) {
            break;
        }
    }
    load_ms = agora_ms() - t0;

    // Busca: percorre a base e insere os aceitos no resultado
    ContextoBusca ctx_busca = {&query, resultado, 0, 0.0, false};
    t0 = agora_ms();
    estrutura_percorrer(base, visitante_busca, &ctx_busca);
    search_ms = agora_ms() - t0;
    if (ctx_busca.erro) {
        fprintf(stderr, "Erro ao inserir na estrutura de resultado.\n");
        goto fim;
    }
    insert_ms = ctx_busca.tempo_insercao_ms;
    search_ms -= insert_ms; // SEARCH_MS não inclui o tempo de inserção
    if (search_ms < 0.0) {
        search_ms = 0.0;
    }

    // Escrita do arquivo de saída
    t0 = agora_ms();
    out = fopen(args.output_path, "w");
    if (out == NULL) {
        fprintf(stderr, "Erro ao abrir arquivo de saida '%s'.\n", args.output_path);
        goto fim;
    }
    fprintf(out, "%s%s", reader_cabecalho(reader), reader_fim_de_linha(reader));
    ContextoEscrita ctx_escrita = {out, reader_fim_de_linha(reader), 0, args.top_k};
    estrutura_percorrer(resultado, visitante_escrita, &ctx_escrita);
    if (fclose(out) != 0) {
        out = NULL;
        fprintf(stderr, "Erro ao gravar arquivo de saida.\n");
        goto fim;
    }
    out = NULL;
    write_ms = agora_ms() - t0;

    printf("STATUS=OK\n");
    printf("INPUT_RECORDS=%ld\n", registros_lidos);
    printf("MATCHED_RECORDS=%ld\n", ctx_busca.combinacoes);
    printf("OUTPUT_RECORDS=%d\n", ctx_escrita.impressos);
    printf("LOAD_MS=%.3f\n", load_ms);
    printf("SEARCH_MS=%.3f\n", search_ms);
    printf("RESULT_INSERT_MS=%.3f\n", insert_ms);
    printf("WRITE_MS=%.3f\n", write_ms);
    status = EXIT_SUCCESS;

fim:
    if (status != EXIT_SUCCESS) {
        printf("STATUS=ERROR\n");
    }
    if (out != NULL) {
        fclose(out);
    }
    // As estruturas só guardam ponteiros; os itens são liberados uma única vez aqui,
    // depois de as duas estruturas terem sido destruídas.
    estrutura_destruir(resultado);
    estrutura_destruir(base);
    for (size_t i = 0; i < donos.quantidade; i++) {
        item_apagar(&donos.itens[i]);
    }
    free(donos.itens);
    reader_fechar(&reader);

    return status;
}