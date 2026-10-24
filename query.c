// interpreta e armazena os comandos 
#include "query.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> //comparar sem case sensitive
#include <math.h>
#include <errno.h> // ver os erros da conversao numerica
#include <ctype.h> // ver espaços em branco (isspace)

static void copia_texto(char *destino, size_t tamanho, const char *origem) {
    // se nao há espaço disponível 
    if (tamanho == 0){
        return;
    }
    strncpy(destino, origem, tamanho - 1);
    // garantir ultimo espaço como \0
    destino[tamanho - 1] = '\0';
}
// remove espaços em branco do início e ao final do texto e nao no meio
// MODIFICA A STRING ORIGINAL
static char *remove_espacos(char *texto) {
    char *fim;
    while (isspace((unsigned char)*texto)){
        // enquanto houver espaço em branco avança o ponteiro
        texto++;
    }
    if (*texto == '\0') {
        return texto;
    }
    // ultimo caracter efetivo 
    fim = texto + strlen(texto) - 1;
    while (fim > texto && isspace((unsigned char)*fim)) {
        *fim = '\0';
        fim--;
    }
    return texto;
}

static bool converte_numero(const char *texto, double *valor) {
    char *fim;
    double numero;
    errno = 0;
    // convertendo o texto
    // fim aponta pro primeiro caracter que nao faz parte do numero eg. \0 se for tudo numero
    numero = strtod(texto, &fim);
    if (texto == fim || errno == ERANGE || !isfinite(numero)) {
        return false;
    }
    while (isspace((unsigned char)*fim)){
        fim++;
    }
    if (*fim != '\0'){
        return false;
    }
    *valor = numero;
    return true;
}
// strcasecmp -> reconhece variações true, True, TRUE, 1
static bool converte_bool(const char *texto, bool *valor) {
    if (strcasecmp(texto, "true") == 0 ||
        strcasecmp(texto, "1") == 0) {
        *valor = true;
        return true;
    }
    if (strcasecmp(texto, "false") == 0 ||
        strcasecmp(texto, "0") == 0) {
        *valor = false;
        return true;
    }
    return false;
}

static bool converte_operador(const char *texto, OpType *op) {
    if (strcmp(texto, "==") == 0) {
        *op = OP_EQ;        //  igual (==)
    } else if (strcmp(texto, "!=") == 0) {
        *op = OP_DIFF;      // diferente (!=)
    } else if (strcmp(texto, "<") == 0) {
        *op = OP_MIN;        // menor (<)
    } else if (strcmp(texto, "<=") == 0) {
        *op = OP_MINEQ;        // menor ou igual (<=)
    } else if (strcmp(texto, ">") == 0) {
        *op = OP_MAX;        // maior (>)
    } else if (strcmp(texto, ">=") == 0) {
        *op = OP_MAXEQ;        // maior ou igual (>=)
    } else {
        return false;
    }
    return true;
}

static bool compara_numero(double valor, double alvo, OpType op) {
    switch (op) {
        case OP_EQ:  return valor == alvo;
        case OP_DIFF: return valor != alvo;
        case OP_MIN:  return valor < alvo;
        case OP_MINEQ:  return valor <= alvo;
        case OP_MAX:  return valor > alvo;
        case OP_MAXEQ:  return valor >= alvo;
    }
    return false;
}

