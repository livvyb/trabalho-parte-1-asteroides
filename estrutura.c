#include "estrutura.h"
#include "lista_seq.h"

#include <stdlib.h>

// Cada estrutura concreta será acessada por meio destas operações.
// main.c não precisa conhecer a implementação interna.

struct Estrutura {
    void *dados;
    bool (*inserir)(void *dados, ITEM *item);
    size_t (*tamanho)(const void *dados);
    bool (*percorrer)(
        const void *dados,
        VisitanteItem visitante,
        void *contexto
    );
    void (*destruir)(void *dados);
};

// Adaptadores para ListaSeq

static bool lista_seq_inserir_adapter(void *dados, ITEM *item){
    return lista_seq_inserir((ListaSeq *)dados, item);
}

static size_t lista_seq_tamanho_adapter(const void *dados){
    return lista_seq_tamanho((const ListaSeq *)dados);
}

static bool lista_seq_percorrer_adapter( const void *dados, VisitanteItem visitante, void *contexto){
    return lista_seq_percorrer(
        (const ListaSeq *)dados,
        visitante,
        contexto
    );
}

static void lista_seq_destruir_adapter(void *dados)
{
    lista_seq_destruir((ListaSeq *)dados);
}

// Criação da estrutura genérica 

Estrutura *estrutura_criar(TipoEstrutura tipo){
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
            estrutura->dados = lista_seq_criar();
            estrutura->inserir = lista_seq_inserir_adapter;
            estrutura->tamanho = lista_seq_tamanho_adapter;
            estrutura->percorrer = lista_seq_percorrer_adapter;
            estrutura->destruir = lista_seq_destruir_adapter;
            break;
        case ESTRUTURA_PILHA:

        break;
        case ESTRUTURA_FILA:

        break;
        case ESTRUTURA_LISTA_ENC:

        break;
        case ESTRUTURA_LISTA_CABECA:

        break;
        case ESTRUTURA_LISTA_ORD:

        break;
        case ESTRUTURA_LISTA_GEN:

        break;
        case ESTRUTURA_LISTA_CRUZ:

        break;

            free(estrutura);
            return NULL;
    }

    if (estrutura->dados == NULL) {
        free(estrutura);
        return NULL;
    }

    return estrutura;
}

// Inserção polimórfica 
bool estrutura_inserir(Estrutura *estrutura, ITEM *item)
{
    if (estrutura == NULL || item == NULL || estrutura->inserir == NULL) {
        return false;
    }

    return estrutura->inserir(estrutura->dados, item);
}

// Consulta polimórfica do tamanho 
size_t estrutura_tamanho(const Estrutura *estrutura)
{
    if (estrutura == NULL || estrutura->tamanho == NULL) {
        return 0;
    }

    return estrutura->tamanho(estrutura->dados);
}

// Percurso polimórfico
bool estrutura_percorrer(const Estrutura *estrutura, VisitanteItem visitante, void *contexto){
    if (estrutura == NULL || visitante == NULL ||
        estrutura->percorrer == NULL) {
        return false;
    }

    return estrutura->percorrer(
        estrutura->dados,
        visitante,
        contexto
    );
}

// Destruição polimórfica 
void estrutura_destruir(Estrutura *estrutura)
{
    if (estrutura == NULL) {
        return;
    }
    if (estrutura->destruir != NULL && estrutura->dados != NULL) {
        estrutura->destruir(estrutura->dados);
    }

    free(estrutura);
}
