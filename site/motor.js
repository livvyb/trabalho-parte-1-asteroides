/* motor.js — a mesma lógica do programa em C, escrita em JavaScript.
 *
 * Cada parte aponta para o arquivo .c que ela copia:
 *   leitor ............ reader.c + reader_patrimonio.c
 *   consulta .......... query.c (carrega_query e avalia_query)
 *   estruturas ........ pilha.c, fila.c, lista_seq.c, lista_enc.c, lista_cabeca.c,
 *                       lista_ord.c, lista_gen.c, lista_cruz.c
 *   execução .......... main.c (carga -> busca -> escrita)
 *
 * A única coisa a mais é a contagem de "passos" de cada inserção e os detalhes
 * de cada avaliação, usados pela interface para animar e comparar as estruturas.
 * site/ferramentas/verificar.js confere que a saída é idêntica à do binário.
 */
(function (raiz) {
  'use strict';

  /* ===================================================================
   * Leitor (reader.c + reader_patrimonio.c)
   * =================================================================== */
  const APELIDOS = [
    'bem', 'tipo_protecao', 'processo', 'municipio', 'cidades',
    'regiao_administrativa', 'resolucao_tombamento', 'tipo_area_envoltoria',
    'resolucao_area_envoltoria', 'processos_vinculados', 'ano_tombamento',
    'livro_tombo', 'publicacao_doe', 'observacoes', 'ata_data_so',
  ];
  const COL_ANO = APELIDOS.indexOf('ano_tombamento');
  const TOTAL_CAMPOS = APELIDOS.length;
  const READER_MAX_LINHA = 8192;
  const CODIFICADOR = new TextEncoder();

  const ehEspaco = (c) => c === ' ' || c === '\t' || c === '\n' || c === '\v' || c === '\f' || c === '\r';
  const bytes = (s) => CODIFICADOR.encode(s).length;

  // remove só os espaços ASCII das pontas (isspace no locale "C")
  function apara(s) {
    let i = 0;
    let f = s.length;
    while (i < f && ehEspaco(s[i])) i++;
    while (f > i && ehEspaco(s[f - 1])) f--;
    return s.slice(i, f);
  }

  // separa_campos: aspas ligam/desligam o modo "entre aspas" e são removidas
  function separaCampos(texto, delimitador) {
    const campos = [];
    let atual = '';
    let entreAspas = false;
    for (const c of texto) {
      if (c === '"') entreAspas = !entreAspas;
      else if (c === delimitador && !entreAspas) { campos.push(atual); atual = ''; }
      else atual += c;
    }
    campos.push(atual);
    return entreAspas ? null : campos;
  }

  const contaAspas = (s) => { let n = 0; for (const c of s) if (c === '"') n++; return n; };

  // patrimonio_converter: campo vazio vira null; ano só se for inteiro
  function converte(campos) {
    const texto = new Array(TOTAL_CAMPOS).fill(null);
    let ano = null;
    for (let i = 0; i < TOTAL_CAMPOS; i++) {
      const c = apara(campos[i]);
      if (i === COL_ANO) {
        if (/^[+-]?\d+$/.test(c)) ano = parseInt(c, 10);
      } else if (c !== '') {
        texto[i] = c;
      }
    }
    return { texto, ano };
  }

  /* Lê o CSV inteiro como o reader.c: linha física a linha física, juntando
     registros com aspas abertas, conferindo o cabeçalho e o nº de colunas. */
  function lerCSV(conteudo) {
    const fisicas = conteudo.split('\n');
    if (fisicas.length && fisicas[fisicas.length - 1] === '') fisicas.pop();
    let pos = 0;
    const avisos = [];

    // le_linha: corta no primeiro \r ou \n; linha longa demais é descartada
    function leLinha() {
      if (pos >= fisicas.length) return null;
      const bruta = fisicas[pos++];
      if (bytes(bruta) > READER_MAX_LINHA - 1) return { longa: true };
      const r = bruta.indexOf('\r');
      return { texto: r >= 0 ? bruta.slice(0, r) : bruta, tinhaCR: r >= 0 };
    }

    const cab = leLinha();
    if (!cab || cab.longa) throw new Error('arquivo vazio');
    const fimDeLinha = cab.tinhaCR ? '\r\n' : '\n';
    const cabecalho = cab.texto;

    const itens = [];
    let numeroLinha = 1;
    for (let l = leLinha(); l !== null; l = leLinha()) {
      numeroLinha++;
      if (l.longa) { avisos.push(`linha ${numeroLinha} ignorada (linha maior que o limite)`); continue; }
      if (l.texto === '') continue;

      // completa_registro: campo entre aspas pode atravessar linhas
      let linha = l.texto;
      let aberta = contaAspas(linha) % 2 !== 0;
      let fechou = true;
      while (aberta) {
        const extra = leLinha();
        if (extra === null) { fechou = false; break; }
        numeroLinha++;
        if (extra.longa) { fechou = false; break; }
        linha += fimDeLinha + extra.texto;
        if (contaAspas(extra.texto) % 2 !== 0) aberta = !aberta;
      }
      if (!fechou) { avisos.push(`linha ${numeroLinha} ignorada (registro incompleto)`); continue; }

      const campos = separaCampos(linha, ',');
      if (campos === null || campos.length !== TOTAL_CAMPOS) {
        avisos.push(`linha ${numeroLinha} ignorada (numero de colunas ou aspas incorretos)`);
        continue;
      }
      const dados = converte(campos);
      itens.push({ id: itens.length, linha, texto: dados.texto, ano: dados.ano, similaridade: 0 });
    }
    return { cabecalho, fimDeLinha, itens, avisos };
  }

  // Acesso aos campos pelo apelido (patrimonio_get_numero / _get_texto / _get_bool)
  function getNumero(item, campo) {
    const ok = APELIDOS.indexOf(campo) === COL_ANO && item.ano !== null;
    return { ok, valor: ok ? item.ano : 0 };
  }
  function getTexto(item, campo) {
    const col = APELIDOS.indexOf(campo);
    if (col < 0 || col === COL_ANO) return null;
    return item.texto[col];
  }
  function getBool() { return { ok: false, valor: false }; }

  /* ===================================================================
   * Consulta (query.c)
   * =================================================================== */
  const OPERADORES = ['==', '!=', '<', '<=', '>', '>='];
  const MAX_FILTERS = 16;
  const MAX_SIMS = 16;
  const MAX_GROUP = 4;
  const CAMPO_MAX = 64;

  // strtod + checagem de que não sobrou nada além de espaços
  function converteNumero(t) {
    const m = /^[ \t\n\v\f\r]*([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)[ \t\n\v\f\r]*$/.exec(t);
    if (!m) return null;
    const v = Number(m[1]);
    return Number.isFinite(v) ? v : null;
  }
  function converteBool(t) {
    const s = t.toLowerCase();
    if (s === 'true' || s === '1') return true;
    if (s === 'false' || s === '0') return false;
    return null;
  }

  function queryVazia() {
    return {
      filtros: [], sims: [], aceite: 0,
      ordem: '', ordemDesc: false,
      grupo: [], cruz: [],
    };
  }

  /* carrega_query: devolve { ok, query } ou { ok:false, erro, linha }.
     strtok ignora delimitadores repetidos, por isso os tokens vazios saem. */
  function carregaQuery(texto) {
    const q = queryVazia();
    const linhas = texto.split('\n');
    const falha = (n, erro) => ({ ok: false, erro, linha: n + 1 });

    for (let n = 0; n < linhas.length; n++) {
      const cmd = apara(linhas[n]);
      if (cmd === '' || cmd[0] === '#') continue;
      const tk = cmd.split('|').filter((t) => t.length > 0);
      const nome = apara(tk[0] || '');

      if (nome === 'FILTER') {
        if (q.filtros.length >= MAX_FILTERS) return falha(n, 'filtros demais');
        if (tk.length !== 4) return falha(n, 'esperado FILTER|campo|operador|valor');
        const campo = apara(tk[1]);
        const op = apara(tk[2]);
        const valor = apara(tk[3]);
        if (bytes(campo) >= CAMPO_MAX || bytes(valor) >= CAMPO_MAX) return falha(n, 'texto longo demais (máx. 63 bytes)');
        if (!OPERADORES.includes(op)) return falha(n, `operador inválido "${op}"`);
        const num = converteNumero(valor);
        const bool = converteBool(valor);
        q.filtros.push({ campo, op, alvoTexto: valor, alvoNum: num === null ? NaN : num, ehBool: bool !== null, alvoBool: bool === true });
      } else if (nome === 'SIM') {
        if (q.sims.length >= MAX_SIMS) return falha(n, 'SIM demais');
        if (tk.length !== 5) return falha(n, 'esperado SIM|campo|alvo|tolerancia|peso');
        const campo = apara(tk[1]);
        const alvo = converteNumero(apara(tk[2]));
        const tol = converteNumero(apara(tk[3]));
        const peso = converteNumero(apara(tk[4]));
        if (bytes(campo) >= CAMPO_MAX || alvo === null || tol === null || peso === null || tol <= 0 || peso < 0) {
          return falha(n, 'SIM inválido (tolerância > 0 e peso >= 0)');
        }
        q.sims.push({ campo, alvo, tolerancia: tol, peso });
      } else if (nome === 'ACCEPT') {
        if (tk.length !== 2) return falha(n, 'esperado ACCEPT|limiar');
        const v = converteNumero(apara(tk[1]));
        if (v === null || v < 0 || v > 1) return falha(n, 'ACCEPT precisa estar entre 0 e 1');
        q.aceite = v;
      } else if (nome === 'ORDER') {
        if (tk.length !== 3) return falha(n, 'esperado ORDER|campo|ASC ou DESC');
        const campo = apara(tk[1]);
        const dir = apara(tk[2]);
        if (bytes(campo) >= CAMPO_MAX || (dir !== 'ASC' && dir !== 'DESC')) return falha(n, 'ORDER inválido (use ASC ou DESC)');
        q.ordem = campo;
        q.ordemDesc = dir === 'DESC';
      } else if (nome === 'GROUP') {
        const campos = [];
        for (const t of tk.slice(1)) {
          const c = apara(t);
          if (campos.length >= MAX_GROUP || c === '' || bytes(c) >= CAMPO_MAX) return falha(n, 'GROUP inválido (até 4 campos)');
          campos.push(c);
        }
        if (campos.length === 0) return falha(n, 'GROUP sem campos');
        q.grupo = campos;
      } else if (nome === 'CROSS') {
        if (tk.length !== 3 || bytes(tk[1]) >= CAMPO_MAX || bytes(tk[2]) >= CAMPO_MAX) return falha(n, 'esperado CROSS|campo1|campo2');
        q.cruz = [apara(tk[1]), apara(tk[2])];
      } else {
        return falha(n, `comando desconhecido "${nome}"`);
      }
    }
    return { ok: true, query: q };
  }

  // strcasecmp no locale "C": só A-Z viram minúsculas; o resto compara por código
  function strcasecmp(a, b) {
    const n = Math.min(a.length, b.length);
    for (let i = 0; i < n; i++) {
      let x = a.charCodeAt(i);
      let y = b.charCodeAt(i);
      if (x >= 65 && x <= 90) x += 32;
      if (y >= 65 && y <= 90) y += 32;
      if (x !== y) return x - y;
    }
    return a.length - b.length;
  }
  const strcmp = (a, b) => (a === b ? 0 : a < b ? -1 : 1);

  function compara(c, op) {
    switch (op) {
      case '==': return c === 0;
      case '!=': return c !== 0;
      case '<': return c < 0;
      case '<=': return c <= 0;
      case '>': return c > 0;
      case '>=': return c >= 0;
    }
    return false;
  }
  const comparaNumero = (v, alvo, op) => compara(v > alvo ? 1 : v < alvo ? -1 : (v === alvo ? 0 : NaN), op);

  // avalia_filtro, devolvendo também o que foi comparado (para a interface)
  function avaliaFiltro(f, item) {
    const b = getBool(item, f.campo);
    if (b.ok) {
      if (!f.ehBool) return { ok: false, valor: b.valor };
      return { ok: comparaNumero(b.valor ? 1 : 0, f.alvoBool ? 1 : 0, f.op), valor: b.valor };
    }
    const num = getNumero(item, f.campo);
    if (num.ok) {
      if (Number.isNaN(f.alvoNum)) return { ok: false, valor: num.valor, motivo: 'alvo não é número' };
      return { ok: comparaNumero(num.valor, f.alvoNum, f.op), valor: num.valor };
    }
    const t = getTexto(item, f.campo);
    if (t === null || t === '') return { ok: false, valor: null, motivo: 'campo vazio' };
    return { ok: compara(strcasecmp(t, f.alvoTexto), f.op), valor: t };
  }

  /* avalia_query: todos os FILTER precisam passar (para no primeiro que falha);
     com SIM, calcula a média ponderada e aplica o ACCEPT. */
  function avaliaQuery(q, item) {
    const passos = [];
    for (const f of q.filtros) {
      const r = avaliaFiltro(f, item);
      passos.push({ tipo: 'filtro', filtro: f, ...r });
      if (!r.ok) return { aceito: false, similaridade: 0, passos };
    }
    let somaPonderada = 0;
    let somaPesos = 0;
    let similaridade = 1;
    for (const s of q.sims) {
      const num = getNumero(item, s.campo);
      if (!num.ok) {
        passos.push({ tipo: 'sim', sim: s, ok: false, valor: null, motivo: 'campo sem número' });
        return { aceito: false, similaridade: 0, passos };
      }
      const diferenca = Math.abs(num.valor - s.alvo);
      let sim = 1 - diferenca / s.tolerancia;
      if (sim < 0) sim = 0;
      if (sim > 1) sim = 1;
      somaPonderada += sim * s.peso;
      somaPesos += s.peso;
      passos.push({ tipo: 'sim', sim: s, ok: true, valor: num.valor, diferenca, nota: sim });
    }
    if (q.sims.length > 0) {
      if (somaPesos <= 0) return { aceito: false, similaridade: 0, passos };
      similaridade = somaPonderada / somaPesos;
    }
    if (q.sims.length > 0 && similaridade < q.aceite) {
      passos.push({ tipo: 'aceite', ok: false, similaridade, limiar: q.aceite });
      return { aceito: false, similaridade, passos };
    }
    if (q.sims.length > 0) passos.push({ tipo: 'aceite', ok: true, similaridade, limiar: q.aceite });
    return { aceito: true, similaridade, passos };
  }

  /* ===================================================================
   * Estruturas
   * Todas têm: inserir(item) -> info, tamanho, iterar(), iterarAlt()
   * 'info.passos' = quantas operações a inserção custou (1 = só ligar o nó)
   * =================================================================== */

  // chave_do_item (lista_gen.c / lista_cruz.c): texto, número (%.15g) ou "N/A"
  function formataG15(v) {
    if (Number.isInteger(v) && Math.abs(v) < 1e15) return String(v);
    let s = v.toPrecision(15);
    if (s.includes('e')) {
      let [m, e] = s.split('e');
      if (m.includes('.')) m = m.replace(/0+$/, '').replace(/\.$/, '');
      const ex = parseInt(e, 10);
      return `${m}e${ex < 0 ? '-' : '+'}${String(Math.abs(ex)).padStart(2, '0')}`;
    }
    if (s.includes('.')) s = s.replace(/0+$/, '').replace(/\.$/, '');
    return s;
  }
  function chaveDoItem(item, campo) {
    const t = getTexto(item, campo);
    if (t !== null && t !== '') return t;
    const n = getNumero(item, campo);
    if (n.ok) return formataG15(n.valor);
    const b = getBool(item, campo);
    if (b.ok) return b.valor ? 'true' : 'false';
    return 'N/A';
  }

  // pilha.c — insere no topo; percorre do topo para baixo
  class Pilha {
    constructor() { this.topo = null; this.tamanho = 0; }
    inserir(item) {
      this.topo = { item, prox: this.topo };
      this.tamanho++;
      return { passos: 1, onde: 'topo' };
    }
    *iterar() { for (let a = this.topo; a; a = a.prox) yield a.item; }
    iterarAlt() { return this.iterar(); }
  }

  // fila.c / lista_enc.c — insere no fim; percorre do início
  class Encadeada {
    constructor() { this.inicio = null; this.fim = null; this.tamanho = 0; }
    inserir(item) {
      const novo = { item, prox: null };
      if (this.fim === null) this.inicio = novo; else this.fim.prox = novo;
      this.fim = novo;
      this.tamanho++;
      return { passos: 1, onde: 'fim' };
    }
    *iterar() { for (let a = this.inicio; a; a = a.prox) yield a.item; }
    iterarAlt() { return this.iterar(); }
  }
  class Fila extends Encadeada {}
  class ListaEnc extends Encadeada {}

  // lista_cabeca.c — nó sentinela sem dado no começo
  class ListaCabeca {
    constructor() {
      this.sentinela = { item: null, prox: null };
      this.fim = this.sentinela;
      this.tamanho = 0;
    }
    inserir(item) {
      const novo = { item, prox: null };
      this.fim.prox = novo;
      this.fim = novo;
      this.tamanho++;
      return { passos: 1, onde: 'fim' };
    }
    *iterar() { for (let a = this.sentinela.prox; a; a = a.prox) yield a.item; }
    iterarAlt() { return this.iterar(); }
  }

  // lista_seq.c — vetor que dobra de capacidade quando enche
  class ListaSeq {
    constructor() { this.itens = []; this.tamanho = 0; this.capacidade = 10; }
    inserir(item) {
      let passos = 1;
      let cresceu = null;
      if (this.tamanho === this.capacidade) {
        cresceu = { de: this.capacidade, para: this.capacidade * 2, copias: this.tamanho };
        passos += this.tamanho; // realloc copia os ponteiros antigos
        this.capacidade *= 2;
      }
      this.itens[this.tamanho] = item;
      this.tamanho++;
      return { passos, onde: this.tamanho - 1, cresceu };
    }
    *iterar() { for (let i = 0; i < this.tamanho; i++) yield this.itens[i]; }
    iterarAlt() { return this.iterar(); }
  }

  // lista_ord.c — inserção ordenada estável; sem chave vai para o final
  class ListaOrd {
    constructor(q) {
      this.inicio = null; this.fim = null; this.tamanho = 0;
      this.campo = 'similarity'; this.usaSimilaridade = true; this.desc = true;
      if (q && q.ordem !== '') {
        this.campo = q.ordem;
        this.usaSimilaridade = q.ordem === 'similarity';
        this.desc = q.ordemDesc;
      }
    }
    chaveDe(item) {
      if (this.usaSimilaridade) return { chave: item.similaridade, texto: null, tem: true };
      const n = getNumero(item, this.campo);
      if (n.ok) return { chave: n.valor, texto: null, tem: true };
      const t = getTexto(item, this.campo);
      if (t !== null && t !== '') return { chave: 0, texto: t, tem: true };
      return { chave: 0, texto: null, tem: false };
    }
    static comparaChaves(a, b) {
      if (a.texto !== null && b.texto !== null) return strcasecmp(a.texto, b.texto);
      if (a.texto === null && b.texto === null) return (a.chave > b.chave) - (a.chave < b.chave);
      return a.texto !== null ? 1 : -1;
    }
    vemAntes(a, b) {
      this._comparacoes++;
      if (!a.tem) return false;
      if (!b.tem) return true;
      const c = ListaOrd.comparaChaves(a, b);
      return this.desc ? c > 0 : c < 0;
    }
    inserir(item) {
      const novo = { item, prox: null, ...this.chaveDe(item) };
      this._comparacoes = 0;
      let posicao;
      let caminho = 'vazia';
      if (this.inicio === null) {
        this.inicio = this.fim = novo;
        posicao = 0;
      } else if (!this.vemAntes(novo, this.fim)) {
        this.fim.prox = novo; // caso rápido: entra no final
        this.fim = novo;
        posicao = this.tamanho;
        caminho = 'fim';
      } else if (this.vemAntes(novo, this.inicio)) {
        novo.prox = this.inicio;
        this.inicio = novo;
        posicao = 0;
        caminho = 'inicio';
      } else {
        let atual = this.inicio;
        posicao = 1;
        while (!this.vemAntes(novo, atual.prox)) { atual = atual.prox; posicao++; }
        novo.prox = atual.prox;
        atual.prox = novo;
        caminho = 'meio';
      }
      this.tamanho++;
      return { passos: 1 + this._comparacoes, comparacoes: this._comparacoes, onde: posicao, caminho, chave: novo.texto !== null ? novo.texto : novo.tem ? novo.chave : null };
    }
    *nos() { for (let a = this.inicio; a; a = a.prox) yield a; }
    *iterar() { for (let a = this.inicio; a; a = a.prox) yield a.item; }
    iterarAlt() { return this.iterar(); }
  }

  // lista_gen.c — sublistas aninhadas, uma por nível do GROUP
  class ListaGen {
    constructor(q) {
      this.raiz = { sub: true, chave: null, filhos: [], total: 0 };
      this.tamanho = 0;
      this.campos = q && q.grupo.length > 0 ? q.grupo.slice(0, 4) : [];
    }
    inserir(item) {
      let atual = this.raiz;
      let comparacoes = 0;
      const caminho = [];
      atual.total++;
      for (let nivel = 0; nivel < this.campos.length; nivel++) {
        const chave = chaveDoItem(item, this.campos[nivel]);
        let f = null;
        for (const filho of atual.filhos) {
          comparacoes++;
          if (filho.sub && strcmp(filho.chave, chave) === 0) { f = filho; break; }
        }
        const criada = f === null;
        if (criada) {
          f = { sub: true, chave, filhos: [], total: 0, nivel };
          atual.filhos.push(f);
        }
        f.total++;
        caminho.push({ chave, criada });
        atual = f;
      }
      atual.filhos.push({ sub: false, item });
      this.tamanho++;
      return { passos: 1 + comparacoes, comparacoes, caminho };
    }
    *iterar() {
      function* sub(no) {
        for (const f of no.filhos) {
          if (f.sub) yield* sub(f); else yield f.item;
        }
      }
      yield* sub(this.raiz);
    }
    iterarAlt() { return this.iterar(); }
  }

  // lista_cruz.c — cada item tem UM nó, ligado na sua linha e na sua coluna
  class ListaCruz {
    constructor(q) {
      this.linhas = []; this.colunas = []; this.tamanho = 0;
      this.campoLinha = ''; this.campoColuna = '';
      if (q && q.cruz.length === 2) { this.campoLinha = q.cruz[0]; this.campoColuna = q.cruz[1]; }
    }
    static achaOuCria(lista, chave) {
      let comparacoes = 0;
      for (const d of lista) {
        comparacoes++;
        if (strcmp(d.chave, chave) === 0) return { d, comparacoes, criada: false };
      }
      const d = { chave, nos: [] };
      lista.push(d);
      return { d, comparacoes, criada: true };
    }
    inserir(item) {
      const l = ListaCruz.achaOuCria(this.linhas, chaveDoItem(item, this.campoLinha));
      const c = ListaCruz.achaOuCria(this.colunas, chaveDoItem(item, this.campoColuna));
      const no = { item, linha: l.d, coluna: c.d };
      l.d.nos.push(no); // mesmo nó nas duas listas, sem duplicar o registro
      c.d.nos.push(no);
      this.tamanho++;
      const comparacoes = l.comparacoes + c.comparacoes;
      return { passos: 1 + comparacoes, comparacoes, linha: l.d.chave, coluna: c.d.chave, novaLinha: l.criada, novaColuna: c.criada };
    }
    *iterar() { for (const l of this.linhas) for (const n of l.nos) yield n.item; }
    *iterarAlt() { for (const c of this.colunas) for (const n of c.nos) yield n.item; }
  }

  const ESTRUTURAS = {
    pilha: Pilha,
    fila: Fila,
    lista_seq: ListaSeq,
    lista_enc: ListaEnc,
    lista_cabeca: ListaCabeca,
    lista_ord: ListaOrd,
    lista_gen: ListaGen,
    lista_cruz: ListaCruz,
  };
  const criaEstrutura = (tipo, q) => new ESTRUTURAS[tipo](q);

  /* ===================================================================
   * Execução (main.c): carga -> busca -> escrita, como uma sequência de
   * eventos. A interface consome um por um (animação) ou todos de uma vez.
   * =================================================================== */
  function* executar(base, cfg) {
    // cfg: { query (objeto já carregado), base, resultado, limite, topK, porColuna }
    const q = cfg.query;
    const eBase = criaEstrutura(cfg.base, q);
    const eRes = criaEstrutura(cfg.resultado, q);
    for (const it of base.itens) it.similaridade = 0;

    const st = { lidos: 0, casados: 0, saida: 0, passosBase: 0, passosResultado: 0 };
    yield { tipo: 'inicio', base: eBase, resultado: eRes, st };

    for (const item of base.itens) {
      const info = eBase.inserir(item);
      st.lidos++;
      st.passosBase += info.passos;
      yield { tipo: 'carga', item, info, st };
      if (cfg.limite > 0 && st.lidos >= cfg.limite) break;
    }

    for (const item of eBase.iterar()) {
      const av = avaliaQuery(q, item);
      // filtro opcional que só existe no site (busca por trecho do nome)
      if (av.aceito && cfg.filtroExtra) {
        const ok = cfg.filtroExtra.teste(item);
        av.passos.push({ tipo: 'extra', ok, descricao: cfg.filtroExtra.descricao });
        if (!ok) av.aceito = false;
      }
      yield { tipo: 'avalia', item, av, st };
      if (av.aceito) {
        item.similaridade = av.similaridade;
        const info = eRes.inserir(item);
        st.casados++;
        st.passosResultado += info.passos;
        yield { tipo: 'insere', item, info, st };
      }
    }

    const saida = [];
    const percurso = cfg.porColuna ? eRes.iterarAlt() : eRes.iterar();
    for (const item of percurso) {
      if (cfg.topK > 0 && st.saida >= cfg.topK) break;
      saida.push(item);
      st.saida++;
      yield { tipo: 'escreve', item, n: st.saida, st };
    }
    yield { tipo: 'fim', saida, st, base: eBase, resultado: eRes };
  }

  // Executa tudo de uma vez e devolve o último evento
  function executarTudo(base, cfg) {
    let ultimo = null;
    for (const ev of executar(base, cfg)) ultimo = ev;
    return ultimo;
  }

  // Monta o CSV de saída exatamente como o main.c escreve
  function textoSaida(base, saida) {
    let s = base.cabecalho + base.fimDeLinha;
    for (const it of saida) s += it.linha + base.fimDeLinha;
    return s;
  }

  const Motor = {
    APELIDOS, COL_ANO, lerCSV, getTexto, getNumero, carregaQuery, avaliaQuery,
    criaEstrutura, executar, executarTudo, textoSaida, chaveDoItem, strcasecmp,
    NOMES_ESTRUTURAS: Object.keys(ESTRUTURAS),
  };
  if (typeof module !== 'undefined' && module.exports) module.exports = Motor;
  raiz.Motor = Motor;
})(typeof window !== 'undefined' ? window : globalThis);