static bool compara_texto(const char *valor, const char *alvo, OpType op) {
    int comparacao = strcmp(valor, alvo);
    // strcmp positivo se valor vier depois de alvo negativo se vier antes na ordem lexicografica
    switch (op) {
        case OP_EQ:  return comparacao == 0;
        case OP_DIFF: return comparacao != 0;
        case OP_MIN:  return comparacao < 0;
        case OP_MINEQ:  return comparacao <= 0;
        case OP_MAX: return comparacao > 0;
        case OP_MAXEQ:  return comparacao >= 0;
    }
    return false;
}
// COMECAMOS A FAZER AS VERIFICAÇÕES UTILIZANDO INFORMACOES DO ASTEROIDS
static bool avalia_filtro(const Filter *f, const ITEM *item) {
    bool valido;
    bool valor_bool;
    double valor_num;
    const char *valor_texto;
     // Os campos booleanos sao tratados separadamente.
    valor_bool = item_get_bool(item, f->field, &valido);
    if (valido) {
        if (!f->is_bool){
            return false;
            // se o campo for booleano mas o valor armazenado nao, é rejeitado
        }
        return compara_numero(
            valor_bool ? 1.0 : 0.0,
            f->target_bool ? 1.0 : 0.0,
            f->op
            // converte booleano em número para reaproveitar funcao de comparaçao
        );
    }

    // Tenta obter o campo como numero.
    valor_num = item_get_numero(item, f->field, &valido);
    if (valido) {
        if (f->is_bool){
            return false;
        }
        return compara_numero(valor_num, f->target_num, f->op);
    }

    // Se nao for numerico nem booleano, tenta como texto.
    valor_texto = item_get_texto(item, f->field);
    if (valor_texto == NULL || valor_texto[0] == '\0') {
        return false;
    }
    if (f->is_bool) return false;
    return compara_texto(valor_texto, f->target_str, f->op);
}
// abre q1.txt, le cada linha, identifica o comando e armazena informacoes na estrutura query
bool carrega_query(const char *filepath, Query *q) {   // filepath -> caminho do arquivo de consulta 
    FILE *arquivo;
    char linha[512];
    // q = eh onde as regras estao guardadas 
    if (filepath == NULL || q == NULL){
        return false;
    }
    memset(q, 0, sizeof(Query)); // preenche todos os bytes com zero
    arquivo = fopen(filepath, "r");
    if (arquivo == NULL){
        return false;
    }
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char *comando;
        char *campo; 
        char *operador;
        char *valor;
        char *extra;
        // limpando e ignorando linhas 
        comando = remove_espacos(linha);
        if (*comando == '\0' || *comando == '#'){
            continue;
        } 
        comando[strcspn(comando, "\r\n")] = '\0';   // remove quebras de linha 
        comando = remove_espacos(comando);
        if (*comando == '\0'){
            continue; // pular pra outra linha
        }
        campo = strtok(comando, "|");   // divide uma string em partes pelo delimitador '|'
        if (campo == NULL) {
            fclose(arquivo);
            return false;
        }
        campo = remove_espacos(campo);
        if (strcmp(campo, "FILTER") == 0) {   
            Filter *f;
            if (q->filter_count >= MAX_FILTERS) {
                fclose(arquivo);
                return false;
            }

            campo = strtok(NULL, "|");
            operador = strtok(NULL, "|");
            valor = strtok(NULL, "|");
            extra = strtok(NULL, "|");

            if (campo == NULL || operador == NULL ||
                valor == NULL || extra != NULL) {
                fclose(arquivo);
                return false;     // FILTER|campo|operador|valor é o modelo esperado
            }

            campo = remove_espacos(campo);
            operador = remove_espacos(operador);
            valor = remove_espacos(valor);

            f = &q->filters[q->filter_count];

            if (strlen(campo) >= sizeof(f->field) ||
                strlen(valor) >= sizeof(f->target_str) ||
                !converte_operador(operador, &f->op)) {
                fclose(arquivo);
                return false;
            }

            copia_texto(f->field, sizeof(f->field), campo);
            copia_texto(f->target_str, sizeof(f->target_str), valor);

            if (converte_bool(valor, &f->target_bool)) {
                f->is_bool = true;
            } else {
                f->is_bool = false;

            // O valor pode ser numerico ou textual.
            // Campos numericos serao reconhecidos na avaliacao.

                if (!converte_numero(valor, &f->target_num)) {
                    f->target_num = 0.0;
                }
            }

            q->filter_count++;

            // UM NOVO FILTRO FOI ARMAZENADO -->  consulta é valida e pode ser carregada

        } else if (strcmp(campo, "SIM") == 0) {
            Sim *s;
            char *alvo;
            char *tolerancia;
            char *peso;

            // SIM|campo|alvo|tolerancia|peso

            if (q->sim_count >= MAX_SIMS) {
                fclose(arquivo);
                return false;
            }

            campo = strtok(NULL, "|");
            alvo = strtok(NULL, "|");
            tolerancia = strtok(NULL, "|");
            peso = strtok(NULL, "|");
            extra = strtok(NULL, "|");

            if (campo == NULL || alvo == NULL ||
                tolerancia == NULL || peso == NULL ||
                extra != NULL) {
                fclose(arquivo);
                return false;
            }

            campo = remove_espacos(campo);
            alvo = remove_espacos(alvo);
            tolerancia = remove_espacos(tolerancia);
            peso = remove_espacos(peso);

            s = &q->sims[q->sim_count];  // s aponta para a proxima posicao livre no array de criterios de similaridade

            if (strlen(campo) >= sizeof(s->field) ||
                !converte_numero(alvo, &s->target) ||
                !converte_numero(tolerancia, &s->tolerance) ||
                !converte_numero(peso, &s->weight) ||
                s->tolerance < 0.0 ||   // precisa ser maior que zero pois vai dividir na formula da similaridade
                s->weight < 0.0) {
                fclose(arquivo);
                return false;
            }
            copia_texto(s->field, sizeof(s->field), campo);
            q->sim_count++;
            // NOVO CRITERIO DE SIMILARIDADE FOI ARMAZENADO

        } else if (strcmp(campo, "ACCEPT") == 0) { // LIMIAR MÍNIMO PRAR A SIMILARIDADE TOTAL
            valor = strtok(NULL, "|");
            extra = strtok(NULL, "|");

            if (valor == NULL || extra != NULL || !converte_numero(remove_espacos(valor),&q->accept_threshold) ||
                q->accept_threshold < 0.0 || q->accept_threshold > 1.0) {
                fclose(arquivo);
                return false;
            }

        } else if (strcmp(campo, "ORDER") == 0) { 
            campo = strtok(NULL, "|");
            valor = strtok(NULL, "|");   // CHAVE USADA PELA LISTA ORDENADA
            extra = strtok(NULL, "|");

            if (campo == NULL || valor == NULL || extra != NULL) {
                fclose(arquivo);
                return false;
            }

            campo = remove_espacos(campo);
            valor = remove_espacos(valor);

            if (strlen(campo) >= sizeof(q->order_key) || (strcmp(valor, "ASC") != 0 && strcmp(valor, "DESC") != 0)) {
                fclose(arquivo);
                return false;
            }
            copia_texto(q->order_key, sizeof(q->order_key), campo);
            q->order_desc = strcmp(valor, "DESC") == 0;

        } else if (strcmp(campo, "GROUP") == 0) {
            char *campo1 = strtok(NULL, "|");
            char *campo2 = strtok(NULL, "|");   // campos usados pela lista ordenada

            if (campo1 == NULL || campo2 == NULL || strtok(NULL, "|") != NULL ||
                strlen(campo1) >= sizeof(q->group_fields[0]) || strlen(campo2) >= sizeof(q->group_fields[1])) {
                fclose(arquivo);
                return false;
            }
            copia_texto(q->group_fields[0],
                        sizeof(q->group_fields[0]),
                        remove_espacos(campo1));

            copia_texto(q->group_fields[1],
                        sizeof(q->group_fields[1]),
                        remove_espacos(campo2));

            q->group_count = 2;

        } else if (strcmp(campo, "CROSS") == 0) {
            char *campo1 = strtok(NULL, "|");
            char *campo2 = strtok(NULL, "|");    // campos usados pela lista cruzada

            if (campo1 == NULL || campo2 == NULL ||
                strtok(NULL, "|") != NULL ||
                strlen(campo1) >= sizeof(q->cross_fields[0]) ||
                strlen(campo2) >= sizeof(q->cross_fields[1])) {
                fclose(arquivo);
                return false;
            }

            copia_texto(q->cross_fields[0],
                        sizeof(q->cross_fields[0]),
                        remove_espacos(campo1));

            copia_texto(q->cross_fields[1],
                        sizeof(q->cross_fields[1]),
                        remove_espacos(campo2));

            q->cross_count = 2;

        } else {
            fclose(arquivo);
            return false;
        }
    }

    if (ferror(arquivo)) {
        fclose(arquivo);
        return false;
    }

    fclose(arquivo);

    return true;
}

