#include "estrutura.h"
#include "lista_seq.h"
#include "fila.h"
#include "pilha.h"
#include "lista_enc.h"
#include "lista_cabeca.h"
#include "lista_ord.h"
#include "lista_gen.h"
#include "lista_cruz.h"

#include <stdlib.h>

// Cada estrutura concreta será acessada por meio destas operações.
// main.c não precisa conhecer a implementação interna.

struct Estrutura {
    void *dados;
    bool (*inserir)(void *dados, ITEM *item);
    size_t (*tamanho)(const void *dados);
    bool (*percorrer)(const void *dados, VisitanteItem visitante, void *contexto);
    void (*destruir)(void *dados);
};

// Adaptadores para Lista Sequencial
static bool lista_seq_inserir_adapter(void *dados, ITEM *item){
    return lista_seq_inserir((ListaSeq *)dados, item);
}
static size_t lista_seq_tamanho_adapter(const void *dados){
    return lista_seq_tamanho((const ListaSeq *)dados);
}
static bool lista_seq_percorrer_adapter(const void *dados, VisitanteItem visitante, void *contexto){
    return lista_seq_percorrer((const ListaSeq *)dados, visitante, contexto);
}
static void lista_seq_destruir_adapter(void *dados){
    lista_seq_destruir((ListaSeq *)dados);
}

// Adaptadores para Fila
static bool fila_inserir_adapter(void *dados, ITEM *item){
    return fila_inserir((FILA *)dados, item);
}
static size_t fila_tamanho_adapter(const void *dados){
    return (size_t)fila_tamanho((const FILA *)dados);
}
static bool fila_percorrer_adapter(const void *dados, VisitanteItem visitante, void *contexto){
    return fila_percorrer((const FILA *)dados, visitante, contexto);
}
static void fila_destruir_adapter(void *dados){
    FILA *f = (FILA *)dados;
    // A função fila_destruir espera um ponteiro duplo (FILA **), então passamos o endereço de f
    fila_destruir(&f); 
}

// Adaptadores para pilha
static bool pilha_inserir_adapter(void *dados, ITEM *item) {
    return pilha_inserir((PILHA *)dados, item);
}
static size_t pilha_tamanho_adapter(const void *dados) {
    return pilha_tamanho((const PILHA *)dados);
}
static bool pilha_percorrer_adapter(const void *dados, VisitanteItem visitante, void *contexto) {
    return pilha_percorrer((const PILHA *)dados, visitante, contexto);
}
static void pilha_destruir_adapter(void *dados) {
    PILHA *p = (PILHA *)dados;
    pilha_destruir(&p);
}
// Adaptadores lista encadeada
static bool lista_enc_inserir_adapter(void *dados, ITEM *item) {
    return lista_enc_inserir((ListaEnc *)dados, item);
}
static size_t lista_enc_tamanho_adapter(const void *dados) {
    return lista_enc_tamanho((const ListaEnc *)dados);
}
static bool lista_enc_percorrer_adapter(const void *dados, VisitanteItem visitante, void *contexto) {
    return lista_enc_percorrer((const ListaEnc *)dados, visitante, contexto);
}
static void lista_enc_destruir_adapter(void *dados) {
    lista_enc_destruir((ListaEnc *)dados);
}

// Adaptadores para lista cabeca
static bool lista_cabeca_inserir_adapter(void *dados, ITEM *item) {
    return lista_cabeca_inserir((ListaCabeca *)dados, item);
}
static size_t lista_cabeca_tamanho_adapter(const void *dados) {
    return lista_cabeca_tamanho((const ListaCabeca *)dados);
}
static bool lista_cabeca_percorrer_adapter(const void *dados, VisitanteItem visitante, void *contexto) {
    return lista_cabeca_percorrer((const ListaCabeca *)dados, visitante, contexto);
}
static void lista_cabeca_destruir_adapter(void *dados) {
    lista_cabeca_destruir((ListaCabeca *)dados);
}
//adapatadores para lista ordenada
static bool lista_ord_inserir_adapter(void *dados, ITEM *item) {
    return lista_ord_inserir((ListaOrd *)dados, item);
}
static size_t lista_ord_tamanho_adapter(const void *dados) {
    return lista_ord_tamanho((const ListaOrd *)dados);
}
static bool lista_ord_percorrer_adapter(const void *dados, VisitanteItem visitante, void *contexto) {
    return lista_ord_percorrer((const ListaOrd *)dados, visitante, contexto);
}
static void lista_ord_destruir_adapter(void *dados) {
    lista_ord_destruir((ListaOrd *)dados);
}
// adaptadores Lista Generalizada
static bool lista_gen_inserir_adapter(void *dados, ITEM *item) { 
    return lista_gen_inserir(dados, item); 
}
static size_t lista_gen_tamanho_adapter(const void *dados) { 
    return lista_gen_tamanho(dados); 
}
static bool lista_gen_percorrer_adapter(const void *dados, VisitanteItem visitante, void *contexto) { 
    return lista_gen_percorrer(dados, visitante, contexto); 
}
static void lista_gen_destruir_adapter(void *dados) { 
    lista_gen_destruir(dados); 
}

//adapatadores Lista Cruzada
static bool lista_cruz_inserir_adapter(void *dados, ITEM *item) { 
    return lista_cruz_inserir(dados, item); 
}
static size_t lista_cruz_tamanho_adapter(const void *dados) { 
    return lista_cruz_tamanho(dados); 
}
static bool lista_cruz_percorrer_adapter(const void *dados, VisitanteItem visitante, void *contexto) { 
    return lista_cruz_percorrer(dados, visitante, contexto); 
}
static void lista_cruz_destruir_adapter(void *dados) { 
    lista_cruz_destruir(dados); 
}

