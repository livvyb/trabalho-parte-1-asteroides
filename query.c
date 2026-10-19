#include "query.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

static void copia_texto(char *destino, size_t tamanho, const char *origem) {
    if (tamanho == 0) return;

    strncpy(destino, origem, tamanho - 1);
    destino[tamanho - 1] = '\0';
}

static char *remove_espacos(char *texto) {
    char *fim;

    while (isspace((unsigned char)*texto)) texto++;

    if (*texto == '\0') return texto;

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
    numero = strtod(texto, &fim);

    if (texto == fim || errno == ERANGE || !isfinite(numero)) {
        return false;
    }

    while (isspace((unsigned char)*fim)) fim++;

    if (*fim != '\0') return false;

    *valor = numero;
    return true;
}

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
        *op = OP_EQ;
    } else if (strcmp(texto, "!=") == 0) {
        *op = OP_NEQ;
    } else if (strcmp(texto, "<") == 0) {
        *op = OP_LT;
    } else if (strcmp(texto, "<=") == 0) {
        *op = OP_LE;
    } else if (strcmp(texto, ">") == 0) {
        *op = OP_GT;
    } else if (strcmp(texto, ">=") == 0) {
        *op = OP_GE;
    } else {
        return false;
    }

    return true;
}

static bool compara_numero(double valor, double alvo, OpType op) {
    switch (op) {
        case OP_EQ:  return valor == alvo;
        case OP_NEQ: return valor != alvo;
        case OP_LT:  return valor < alvo;
        case OP_LE:  return valor <= alvo;
        case OP_GT:  return valor > alvo;
        case OP_GE:  return valor >= alvo;
    }

    return false;
}

static bool compara_texto(const char *valor, const char *alvo, OpType op) {
    int comparacao = strcmp(valor, alvo);

    switch (op) {
        case OP_EQ:  return comparacao == 0;
        case OP_NEQ: return comparacao != 0;
        case OP_LT:  return comparacao < 0;
        case OP_LE:  return comparacao <= 0;
        case OP_GT:  return comparacao > 0;
        case OP_GE:  return comparacao >= 0;
    }

    return false;
}

static bool avalia_filtro(const Filter *f, const Asteroid *ast) {
    bool valido;
    bool valor_bool;
    double valor_num;
    const char *valor_texto;

    /*
     * Os campos booleanos sao tratados separadamente.
     */
    valor_bool = asteroid_get_bool_field(ast, f->field, &valido);

    if (valido) {
        if (!f->is_bool) return false;

        return compara_numero(
            valor_bool ? 1.0 : 0.0,
            f->target_bool ? 1.0 : 0.0,
            f->op
        );
    }

    /*
     * Tenta obter o campo como numero.
     */
    valor_num = asteroid_get_numeric_field(ast, f->field, &valido);

    if (valido) {
        if (f->is_bool) return false;

        return compara_numero(valor_num, f->target_num, f->op);
    }

    /*
     * Se nao for numerico nem booleano, tenta como texto.
     */
    valor_texto = asteroid_get_string_field(ast, f->field);

    if (valor_texto == NULL || valor_texto[0] == '\0') {
        return false;
    }

    if (f->is_bool) return false;

    return compara_texto(valor_texto, f->target_str, f->op);
}

bool carrega_query(const char *filepath, Query *q) {
    FILE *arquivo;
    char linha[512];

    if (filepath == NULL || q == NULL) return false;

    memset(q, 0, sizeof(Query));

    arquivo = fopen(filepath, "r");

    if (arquivo == NULL) return false;

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char *comando;
        char *campo;
        char *operador;
        char *valor;
        char *extra;

        comando = remove_espacos(linha);

        if (*comando == '\0' || *comando == '#') continue;

        comando[strcspn(comando, "\r\n")] = '\0';

        comando = remove_espacos(comando);

        if (*comando == '\0') continue;

        campo = strtok(comando, "|");

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
                return false;
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

                /*
                 * O valor pode ser numerico ou textual.
                 * Campos numericos serao reconhecidos na avaliacao.
                 */
                if (!converte_numero(valor, &f->target_num)) {
                    f->target_num = 0.0;
                }
            }

            q->filter_count++;

        } else if (strcmp(campo, "SIM") == 0) {
            Sim *s;
            char *alvo;
            char *tolerancia;
            char *peso;

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

            s = &q->sims[q->sim_count];

            if (strlen(campo) >= sizeof(s->field) ||
                !converte_numero(alvo, &s->target) ||
                !converte_numero(tolerancia, &s->tolerance) ||
                !converte_numero(peso, &s->weight) ||
                s->tolerance <= 0.0 ||
                s->weight <= 0.0) {
                fclose(arquivo);
                return false;
            }

            copia_texto(s->field, sizeof(s->field), campo);
            q->sim_count++;

        } else if (strcmp(campo, "ACCEPT") == 0) {
            valor = strtok(NULL, "|");
            extra = strtok(NULL, "|");

            if (valor == NULL || extra != NULL ||
                !converte_numero(remove_espacos(valor),
                                 &q->accept_threshold) ||
                q->accept_threshold < 0.0 ||
                q->accept_threshold > 1.0) {
                fclose(arquivo);
                return false;
            }

        } else if (strcmp(campo, "ORDER") == 0) {
            campo = strtok(NULL, "|");
            valor = strtok(NULL, "|");
            extra = strtok(NULL, "|");

            if (campo == NULL || valor == NULL || extra != NULL) {
                fclose(arquivo);
                return false;
            }

            campo = remove_espacos(campo);
            valor = remove_espacos(valor);

            if (strlen(campo) >= sizeof(q->order_key) ||
                (strcmp(valor, "ASC") != 0 &&
                 strcmp(valor, "DESC") != 0)) {
                fclose(arquivo);
                return false;
            }

            copia_texto(q->order_key, sizeof(q->order_key), campo);
            q->order_desc = strcmp(valor, "DESC") == 0;

        } else if (strcmp(campo, "GROUP") == 0) {
            char *campo1 = strtok(NULL, "|");
            char *campo2 = strtok(NULL, "|");

            if (campo1 == NULL || campo2 == NULL ||
                strtok(NULL, "|") != NULL ||
                strlen(campo1) >= sizeof(q->group_fields[0]) ||
                strlen(campo2) >= sizeof(q->group_fields[1])) {
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
            char *campo2 = strtok(NULL, "|");

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

bool avalia_query(const Query *q, const Asteroid *ast,
                  double *out_similarity) {
    double soma_ponderada = 0.0;
    double soma_pesos = 0.0;
    double similaridade = 1.0;

    if (q == NULL || ast == NULL || out_similarity == NULL) {
        return false;
    }

    *out_similarity = 0.0;

    /*
     * Todos os filtros sao obrigatorios.
     */
    for (int i = 0; i < q->filter_count; i++) {
        if (!avalia_filtro(&q->filters[i], ast)) {
            return false;
        }
    }

    /*
     * Se houver SIM, calcula a media ponderada.
     */
    for (int i = 0; i < q->sim_count; i++) {
        bool valido;
        double valor;
        double diferenca;
        double sim;
        const Sim *s = &q->sims[i];

        valor = asteroid_get_numeric_field(ast, s->field, &valido);

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

    /*
     * ACCEPT so rejeita pela similaridade quando existe SIM.
     */
    if (q->sim_count > 0 &&
        similaridade < q->accept_threshold) {
        return false;
    }

    return true;
}