bool avalia_query(const Query *q, const ITEM *item,
                  double *out_similarity) {
    double soma_ponderada = 0.0;
    double soma_pesos = 0.0;
    double similaridade = 1.0;

    if (q == NULL || item == NULL || out_similarity == NULL) {
        return false;
    }

    *out_similarity = 0.0;

    // Todos os filtros sao obrigatorios.

    for (int i = 0; i < q->filter_count; i++) {
        if (!avalia_filtro(&q->filters[i], item)) {
            return false;
        }
    }

    //  Se houver SIM, calcula a media ponderada.

    for (int i = 0; i < q->sim_count; i++) {
        bool valido;
        double valor;
        double diferenca;
        double sim;
        const Sim *s = &q->sims[i];

        valor = item_get_numero(item, s->field, &valido);

        if (!valido) {
            return false;
        }

        diferenca = fabs(valor - s->target);
        sim = 1.0 - diferenca / s->tolerance;

        if (sim < 0.0) sim = 0.0;
        if (sim > 1.0) sim = 1.0;

        soma_ponderada += sim * s->weight;
        soma_pesos += s->weight;
    }

    if (q->sim_count > 0) {
        if (soma_pesos <= 0.0) return false;

        similaridade = soma_ponderada / soma_pesos;
    }

    *out_similarity = similaridade;

    // ACCEPT so rejeita pela similaridade quando existe SIM.

    if (q->sim_count > 0 &&
        similaridade < q->accept_threshold) {
        return false;
    }

    return true;
}