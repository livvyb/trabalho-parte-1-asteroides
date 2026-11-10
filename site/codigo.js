// Gerado por site/ferramentas/gerar_dados.py: trechos copiados dos arquivos .c do trabalho
window.CODIGO_C = {
 "pilha": [
  {
   "arquivo": "pilha.c",
   "funcao": "pilha_inserir",
   "codigo": "bool pilha_inserir(PILHA *p, ITEM *item) {\n    if (p == NULL || item == NULL) return false;\n    \n    NoPilha *novo = malloc(sizeof(NoPilha));\n    if (novo == NULL) return false;\n    \n    novo->item = item;\n    novo->prox = p->topo;\n    p->topo = novo;\n    p->tamanho++;\n    \n    return true;\n}"
  },
  {
   "arquivo": "pilha.c",
   "funcao": "pilha_percorrer",
   "codigo": "bool pilha_percorrer(const PILHA *p, VisitanteItem visitante, void *contexto) {\n    if (p == NULL || visitante == NULL) return false;\n    \n    for (NoPilha *atual = p->topo; atual != NULL; atual = atual->prox) {\n        if (!visitante(atual->item, contexto)) {\n            return false;\n        }\n    }\n    return true;\n}"
  }
 ],
 "fila": [
  {
   "arquivo": "fila.c",
   "funcao": "fila_inserir",
   "codigo": "bool fila_inserir(FILA *f, ITEM *item) {\n    if (f == NULL || item == NULL) return false;\n    \n    NoFila *novo = malloc(sizeof(NoFila));\n    if (novo == NULL) return false;\n    \n    novo->item = item;\n    novo->prox = NULL;\n    \n    if (f->fim == NULL) {\n        f->inicio = novo;\n    } else {\n        f->fim->prox = novo;\n    }\n    f->fim = novo;\n    f->tamanho++;\n    \n    return true;\n}"
  },
  {
   "arquivo": "fila.c",
   "funcao": "fila_percorrer",
   "codigo": "bool fila_percorrer(const FILA *f, VisitanteItem visitante, void *contexto) {\n    if (f == NULL || visitante == NULL) return false;\n    \n    for (NoFila *atual = f->inicio; atual != NULL; atual = atual->prox) {\n        if (!visitante(atual->item, contexto)) {\n            return false;\n        }\n    }\n    return true;\n}"
  }
 ],
 "lista_seq": [
  {
   "arquivo": "lista_seq.c",
   "funcao": "lista_seq_inserir",
   "codigo": "bool lista_seq_inserir(ListaSeq *lista, ITEM *item)\n{\n    if (lista == NULL || item == NULL) {\n        return false;\n    }\n    /* Se o vetor estiver cheio, aumenta sua capacidade */\n    if (lista->tamanho == lista->capacidade) {\n        if (lista->capacidade > SIZE_MAX / 2) {\n            return false;\n        }\n        size_t nova_capacidade = lista->capacidade * 2;\n\n        if (nova_capacidade > SIZE_MAX / sizeof(ITEM *)) {\n            return false;\n        }\n        ITEM **novo_vetor = realloc(\n            lista->itens,\n            nova_capacidade * sizeof(ITEM *)\n        );\n        if (novo_vetor == NULL) {\n            return false;\n        }\n        lista->itens = novo_vetor;\n        lista->capacidade = nova_capacidade;\n    }\n    lista->itens[lista->tamanho] = item;\n    lista->tamanho++;\n    return true;\n}"
  }
 ],
 "lista_enc": [
  {
   "arquivo": "lista_enc.c",
   "funcao": "lista_enc_inserir",
   "codigo": "bool lista_enc_inserir(ListaEnc *lista, ITEM *item) {\n    if (lista == NULL || item == NULL) return false;\n\n    NoLista *novo = malloc(sizeof(NoLista));\n    if (novo == NULL) return false;\n\n    novo->item = item;\n    novo->prox = NULL;\n\n    if (lista->fim == NULL) {\n        lista->inicio = novo;\n    } else {\n        lista->fim->prox = novo;\n    }\n    lista->fim = novo;\n    lista->tamanho++;\n\n    return true;\n}"
  },
  {
   "arquivo": "lista_enc.c",
   "funcao": "lista_enc_percorrer",
   "codigo": "bool lista_enc_percorrer(const ListaEnc *lista, VisitanteItem visitante, void *contexto) {\n    if (lista == NULL || visitante == NULL) return false;\n\n    for (NoLista *atual = lista->inicio; atual != NULL; atual = atual->prox) {\n        if (!visitante(atual->item, contexto)) {\n            return false;\n        }\n    }\n    return true;\n}"
  }
 ],
 "lista_cabeca": [
  {
   "arquivo": "lista_cabeca.c",
   "funcao": "criar_lista_cabeca",
   "codigo": "ListaCabeca *criar_lista_cabeca(void) {\n    ListaCabeca *lista = malloc(sizeof(ListaCabeca));\n    if (lista != NULL) {\n        lista->sentinela = malloc(sizeof(NoCabeca));\n        if (lista->sentinela != NULL) {\n            lista->sentinela->item = NULL; \n            lista->sentinela->prox = NULL;\n            lista->fim = lista->sentinela;\n            lista->tamanho = 0;\n        } else {\n            free(lista);\n            return NULL;\n        }\n    }\n    return lista;\n}"
  },
  {
   "arquivo": "lista_cabeca.c",
   "funcao": "lista_cabeca_inserir",
   "codigo": "bool lista_cabeca_inserir(ListaCabeca *lista, ITEM *item) {\n    if (lista == NULL || item == NULL) return false;\n\n    NoCabeca *novo = malloc(sizeof(NoCabeca));\n    if (novo == NULL) return false;\n    novo->item = item;\n    novo->prox = NULL;\n    lista->fim->prox = novo;\n    lista->fim = novo;\n    lista->tamanho++;\n\n    return true;\n}"
  }
 ],
 "lista_ord": [
  {
   "arquivo": "lista_ord.c",
   "funcao": "vem_antes",
   "codigo": "// Itens sem a chave ficam sempre no final, em qualquer direção\nstatic bool vem_antes(const ListaOrd *lista, const NoOrd *a, const NoOrd *b) {\n    if (!a->tem_chave) return false;\n    if (!b->tem_chave) return true;\n    int c = compara_chaves(a, b);\n    return lista->desc ? c > 0 : c < 0;\n}"
  },
  {
   "arquivo": "lista_ord.c",
   "funcao": "lista_ord_inserir",
   "codigo": "bool lista_ord_inserir(ListaOrd *lista, ITEM *item) {\n    if (lista == NULL || item == NULL) return false;\n\n    NoOrd *novo = malloc(sizeof(NoOrd));\n    if (novo == NULL) return false;\n        novo->item = item;\n        novo->prox = NULL;\n        novo->texto = NULL;\n        if (lista->usa_similaridade) {\n            novo->chave = item_get_similaridade(item);\n            novo->tem_chave = true;\n        } else {\n            bool ok;\n            novo->chave = item_get_numero(item, lista->campo, &ok);\n            if (!ok) {\n                // não é numérico: tenta como texto\n                const char *t = item_get_texto(item, lista->campo);\n                if (t != NULL && t[0] != '\\0') {\n                    novo->texto = t;\n                    ok = true;\n                }\n            }\n            novo->tem_chave = ok;\n    }\n\n    // A ordem é mantida durante a inserção (inserção ordenada estável)\n    if (lista->inicio == NULL) {\n        lista->inicio = lista->fim = novo;\n    } else if (!vem_antes(lista, novo, lista->fim)) {\n        // não vem antes do último: entra no final (caso rápido, O(1))\n        lista->fim->prox = novo;\n        lista->fim = novo;\n    } else if (vem_antes(lista, novo, lista->inicio)) {\n        novo->prox = lista->inicio;\n        lista->inicio = novo;\n    } else {\n        NoOrd *atual = lista->inicio;\n        while (!vem_antes(lista, novo, atual->prox)) {\n            atual = atual->prox;\n        }\n        novo->prox = atual->prox;\n        atual->prox = novo;\n    }\n\n    lista->tamanho++;\n    return true;\n}"
  }
 ],
 "lista_gen": [
  {
   "arquivo": "lista_gen.c",
   "funcao": "lista_gen_inserir",
   "codigo": "bool lista_gen_inserir(ListaGen *lista, ITEM *item) {\n    NoGen *atual, *folha;\n    if (lista == NULL || item == NULL) return false;\n\n    atual = &lista->raiz;\n    for (int nivel = 0; nivel < lista->niveis; nivel++) {\n        char buf[48];\n        const char *chave = chave_do_item(item, lista->campos[nivel], buf, sizeof(buf));\n        NoGen *f = atual->u.sub.filhos;\n\n        while (f != NULL && !(f->eh_sublista && strcmp(f->u.sub.chave, chave) == 0)) {\n            f = f->prox;\n        }\n        if (f == NULL) {                  // cria a sublista deste nível\n            f = calloc(1, sizeof(NoGen));\n            if (f == NULL) return false;\n            f->eh_sublista = true;\n            f->u.sub.chave = duplica(chave);\n            if (f->u.sub.chave == NULL) { free(f); return false; }\n            anexa(atual, f);\n        }\n        atual = f;\n    }\n\n    folha = calloc(1, sizeof(NoGen));\n    if (folha == NULL) return false;\n    folha->u.item = item;\n    anexa(atual, folha);\n    lista->tamanho++;\n    return true;\n}"
  },
  {
   "arquivo": "lista_gen.c",
   "funcao": "percorre_sub",
   "codigo": "static bool percorre_sub(const NoGen *sub, VisitanteItem visitante, void *contexto) {\n    for (const NoGen *f = sub->u.sub.filhos; f != NULL; f = f->prox) {\n        if (f->eh_sublista) {\n            if (!percorre_sub(f, visitante, contexto)) return false;\n        } else if (!visitante(f->u.item, contexto)) {\n            return false;\n        }\n    }\n    return true;\n}"
  }
 ],
 "lista_cruz": [
  {
   "arquivo": "lista_cruz.c",
   "funcao": "lista_cruz_inserir",
   "codigo": "bool lista_cruz_inserir(ListaCruz *lista, ITEM *item) {\n    char buf1[48], buf2[48];\n    if (!lista || !item) return false;\n\n    Descritor *l = acha_ou_cria(&lista->linhas, &lista->ultima_linha,\n                                chave_do_item(item, lista->campo_linha, buf1, sizeof(buf1)));\n    if (!l) return false;\n    Descritor *c = acha_ou_cria(&lista->colunas, &lista->ultima_coluna,\n                                chave_do_item(item, lista->campo_coluna, buf2, sizeof(buf2)));\n    if (!c) return false;\n\n    NoCruzado *novo = malloc(sizeof(NoCruzado));\n    if (!novo) return false;\n    novo->item = item;\n    novo->prox_linha = NULL;\n    novo->prox_coluna = NULL;\n\n    // mesmo nó encadeado nas duas organizações, sem duplicar o registro\n    if (l->fim) l->fim->prox_linha = novo; else l->inicio = novo;\n    l->fim = novo;\n    if (c->fim) c->fim->prox_coluna = novo; else c->inicio = novo;\n    c->fim = novo;\n\n    lista->tamanho++;\n    return true;\n}"
  },
  {
   "arquivo": "lista_cruz.c",
   "funcao": "lista_cruz_percorrer_por_coluna",
   "codigo": "// Percurso pela 2ª organização (colunas, ex.: year), sobre os mesmos nós\nbool lista_cruz_percorrer_por_coluna(const ListaCruz *lista, VisitanteItem visitante, void *contexto) {\n    if (!lista || !visitante) return false;\n    for (Descritor *c = lista->colunas; c; c = c->prox) {\n        for (NoCruzado *n = c->inicio; n; n = n->prox_coluna) {\n            if (!visitante(n->item, contexto)) return false;\n        }\n    }\n    return true;\n}"
  }
 ],
 "query": [
  {
   "arquivo": "query.c",
   "funcao": "avalia_filtro",
   "codigo": "static bool avalia_filtro(const Filter *f, const ITEM *item) {\n    bool valido;\n    bool valor_bool;\n    double valor_num;\n    const char *valor_texto;\n\n    // Campo booleano: o alvo precisa ser true/false/1/0\n    valor_bool = item_get_bool(item, f->field, &valido);\n    if (valido) {\n        if (!f->is_bool) {\n            return false;\n        }\n        return compara_numero(valor_bool ? 1.0 : 0.0, f->target_bool ? 1.0 : 0.0, f->op);\n    }\n\n    // Campo numérico: o alvo precisa ser um número (target_num é NAN caso contrário)\n    valor_num = item_get_numero(item, f->field, &valido);\n    if (valido) {\n        if (isnan(f->target_num)) {\n            return false;\n        }\n        return compara_numero(valor_num, f->target_num, f->op);\n    }\n\n    // Campo textual: compara com o texto original do alvo (inclusive \"0\" e \"1\")\n    valor_texto = item_get_texto(item, f->field);\n    if (valor_texto == NULL || valor_texto[0] == '\\0') {\n        return false;\n    }\n    return compara_texto(valor_texto, f->target_str, f->op);\n}"
  },
  {
   "arquivo": "query.c",
   "funcao": "avalia_query",
   "codigo": "bool avalia_query(const Query *q, const ITEM *item,\n                  double *out_similarity) {\n    double soma_ponderada = 0.0;\n    double soma_pesos = 0.0;\n    double similaridade = 1.0;\n\n    if (q == NULL || item == NULL || out_similarity == NULL) {\n        return false;\n    }\n\n    *out_similarity = 0.0;\n\n    // Todos os filtros sao obrigatorios.\n\n    for (int i = 0; i < q->filter_count; i++) {\n        if (!avalia_filtro(&q->filters[i], item)) {\n            return false;\n        }\n    }\n\n    //  Se houver SIM, calcula a media ponderada.\n\n    for (int i = 0; i < q->sim_count; i++) {\n        bool valido;\n        double valor;\n        double diferenca;\n        double sim;\n        const Sim *s = &q->sims[i];\n\n        valor = item_get_numero(item, s->field, &valido);\n\n        if (!valido) {\n            return false;\n        }\n\n        diferenca = fabs(valor - s->target);\n        sim = 1.0 - diferenca / s->tolerance;\n\n        if (sim < 0.0) sim = 0.0;\n        if (sim > 1.0) sim = 1.0;\n\n        soma_ponderada += sim * s->weight;\n        soma_pesos += s->weight;\n    }\n\n    if (q->sim_count > 0) {\n        if (soma_pesos <= 0.0) return false;\n\n        similaridade = soma_ponderada / soma_pesos;\n    }\n\n    *out_similarity = similaridade;\n\n    // ACCEPT so rejeita pela similaridade quando existe SIM.\n\n    if (q->sim_count > 0 &&\n        similaridade < q->accept_threshold) {\n        return false;\n    }\n\n    return true;\n}"
  }
 ],
 "main": [
  {
   "arquivo": "main.c",
   "funcao": "visitante_busca",
   "codigo": "// Visitante 1: avalia cada item da base; se aprovado, insere nos resultados.\nstatic bool visitante_busca(const ITEM *c_item, void *ctx) {\n    ContextoBusca *contexto = (ContextoBusca *)ctx;\n    ITEM *item = (ITEM *)c_item; // o item é mutável (similaridade)\n    double similaridade;\n\n    if (avalia_query(contexto->query, item, &similaridade)) {\n        double t0;\n        bool ok;\n\n        item_set_similaridade(item, similaridade);\n        t0 = agora_ms();\n        ok = estrutura_inserir(contexto->resultado, item);\n        contexto->tempo_insercao_ms += agora_ms() - t0;\n        if (!ok) {\n            contexto->erro = true;\n            return false;\n        }\n        contexto->combinacoes++;\n    }\n    return true;\n}"
  },
  {
   "arquivo": "main.c",
   "funcao": "visitante_escrita",
   "codigo": "// Visitante 2: escreve o item na saída; para ao atingir o top-k.\nstatic bool visitante_escrita(const ITEM *item, void *ctx) {\n    ContextoEscrita *contexto = (ContextoEscrita *)ctx;\n\n    if (contexto->limite_top_k > 0 && contexto->impressos >= contexto->limite_top_k) {\n        return false;\n    }\n    item_escrever(item, contexto->fim_linha, contexto->arquivo_saida);\n    contexto->impressos++;\n    return true;\n}"
  }
 ]
};
