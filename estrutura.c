#include "estrutura.h"
#include "lista_seq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Cada estrutura concreta tem a sua própria API (lista_seq_inserir,
   pilha_empilhar...). Os adaptadores abaixo convertem o void* para o tipo
   real e repassam a chamada, assim a main usa sempre as mesmas funções. */
struct Estrutura {
    void *dados; // a estrutura real (ListaSeq*, PILHA*...)
    bool (*inserir)(void *dados, ITEM *item);
    size_t (*tamanho)(const void *dados);
    bool (*percorrer)(const void *dados, VisitanteItem visitante, void *contexto);
    void (*destruir)(void *dados);
};

// Nomes aceitos em --base/--result, na mesma ordem de TipoEstrutura
static const char *NOMES[] = {
    "pilha", "fila", "lista_seq", "lista_enc",
    "lista_cabeca", "lista_ord", "lista_gen", "lista_cruz"
};

// Adaptadores da lista sequencial

static bool lista_seq_inserir_adapter(void *dados, ITEM *item) {
    return lista_seq_inserir((ListaSeq *)dados, item);
}

static size_t lista_seq_tamanho_adapter(const void *dados) {
    return lista_seq_tamanho((const ListaSeq *)dados);
}

static bool lista_seq_percorrer_adapter(const void *dados, VisitanteItem visitante, void *contexto) {
    return lista_seq_percorrer((const ListaSeq *)dados, visitante, contexto);
}

static void lista_seq_destruir_adapter(void *dados) {
    lista_seq_destruir((ListaSeq *)dados);
}

// Configuração vinda da query 

// Chave do ORDER; sem ORDER, usa "similarity DESC"
static void obter_ordem(const Query *query, const char **chave, bool *decrescente) {
    if (query == NULL || query->order_key[0] == '\0') {
        fprintf(stderr, "aviso: consulta sem ORDER, usando similarity DESC\n");
        *chave = "similarity";
        *decrescente = true;
        return;
    }
    *chave = query->order_key;
    *decrescente = query->order_desc;
}

// GROUP e CROSS não têm padrão genérico: sem eles a estrutura não é criada
static bool tem_campos(int quantidade, int minimo, const char *comando) {
    if (quantidade < minimo) {
        fprintf(stderr, "erro: a estrutura escolhida exige %s com %d campo(s) na consulta\n",
                comando, minimo);
        return false;
    }
    return true;
}

// Interface pública

bool estrutura_tipo_por_nome(const char *nome, TipoEstrutura *tipo) {
    if (nome == NULL || tipo == NULL) {
        return false;
    }
    for (int i = 0; i < (int)(sizeof(NOMES) / sizeof(NOMES[0])); i++) {
        if (strcmp(NOMES[i], nome) == 0) {
            *tipo = (TipoEstrutura)i;
            return true;
        }
    }
    return false;
}

Estrutura *estrutura_criar(TipoEstrutura tipo, const Query *query) {
    Estrutura *estrutura = calloc(1, sizeof(Estrutura));
    const char *chave;
    bool decrescente;

    if (estrutura == NULL) {
        return NULL;
    }

    /* Para ligar uma estrutura nova: criar os 4 adaptadores (como os da
       lista_seq acima), preencher os campos no case correspondente e
       incluir o .h e o .o (Makefile). */
    switch (tipo) {
        case ESTRUTURA_LISTA_SEQ:
            estrutura->dados = lista_seq_criar();
            estrutura->inserir = lista_seq_inserir_adapter;
            estrutura->tamanho = lista_seq_tamanho_adapter;
            estrutura->percorrer = lista_seq_percorrer_adapter;
            estrutura->destruir = lista_seq_destruir_adapter;
            break;

        case ESTRUTURA_PILHA:
            // TODO: estrutura->dados = pilha_criar(); + adaptadores
            break;

        case ESTRUTURA_FILA:
            // TODO: estrutura->dados = fila_criar(); + adaptadores
            break;

        case ESTRUTURA_LISTA_ENC:
            // TODO: estrutura->dados = lista_enc_criar(); + adaptadores
            break;

        case ESTRUTURA_LISTA_CABECA:
            // TODO: estrutura->dados = lista_cabeca_criar(); + adaptadores
            break;

        case ESTRUTURA_LISTA_ORD:
            obter_ordem(query, &chave, &decrescente);
            // TODO: estrutura->dados = lista_ord_criar(chave, decrescente); + adaptadores
            (void)chave; // remover estas duas linhas ao implementar
            (void)decrescente;
            break;

        case ESTRUTURA_LISTA_GEN:
            if (!tem_campos(query != NULL ? query->group_count : 0, 1, "GROUP")) {
                free(estrutura);
                return NULL;
            }
            // TODO: estrutura->dados = lista_gen_criar(query->group_fields, query->group_count); + adaptadores
            break;

        case ESTRUTURA_LISTA_CRUZ:
            if (!tem_campos(query != NULL ? query->cross_count : 0, 2, "CROSS")) {
                free(estrutura);
                return NULL;
            }
            // TODO: estrutura->dados = lista_cruz_criar(query->cross_fields[0], query->cross_fields[1]); + adaptadores
            break;
    }

    if (estrutura->dados == NULL) {
        if (estrutura->inserir == NULL) {
            fprintf(stderr, "erro: estrutura '%s' ainda nao implementada\n", NOMES[tipo]);
        }
        free(estrutura);
        return NULL;
    }

    return estrutura;
}

bool estrutura_inserir(Estrutura *estrutura, ITEM *item) {
    if (estrutura == NULL || item == NULL) {
        return false;
    }
    return estrutura->inserir(estrutura->dados, item);
}

size_t estrutura_tamanho(const Estrutura *estrutura) {
    if (estrutura == NULL) {
        return 0;
    }
    return estrutura->tamanho(estrutura->dados);
}

bool estrutura_percorrer(const Estrutura *estrutura, VisitanteItem visitante, void *contexto) {
    if (estrutura == NULL || visitante == NULL) {
        return false;
    }
    return estrutura->percorrer(estrutura->dados, visitante, contexto);
}

void estrutura_destruir(Estrutura **estrutura) {
    if (estrutura != NULL && *estrutura != NULL) {
        (*estrutura)->destruir((*estrutura)->dados);
        free(*estrutura);
        *estrutura = NULL;
    }
}
