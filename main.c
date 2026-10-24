#include <stdio.h>
#include "cli.h"
#include "item.h"
#include "query.h"
#include "reader.h"

/* Versão provisória: lê o CSV com o reader escolhido e avalia a consulta
   em cada registro. As estruturas (--base/--result) ainda não foram
   integradas, então cada item é liberado logo após ser avaliado. */
int main(int argc, char *argv[]) {
    ARGS args;
    Query query;
    READER *reader;
    ITEM *item;
    long lidos = 0;
    long aceitos = 0;

    // 1. Le os argumentos da linha de comando
    if (!parse_cli_args(argc, argv, &args)) {
        printf("Erro: argumentos invalidos ou incompletos.\n");
        printf("Uso: ./trabalho --reader nasa --input arquivo.csv "
               "--base lista_enc --result lista_ord "
               "--query consulta.txt --output resultado.csv "
               "--top-k 100 --limit 10000\n");
        return 1;
    }

    // 2. Carrega a consulta
    if (!carrega_query(args.query_path, &query)) {
        printf("STATUS=ERROR\n");
        fprintf(stderr, "erro: nao foi possivel carregar a consulta\n");
        return 1;
    }

    // 3. Abre o dataset com o leitor pedido em --reader
    reader = reader_abrir(args.reader, args.input_path);
    if (reader == NULL) {
        printf("STATUS=ERROR\n");
        return 1;
    }

    // 4. Le registro a registro (respeitando --limit; 0 = todos)
    while ((args.limit == 0 || lidos < args.limit) &&
           (item = reader_proximo(reader)) != NULL) {
        double similaridade;

        lidos++;

        if (avalia_query(&query, item, &similaridade)) {
            item_set_similaridade(item, similaridade);
            aceitos++;
        }
        item_apagar(&item);
    }

    if (reader_linhas_puladas(reader) > 0) {
        fprintf(stderr, "aviso: %ld linhas invalidas ignoradas no total\n",
                reader_linhas_puladas(reader));
    }
    reader_fechar(&reader);

    printf("STATUS=OK\n");
    printf("INPUT_RECORDS=%ld\n", lidos);
    printf("MATCHED_RECORDS=%ld\n", aceitos);

    return 0;
}