// Criação da estrutura genérica 
// 'query' define ORDER/GROUP/CROSS para lista_ord/lista_gen/lista_cruz (pode ser NULL)
Estrutura *estrutura_criar(TipoEstrutura tipo, const Query *query){
    Estrutura *estrutura = malloc(sizeof(Estrutura));
    if (estrutura == NULL) {
        return NULL;
    }
    
    estrutura->dados = NULL;
    estrutura->inserir = NULL;
    estrutura->tamanho = NULL;
    estrutura->percorrer = NULL;
    estrutura->destruir = NULL;
    
    switch (tipo) {
        case ESTRUTURA_LISTA_SEQ:
            estrutura->dados = criar_lista_seq(); 
            estrutura->inserir = lista_seq_inserir_adapter;
            estrutura->tamanho = lista_seq_tamanho_adapter;
            estrutura->percorrer = lista_seq_percorrer_adapter;
            estrutura->destruir = lista_seq_destruir_adapter;
            break;
            
        case ESTRUTURA_FILA:
            estrutura->dados = fila_criar();
            estrutura->inserir = fila_inserir_adapter;
            estrutura->tamanho = fila_tamanho_adapter;
            estrutura->percorrer = fila_percorrer_adapter;
            estrutura->destruir = fila_destruir_adapter;
            break;
            
        case ESTRUTURA_PILHA:
                estrutura->dados = pilha_criar();
                estrutura->inserir = pilha_inserir_adapter;
                estrutura->tamanho = pilha_tamanho_adapter;
                estrutura->percorrer = pilha_percorrer_adapter;
                estrutura->destruir = pilha_destruir_adapter;
                break;
            
        case ESTRUTURA_LISTA_ENC:
            estrutura->dados = criar_lista_enc();
            estrutura->inserir = lista_enc_inserir_adapter;
            estrutura->tamanho = lista_enc_tamanho_adapter;
            estrutura->percorrer = lista_enc_percorrer_adapter;
            estrutura->destruir = lista_enc_destruir_adapter;
            break;
            
        case ESTRUTURA_LISTA_CABECA:
            estrutura->dados = criar_lista_cabeca();
            estrutura->inserir = lista_cabeca_inserir_adapter;
            estrutura->tamanho = lista_cabeca_tamanho_adapter;
            estrutura->percorrer = lista_cabeca_percorrer_adapter;
            estrutura->destruir = lista_cabeca_destruir_adapter;
            break;
            
        case ESTRUTURA_LISTA_ORD:
            estrutura->dados = criar_lista_ord();
            if (estrutura->dados != NULL && query != NULL && query->order_key[0] != '\0' &&
                !lista_ord_configurar(estrutura->dados, query->order_key, query->order_desc)) {
                lista_ord_destruir(estrutura->dados);
                estrutura->dados = NULL;
            }
            estrutura->inserir = lista_ord_inserir_adapter;
            estrutura->tamanho = lista_ord_tamanho_adapter;
            estrutura->percorrer = lista_ord_percorrer_adapter;
            estrutura->destruir = lista_ord_destruir_adapter;
            break;
            
        case ESTRUTURA_LISTA_GEN:
            estrutura->dados = criar_lista_gen();
            if (estrutura->dados != NULL && query != NULL && query->group_count == 2 &&
                !lista_gen_configurar(estrutura->dados, query->group_fields[0], query->group_fields[1])) {
                lista_gen_destruir(estrutura->dados);
                estrutura->dados = NULL;
            }
            estrutura->inserir = lista_gen_inserir_adapter;
            estrutura->tamanho = lista_gen_tamanho_adapter;
            estrutura->percorrer = lista_gen_percorrer_adapter;
            estrutura->destruir = lista_gen_destruir_adapter;
            break;
            
        case ESTRUTURA_LISTA_CRUZ:
            estrutura->dados = criar_lista_cruz();
            if (estrutura->dados != NULL && query != NULL && query->cross_count == 2 &&
                !lista_cruz_configurar(estrutura->dados, query->cross_fields[0], query->cross_fields[1])) {
                lista_cruz_destruir(estrutura->dados);
                estrutura->dados = NULL;
            }
            estrutura->inserir = lista_cruz_inserir_adapter;
            estrutura->tamanho = lista_cruz_tamanho_adapter;
            estrutura->percorrer = lista_cruz_percorrer_adapter;
            estrutura->destruir = lista_cruz_destruir_adapter;
            break;

        default:
            free(estrutura);
            return NULL;
    }

    if (estrutura->dados == NULL) {
        free(estrutura);
        return NULL;
    }

    return estrutura;
}

// Funções Polimórficas (permanecem inalteradas)

bool estrutura_inserir(Estrutura *estrutura, ITEM *item){
    if (estrutura == NULL || item == NULL || estrutura->inserir == NULL) {
        return false;
    }
    return estrutura->inserir(estrutura->dados, item);
}

size_t estrutura_tamanho(const Estrutura *estrutura){
    if (estrutura == NULL || estrutura->tamanho == NULL) {
        return 0;
    }
    return estrutura->tamanho(estrutura->dados);
}

bool estrutura_percorrer(const Estrutura *estrutura, VisitanteItem visitante, void *contexto){
    if (estrutura == NULL || visitante == NULL || estrutura->percorrer == NULL) {
        return false;
    }
    return estrutura->percorrer(estrutura->dados, visitante, contexto);
}

void estrutura_destruir(Estrutura *estrutura) {
    if (estrutura == NULL) {
        return;
    }
    if (estrutura->destruir != NULL && estrutura->dados != NULL) {
        estrutura->destruir(estrutura->dados);
    }
    free(estrutura);
}