#ifndef ESTRUTURA_H
#define ESTRUTURA_H

#include "item.h"
#include "query.h"
#include <stdbool.h>
#include <stddef.h>

/* Interface única para as 8 estruturas. A main só usa estas funções e não
   precisa saber qual estrutura foi escolhida em --base ou --result.

   Regras que toda estrutura deve seguir:
   - guarda apenas ponteiros ITEM*, nunca cópias dos itens;
   - nunca libera os itens (base e resultado compartilham os mesmos ponteiros;
     quem libera é a main, uma única vez);
   - o percurso segue a ordem de acesso da estrutura, que define o top-k:
       pilha                              -> do topo para a base
       fila                               -> do início para o fim
       lista_seq, lista_enc, lista_cabeca -> ordem de inserção
       lista_ord                          -> ordem do ORDER
       lista_gen                          -> grupo a grupo, com grupos e subgrupos
                                             em ordem crescente do campo (item_comparar)
       lista_cruz                         -> pela 1a dimensão do CROSS e depois pela 2a
   - para comparar itens (ORDER, GROUP, CROSS), use item_comparar (item.h);
   - funções nomeadas como <estrutura>_<metodo> (ex.: lista_seq_criar,
     pilha_empilhar, lista_enc_inserir). */

typedef enum {
    ESTRUTURA_PILHA,
    ESTRUTURA_FILA,
    ESTRUTURA_LISTA_SEQ,
    ESTRUTURA_LISTA_ENC,
    ESTRUTURA_LISTA_CABECA,
    ESTRUTURA_LISTA_ORD,
    ESTRUTURA_LISTA_GEN,
    ESTRUTURA_LISTA_CRUZ
} TipoEstrutura;

typedef struct Estrutura Estrutura;

// Chamado para cada item durante o percurso. Retornar false interrompe (ex.: top-k)
typedef bool (*VisitanteItem)(ITEM *item, void *contexto);

// Converte o nome usado em --base/--result (ex.: "lista_enc") no tipo
bool estrutura_tipo_por_nome(const char *nome, TipoEstrutura *tipo);

/* Cria a estrutura. A query fornece ORDER, GROUP e CROSS para lista_ord,
   lista_gen e lista_cruz (as demais a ignoram) e deve continuar existindo
   enquanto a estrutura for usada.
   Sem ORDER, a lista_ord usa "similarity DESC" (com aviso no stderr).
   Sem GROUP/CROSS, lista_gen/lista_cruz não são criadas.
   Retorna NULL em caso de erro, com mensagem no stderr. */
Estrutura *estrutura_criar(TipoEstrutura tipo, const Query *query);

bool estrutura_inserir(Estrutura *estrutura, ITEM *item);

size_t estrutura_tamanho(const Estrutura *estrutura);

// Retorna false se os parâmetros forem inválidos ou se o visitante interromper
bool estrutura_percorrer(const Estrutura *estrutura, VisitanteItem visitante, void *contexto);

// Libera a estrutura e seus nós, mas não os itens
void estrutura_destruir(Estrutura **estrutura);

#endif
