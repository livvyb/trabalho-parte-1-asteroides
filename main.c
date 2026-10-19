
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "item.h"
#include "query.h"

int main(int argc, char *argv[]) {
    ARGS args;
    Query query;
    Asteroid ast;
    double similaridade = 0.0;

    /* 1. Le os argumentos da linha de comando */
    if (!parse_cli_args(argc, argv, &args)) {
        printf("Erro: argumentos invalidos ou incompletos.\n");
        printf("Uso: ./trabalho --reader nasa --input arquivo.csv "
               "--base lista_enc --result lista_ord "
               "--query consulta.txt --output resultado.csv "
               "--top-k 100 --limit 10000\n");
        return 1;
    }

    printf("Argumentos carregados com sucesso.\n");
    printf("Arquivo de entrada: %s\n", args.input_path);
    printf("Arquivo de consulta: %s\n", args.query_path);

    /* 2. Carrega a consulta */
    if (!carrega_query(args.query_path, &query)) {
        printf("Erro: nao foi possivel carregar a consulta.\n");
        return 1;
    }

    printf("Consulta carregada com sucesso.\n");

    /* 3. Cria um asteroide ficticio para o teste */
    memset(&ast, 0, sizeof(Asteroid));

    strcpy(ast.asteroid_designation, "TESTE-001");
    strcpy(ast.asteroid_fullname, "Asteroide de teste");
    strcpy(ast.close_approach_date, "2027-01-15");

    ast.year = 2027;
    ast.month = 1;
    ast.distance_au = 0.05;
    ast.velocity_km_s = 15.0;
    ast.absolute_magnitude = 22.0;
    ast.has_absolute_magnitude = true;
    ast.days_until_approach = 90;
    ast.is_past_event = false;
    ast.is_future_event = true;
    ast.risk_score = 0.70;
    ast.panic_level = 2;

    strcpy(ast.threat_category, "BAIXO");
    strcpy(ast.panic_verdict, "Teste");

    ast.on_sentry_list = false;
    ast.has_sentry_data = false;

    /* 4. Avalia o asteroide de acordo com a consulta */
    printf("\nAvaliando o asteroide de teste...\n");

    if (avalia_query(&query, &ast, &similaridade)) {
        printf("Resultado: ASTEROIDE ACEITO\n");
        printf("Designacao: %s\n", ast.asteroid_designation);
        printf("Distancia: %.4f AU\n", ast.distance_au);
        printf("Velocidade: %.2f km/s\n", ast.velocity_km_s);
        printf("Pontuacao de risco: %.2f\n", ast.risk_score);
        printf("Similaridade: %.4f\n", similaridade);
    } else {
        printf("Resultado: ASTEROIDE REJEITADO\n");
        printf("O asteroide nao satisfez a consulta.\n");
    }

    printf("\nTeste finalizado.\n");

    return 0;
}
