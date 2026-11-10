/* app.js — interface: monta a consulta, roda o motor e desenha mapa, lista,
 * animação das estruturas, corrida e desafios. */
(function () {
  'use strict';

  const $ = (s, r = document) => r.querySelector(s);
  const $$ = (s, r = document) => [...r.querySelectorAll(s)];
  const { INFO, esc, curto, nomeDo, plural } = window.Viz;
  const Motor = window.Motor;
  const NF = new Intl.NumberFormat('pt-BR');
  const dec = (v, d = 2) => v.toLocaleString('pt-BR', { minimumFractionDigits: d, maximumFractionDigits: d });
  const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
  const guarda = {
    ler(k) { try { return localStorage.getItem('patrimonio.' + k); } catch (e) { return null; } },
    gravar(k, v) { try { localStorage.setItem('patrimonio.' + k, v); } catch (e) { /* sem armazenamento */ } },
  };

  /* ================================================================
   * Dados
   * ================================================================ */
  const BASE = Motor.lerCSV(window.PATRIMONIO_CSV);
  const ITENS = BASE.itens;
  const COL = Object.fromEntries(Motor.APELIDOS.map((a, i) => [a, i]));
  const campo = (it, nome) => it.texto[COL[nome]];
  const normaliza = (s) => s.normalize('NFD').replace(/[\u0300-\u036f]/g, '').toLowerCase()
    .replace(/[-'’]/g, ' ').replace(/\s+/g, ' ').trim();
  const codigoDe = (nome) => (nome ? window.NOME_PARA_IBGE[normaliza(nome)] || null : null);

  for (const it of ITENS) {
    it.cod = codigoDe(campo(it, 'municipio'));
    it.cidades = [];
    const cid = campo(it, 'cidades');
    if (cid && cid !== 'Não se aplica') {
      for (const parte of cid.split(';')) {
        const c = codigoDe(parte.split(/\s[–-]\s/)[0]);
        if (c && !it.cidades.includes(c)) it.cidades.push(c);
      }
    }
    it.nomeNorm = normaliza(nomeDo(it));
  }
  const anos = ITENS.map((i) => i.ano).filter((a) => a !== null);
  const ANO_MIN = Math.min(...anos);
  const ANO_MAX = Math.max(...anos);
  const MUN = new Map(window.MAPA_SP.municipios.map((m) => [m.c, m]));

  // quantos bens cada município tem na base inteira e qual grafia usar no FILTER
  const NA_BASE = new Map();
  const GRAFIAS = new Map();
  for (const it of ITENS) {
    if (!it.cod) continue;
    NA_BASE.set(it.cod, (NA_BASE.get(it.cod) || 0) + 1);
    const g = GRAFIAS.get(it.cod) || new Map();
    const nome = campo(it, 'municipio');
    g.set(nome, (g.get(nome) || 0) + 1);
    GRAFIAS.set(it.cod, g);
  }
  const nomeFiltroDe = (cod) => {
    const g = GRAFIAS.get(cod);
    return g ? [...g.entries()].sort((a, b) => b[1] - a[1])[0][0] : null;
  };

  const ROTULO_CAMPO = {
    ano_tombamento: 'Ano do tombamento',
    bem: 'Nome do bem',
    municipio: 'Município',
    regiao_administrativa: 'Região administrativa',
    tipo_protecao: 'Tipo de proteção',
    livro_tombo: 'Livro do Tombo',
    tipo_area_envoltoria: 'Área envoltória',
    similarity: 'Nota de similaridade',
  };
  const CAMPOS_ORDEM = ['ano_tombamento', 'bem', 'municipio', 'regiao_administrativa', 'tipo_protecao', 'livro_tombo'];
  const CAMPOS_GRUPO = ['tipo_protecao', 'livro_tombo', 'regiao_administrativa', 'municipio', 'ano_tombamento', 'tipo_area_envoltoria'];
  const TIPOS = [
    { v: '', rot: 'Todos' },
    { v: 'T', rot: 'T · Tombamento', dica: 'Tombado por uma resolução do governo do estado.' },
    { v: 'TEO', rot: 'TEO · Ex officio', dica: 'Tombamento ex officio: o estado tombou automaticamente um bem que já era protegido pelo governo federal (IPHAN).' },
    { v: 'RVG', rot: 'RVG', dica: 'Sigla usada na planilha original do CONDEPHAAT.' },
  ];

  /* ================================================================
   * Estado da busca -> arquivo de consulta
   * ================================================================ */
  const PADRAO = () => ({
    municipio: '', regiao: '', tipo: '', livro: '', anoMin: ANO_MIN, anoMax: ANO_MAX, nome: '',
    simAtivo: false, simAlvo: 1975, simTol: 10, simAceite: 0.5,
    resultado: 'lista_ord', ordemCampo: 'ano_tombamento', ordemDesc: false,
    grupo1: 'tipo_protecao', grupo2: 'municipio', cruzLinha: 'tipo_protecao', cruzColuna: 'livro_tombo', porColuna: false,
    topK: 10, base: 'fila', limite: 0,
  });
  let E = PADRAO();
  let manual = null; // texto da consulta quando editada à mão

  function textoConsulta(s, { todas = false } = {}) {
    const L = ['# Consulta montada no site Patrimônio Paulista'];
    if (s.municipio) L.push(`FILTER|municipio|==|${s.municipio}`);
    if (s.regiao) L.push(`FILTER|regiao_administrativa|==|${s.regiao}`);
    if (s.tipo) L.push(`FILTER|tipo_protecao|==|${s.tipo}`);
    if (s.livro) L.push(`FILTER|livro_tombo|==|${s.livro}`);
    if (s.anoMin > ANO_MIN) L.push(`FILTER|ano_tombamento|>=|${s.anoMin}`);
    if (s.anoMax < ANO_MAX) L.push(`FILTER|ano_tombamento|<=|${s.anoMax}`);
    if (s.simAtivo) {
      L.push(`SIM|ano_tombamento|${s.simAlvo}|${s.simTol}|1`);
      L.push(`ACCEPT|${Number(s.simAceite.toFixed(2))}`);
    }
    const usa = (t) => todas || s.resultado === t || s.base === t;
    if (usa('lista_ord')) L.push(`ORDER|${s.ordemCampo}|${s.ordemDesc ? 'DESC' : 'ASC'}`);
    if (usa('lista_gen')) L.push(`GROUP|${[s.grupo1, s.grupo2].filter(Boolean).join('|')}`);
    if (usa('lista_cruz')) L.push(`CROSS|${s.cruzLinha}|${s.cruzColuna}`);
    if (s.nome.trim()) L.push(`# extra do site (o programa em C ignora): nome contém "${s.nome.trim()}"`);
    return L.join('\n') + '\n';
  }

  function textoComando(s) {
    let c = './trabalho --reader patrimonio --input dados/patrimonio.csv \\\n' +
      `  --base ${s.base} --result ${s.resultado} \\\n` +
      `  --query minha_consulta.txt --output saida.csv --top-k ${s.topK}`;
    if (s.limite > 0) c += ` --limit ${s.limite}`;
    if (s.resultado === 'lista_cruz') c += ` --view ${s.porColuna ? 'col' : 'row'}`;
    return c;
  }

  function filtroExtraDe(s) {
    const termo = s.nome.trim();
    if (!termo || manual !== null) return null;
    const n = normaliza(termo);
    return { teste: (it) => it.nomeNorm.includes(n), descricao: `nome contém “${termo}”` };
  }

  function configuracao(s, texto) {
    const r = Motor.carregaQuery(texto);
    if (!r.ok) return { erro: r };
    return {
      query: r.query, base: s.base, resultado: s.resultado, limite: s.limite,
      topK: s.topK, porColuna: s.porColuna, filtroExtra: filtroExtraDe(s),
    };
  }

  /* ================================================================
   * Execução
   * ================================================================ */
  let ULTIMO = null; // { cfg, base, resultado, saida, st, casados, sim }
  let abaAtual = 'mapa';

  function executar() {
    const texto = manual ?? textoConsulta(E);
    const cfg = configuracao(E, texto);
    if (cfg.erro) { ULTIMO = { erro: cfg.erro }; return; }
    const fim = Motor.executarTudo(BASE, cfg);
    const casados = [...fim.resultado.iterar()];
    ULTIMO = {
      cfg, base: fim.base, resultado: fim.resultado, saida: fim.saida, st: fim.st, casados,
      sim: new Map(casados.map((it) => [it.id, it.similaridade])),
    };
  }

  function atualizar() {
    executar();
    renderConsulta();
    renderPlacar();
    renderMapaDados();
    renderLista();
    pararAnimacao(true);
    if (abaAtual === 'estruturas') renderEstruturas();
    if (abaAtual === 'corrida') renderCorrida();
  }

  /* ================================================================
   * Painel da busca
   * ================================================================ */
  const opcoes = (lista, sel) => lista.map((o) => `<option value="${esc(o.v)}"${o.v === sel ? ' selected' : ''}>${esc(o.rot)}</option>`).join('');

  function preencheControles() {
    // municípios (sem diferenciar maiúsculas, como o strcasecmp do C)
    const mun = new Map();
    for (const it of ITENS) {
      const v = campo(it, 'municipio');
      if (!v) continue;
      const k = v.toLowerCase();
      const m = mun.get(k) || { n: 0, grafias: new Map() };
      m.n++;
      m.grafias.set(v, (m.grafias.get(v) || 0) + 1);
      mun.set(k, m);
    }
    const listaMun = [...mun.values()].map((m) => {
      const v = [...m.grafias.entries()].sort((a, b) => b[1] - a[1])[0][0];
      return { v, rot: `${v} (${m.n})` };
    }).sort((a, b) => a.v.localeCompare(b.v, 'pt-BR'));
    $('#f-municipio').innerHTML = opcoes([{ v: '', rot: `Todos os municípios` }, ...listaMun], '');

    const contagem = (col) => {
      const m = new Map();
      for (const it of ITENS) { const v = campo(it, col); if (v) m.set(v, (m.get(v) || 0) + 1); }
      return [...m.entries()].sort((a, b) => a[0].localeCompare(b[0], 'pt-BR')).map(([v, n]) => ({ v, rot: `${v} (${n})` }));
    };
    $('#f-regiao').innerHTML = opcoes([{ v: '', rot: 'Todas as regiões' }, ...contagem('regiao_administrativa')], '');
    $('#f-livro').innerHTML = opcoes([{ v: '', rot: 'Todos os livros' }, ...contagem('livro_tombo')], '');

    $('#f-tipo').innerHTML = TIPOS.map((t) => {
      const n = t.v ? ITENS.filter((i) => campo(i, 'tipo_protecao') === t.v).length : ITENS.length;
      const cor = t.v ? `<span class="marca-tipo" style="background:var(--tipo-${t.v})"></span>` : '';
      return `<button type="button" class="chip" role="radio" aria-checked="false" data-tipo="${t.v}" ${t.dica ? `data-dica="${esc(t.dica)}"` : ''}>${cor}${esc(t.rot)} <span class="sub">${n}</span></button>`;
    }).join('');

    for (const id of ['#f-ano-min', '#f-ano-max', '#s-alvo']) {
      $(id).min = ANO_MIN;
      $(id).max = ANO_MAX;
    }
    $('#limite').max = ITENS.length;
    $('#sel-resultado').innerHTML = Motor.NOMES_ESTRUTURAS.map((t) =>
      `<button type="button" class="estrutura-btn" role="radio" aria-checked="false" data-estrutura="${t}" data-dica="${esc(INFO[t].regra)}">${window.Viz.icone(t)}<span>${INFO[t].curto}</span></button>`).join('');
    $('#sel-base').innerHTML = opcoes(Motor.NOMES_ESTRUTURAS.map((t) => ({ v: t, rot: `${INFO[t].nome} (${t})` })), E.base);
    $('#total-bens').textContent = NF.format(ITENS.length);
  }

  // coloca os valores do estado E nos controles
  function sincronizaControles() {
    $('#f-municipio').value = E.municipio;
    $('#f-regiao').value = E.regiao;
    $('#f-livro').value = E.livro;
    $$('#f-tipo .chip').forEach((b) => b.setAttribute('aria-checked', String(b.dataset.tipo === E.tipo)));
    $('#f-ano-min').value = E.anoMin;
    $('#f-ano-max').value = E.anoMax;
    $('#f-ano-min-v').textContent = E.anoMin;
    $('#f-ano-max-v').textContent = E.anoMax;
    $('#f-nome').value = E.nome;
    $('#s-ativo').checked = E.simAtivo;
    $('#s-alvo').value = E.simAlvo;
    $('#s-tol').value = E.simTol;
    $('#s-aceite').value = E.simAceite;
    $('#s-alvo-v').textContent = E.simAlvo;
    $('#s-tol-v').textContent = plural(E.simTol, 'ano', 'anos');
    $('#s-aceite-v').textContent = dec(E.simAceite);
    $('#s-controles').classList.toggle('desligado', !E.simAtivo);
    $$('#sel-resultado .estrutura-btn').forEach((b) => b.setAttribute('aria-checked', String(b.dataset.estrutura === E.resultado)));
    $('#topk').value = String(E.topK);
    $('#sel-base').value = E.base;
    $('#limite').value = E.limite;
    $('#limite-v').textContent = E.limite ? E.limite : 'todos';
    $('#a-poucos').textContent = E.limite ? 'Usar todos os bens' : 'Usar só 15 bens';
    renderOrg();
    $$('.consulta-campo').forEach((f) => { f.disabled = manual !== null; });
  }

  function renderOrg() {
    const t = E.resultado;
    const travado = manual !== null ? ' disabled' : '';
    const camposOrdem = E.simAtivo ? [...CAMPOS_ORDEM, 'similarity'] : CAMPOS_ORDEM;
    const op = (lista, sel, vazio) => (vazio ? `<option value="">${vazio}</option>` : '') +
      lista.map((c) => `<option value="${c}"${c === sel ? ' selected' : ''}>${ROTULO_CAMPO[c]}</option>`).join('');
    let html = `<p class="analogia">${INFO[t].analogia}</p>`;
    if (t === 'lista_ord') {
      html += `<label class="campo"><span class="rot">Ordenar por <code>ORDER</code></span><select id="o-campo"${travado}>${op(camposOrdem, E.ordemCampo)}</select></label>` +
        `<div class="campo botoes-direcao" role="group" aria-label="Direção">` +
        `<button type="button" class="botao-mini" data-dir="asc" aria-pressed="${!E.ordemDesc}"${travado}>↑ crescente (ASC)</button>` +
        `<button type="button" class="botao-mini" data-dir="desc" aria-pressed="${E.ordemDesc}"${travado}>↓ decrescente (DESC)</button></div>`;
    } else if (t === 'lista_gen') {
      html += `<label class="campo"><span class="rot">Agrupar por <code>GROUP</code></span><select id="g-1"${travado}>${op(CAMPOS_GRUPO, E.grupo1)}</select></label>` +
        `<label class="campo"><span class="rot">e dentro de cada grupo, por</span><select id="g-2"${travado}>${op(CAMPOS_GRUPO.filter((c) => c !== E.grupo1), E.grupo2, '— só um nível —')}</select></label>`;
    } else if (t === 'lista_cruz') {
      html += `<label class="campo"><span class="rot">Linhas <code>CROSS</code></span><select id="c-l"${travado}>${op(CAMPOS_GRUPO, E.cruzLinha)}</select></label>` +
        `<label class="campo"><span class="rot">Colunas</span><select id="c-c"${travado}>${op(CAMPOS_GRUPO, E.cruzColuna)}</select></label>` +
        `<div class="campo"><span class="rot">Percorrer por <code>--view</code></span><div class="botoes-direcao" role="group">` +
        `<button type="button" class="botao-mini" data-view="row" aria-pressed="${!E.porColuna}">linha →</button>` +
        `<button type="button" class="botao-mini" data-view="col" aria-pressed="${E.porColuna}">coluna ↓</button></div></div>`;
    } else {
      html += `<p class="analogia"><b>Ordem do resultado:</b> ${INFO[t].ordem}.</p>`;
    }
    $('#org-detalhe').innerHTML = html;
  }

  function mudar(parcial) {
    Object.assign(E, parcial);
    if (E.anoMin > E.anoMax) {
      if ('anoMin' in parcial) E.anoMax = E.anoMin; else E.anoMin = E.anoMax;
    }
    sincronizaControles();
    atualizar();
  }

  function ligaControles() {
    $('#f-municipio').addEventListener('change', (e) => mudar({ municipio: e.target.value }));
    $('#f-regiao').addEventListener('change', (e) => mudar({ regiao: e.target.value }));
    $('#f-livro').addEventListener('change', (e) => mudar({ livro: e.target.value }));
    $('#f-tipo').addEventListener('click', (e) => {
      const b = e.target.closest('.chip');
      if (b) mudar({ tipo: b.dataset.tipo });
    });
    $('#f-ano-min').addEventListener('input', (e) => mudar({ anoMin: Number(e.target.value) }));
    $('#f-ano-max').addEventListener('input', (e) => mudar({ anoMax: Number(e.target.value) }));
    let tNome;
    $('#f-nome').addEventListener('input', (e) => {
      clearTimeout(tNome);
      tNome = setTimeout(() => mudar({ nome: e.target.value }), 150);
    });
    $('#s-ativo').addEventListener('change', (e) => ligaSim(e.target.checked));
    $('#s-alvo').addEventListener('input', (e) => mudarSim({ simAlvo: Number(e.target.value) }));
    $('#s-tol').addEventListener('input', (e) => mudarSim({ simTol: Number(e.target.value) }));
    $('#s-aceite').addEventListener('input', (e) => mudarSim({ simAceite: Number(e.target.value) }));
    $('#sel-resultado').addEventListener('click', (e) => {
      const b = e.target.closest('.estrutura-btn');
      if (b) mudar({ resultado: b.dataset.estrutura });
    });
    $('#org-detalhe').addEventListener('change', (e) => {
      const id = e.target.id;
      if (id === 'o-campo') mudar({ ordemCampo: e.target.value });
      if (id === 'g-1') mudar({ grupo1: e.target.value, grupo2: E.grupo2 === e.target.value ? '' : E.grupo2 });
      if (id === 'g-2') mudar({ grupo2: e.target.value });
      if (id === 'c-l') mudar({ cruzLinha: e.target.value });
      if (id === 'c-c') mudar({ cruzColuna: e.target.value });
    });
    $('#org-detalhe').addEventListener('click', (e) => {
      const b = e.target.closest('button');
      if (!b) return;
      if (b.dataset.dir) mudar({ ordemDesc: b.dataset.dir === 'desc' });
      if (b.dataset.view) mudar({ porColuna: b.dataset.view === 'col' });
    });
    $('#topk').addEventListener('change', (e) => mudar({ topK: Number(e.target.value) }));
    $('#sel-base').addEventListener('change', (e) => mudar({ base: e.target.value }));
    $('#limite').addEventListener('input', (e) => mudar({ limite: Number(e.target.value) }));
    $('#a-poucos').addEventListener('click', () => mudar({ limite: E.limite ? 0 : 15 }));
    $('#limpar').addEventListener('click', () => {
      manual = null;
      E = { ...PADRAO(), base: E.base };
      mostraManual(false);
      sincronizaControles();
      atualizar();
    });

    $('#q-copiar').addEventListener('click', (e) => copiar(manual ?? textoConsulta(E), e.currentTarget));
    $('#cmd-copiar').addEventListener('click', (e) => copiar(textoComando(E).replace(/\\\n\s*/g, ''), e.currentTarget));
    $('#q-baixar').addEventListener('click', () => baixar('minha_consulta.txt', manual ?? textoConsulta(E)));
    $('#q-editar').addEventListener('click', () => {
      if (manual === null) {
        manual = textoConsulta(E).split('\n').filter((l) => !l.startsWith('# extra do site')).join('\n');
        mostraManual(true);
      } else {
        manual = null;
        mostraManual(false);
      }
      sincronizaControles();
      atualizar();
    });
    $('#q-manual').addEventListener('input', (e) => { manual = e.target.value; atualizar(); });
  }

  function ligaSim(ativo) {
    const p = { simAtivo: ativo };
    if (ativo && E.resultado === 'lista_ord') { p.ordemCampo = 'similarity'; p.ordemDesc = true; }
    if (!ativo && E.ordemCampo === 'similarity') { p.ordemCampo = 'ano_tombamento'; p.ordemDesc = false; }
    mudar(p);
  }
  function mudarSim(p) {
    if (!E.simAtivo) {
      p.simAtivo = true;
      if (E.resultado === 'lista_ord') { p.ordemCampo = 'similarity'; p.ordemDesc = true; }
    }
    mudar(p);
  }

  function mostraManual(sim) {
    $('#q-manual').hidden = !sim;
    $('#q-texto').hidden = sim;
    $('#q-editar').textContent = sim ? 'Voltar ao construtor' : 'Editar à mão';
    $('#q-editar').setAttribute('aria-pressed', String(sim));
    if (sim) { $('#q-manual').value = manual; $('#q-manual').focus(); }
  }

  function realcaConsulta(texto) {
    return texto.replace(/\n$/, '').split('\n').map((l) => {
      if (l.trim().startsWith('#')) return `<span class="q-com">${esc(l)}</span>`;
      const p = l.split('|');
      if (p.length < 2) return esc(l);
      const cls = (i) => (i === 0 ? 'q-campo' : i === 1 && p[0].trim() === 'FILTER' ? 'q-op' : 'q-val');
      return `<span class="q-cmd">${esc(p[0])}</span>` + p.slice(1).map((x, i) => `<span class="q-sep">|</span><span class="${cls(i)}">${esc(x)}</span>`).join('');
    }).join('\n');
  }

  function renderConsulta() {
    if (manual === null) $('#q-texto').innerHTML = realcaConsulta(textoConsulta(E));
    $('#q-comando').textContent = textoComando(E);
    const st = $('#q-status');
    if (ULTIMO && ULTIMO.erro) {
      st.className = 'status erro';
      st.textContent = `STATUS=ERROR · linha ${ULTIMO.erro.linha}: ${ULTIMO.erro.erro}`;
    } else {
      st.className = 'status ok';
      st.textContent = manual !== null ? 'STATUS=OK · consulta válida' : 'STATUS=OK';
    }
  }

  async function copiar(texto, botao) {
    let ok = false;
    try { await navigator.clipboard.writeText(texto); ok = true; } catch (e) {
      const t = document.createElement('textarea');
      t.value = texto;
      document.body.append(t);
      t.select();
      try { ok = document.execCommand('copy'); } catch (e2) { ok = false; }
      t.remove();
    }
    const antes = botao.textContent;
    botao.textContent = ok ? 'Copiado!' : 'Não deu';
    setTimeout(() => { botao.textContent = antes; }, 1300);
  }
  function baixar(nome, texto) {
    const url = URL.createObjectURL(new Blob([texto], { type: 'text/plain;charset=utf-8' }));
    const a = Object.assign(document.createElement('a'), { href: url, download: nome });
    document.body.append(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  }

  /* ================================================================
   * Placar
   * ================================================================ */
  function renderPlacar() {
    const el = $('#placar');
    if (!ULTIMO || ULTIMO.erro) {
      el.innerHTML = `<div class="tile"><span class="rot">A consulta tem um erro</span><span class="valor">—</span><code>STATUS=ERROR</code></div>`;
      return;
    }
    const st = ULTIMO.st;
    el.innerHTML =
      `<div class="tile"><span class="rot">Bens lidos do arquivo</span><span class="valor">${NF.format(st.lidos)}</span><code>INPUT_RECORDS</code></div>` +
      `<span class="seta-placar" aria-hidden="true">→</span>` +
      `<div class="tile destaque"><span class="rot">Passaram na busca</span><span class="valor">${NF.format(st.casados)}</span><code>MATCHED_RECORDS</code></div>` +
      `<span class="seta-placar" aria-hidden="true">→</span>` +
      `<div class="tile"><span class="rot">Mostrados (top-k)</span><span class="valor">${NF.format(st.saida)}</span><code>OUTPUT_RECORDS</code></div>`;
    const aviso = $('#aviso-limite');
    aviso.hidden = !(E.limite > 0);
    if (E.limite > 0) {
      aviso.innerHTML = `<span>Atenção: o programa está lendo só os primeiros <b>${E.limite}</b> bens do arquivo (<code>--limit ${E.limite}</code>).</span><button class="botao-mini" id="tirar-limite">Ler todos</button>`;
      $('#tirar-limite').onclick = () => mudar({ limite: 0 });
    }
  }

  /* ================================================================
   * Mapa
   * ================================================================ */
  const NS = 'http://www.w3.org/2000/svg';
  const W = window.MAPA_SP.largura;
  const H = window.MAPA_SP.altura;
  const svg = $('#mapa');
  const area = $('#mapa-area');
  const novo = (tag, attrs = {}) => {
    const e = document.createElementNS(NS, tag);
    for (const [k, v] of Object.entries(attrs)) e.setAttribute(k, v);
    return e;
  };
  const gMun = novo('g');
  const gSel = novo('g');
  const gRealce = novo('g');
  const gBolhas = novo('g');
  let vb = { x: 0, y: 0, w: W, h: H };
  let contagem = new Map();

  function montaMapa() {
    svg.setAttribute('viewBox', `0 0 ${W} ${H}`);
    area.style.aspectRatio = `${W} / ${H}`;
    const frag = document.createDocumentFragment();
    for (const m of window.MAPA_SP.municipios) {
      frag.append(novo('path', { d: m.d, class: NA_BASE.has(m.c) ? 'mun tem' : 'mun', 'data-cod': m.c }));
    }
    gMun.append(frag);
    svg.append(gMun, gSel, gRealce, gBolhas);

    // legenda das bolhas
    const leg = [1, 10, 100].map((n) => {
      const r = raioPx(n);
      return `<span class="legenda-item"><svg width="${2 * r + 4}" height="${2 * r + 4}" aria-hidden="true"><circle cx="${r + 2}" cy="${r + 2}" r="${r}"/></svg>${n}</span>`;
    }).join('');
    $('#legenda-bolhas').innerHTML = `<span>Bens encontrados:</span>${leg}`;
  }

  const raioPx = (n) => Math.max(4, 2.3 * Math.sqrt(n));
  const pxPorUnidade = () => (svg.clientWidth || 640) / vb.w;

  function aplicaVB() {
    svg.setAttribute('viewBox', `${vb.x} ${vb.y} ${vb.w} ${vb.h}`);
    desenhaBolhas();
  }
  function zoom(fator, cx = vb.x + vb.w / 2, cy = vb.y + vb.h / 2) {
    const nw = clamp(vb.w / fator, W / 60, W);
    const k = nw / vb.w;
    vb = { x: cx - (cx - vb.x) * k, y: cy - (cy - vb.y) * k, w: nw, h: (nw * H) / W };
    limitaVB();
    aplicaVB();
  }
  function limitaVB() {
    vb.x = clamp(vb.x, -vb.w / 2, W - vb.w / 2);
    vb.y = clamp(vb.y, -vb.h / 2, H - vb.h / 2);
  }
  function centraEm(cod, largura) {
    const m = MUN.get(cod);
    vb = { w: largura, h: (largura * H) / W, x: m.x - largura / 2, y: m.y - (largura * H) / W / 2 };
    limitaVB();
    aplicaVB();
  }

  function renderMapaDados() {
    contagem = new Map();
    const fora = new Map();
    if (ULTIMO && !ULTIMO.erro) {
      for (const it of ULTIMO.casados) {
        if (it.cod) contagem.set(it.cod, (contagem.get(it.cod) || 0) + 1);
        else {
          const v = campo(it, 'municipio') || '(vazio)';
          const k = v.toLowerCase();
          const f = fora.get(k) || { rot: v, n: 0 };
          f.n++;
          fora.set(k, f);
        }
      }
    }
    // município selecionado no filtro
    gSel.innerHTML = '';
    const selCod = codigoDe(E.municipio);
    if (selCod && manual === null) gSel.append(novo('path', { d: MUN.get(selCod).d, class: 'mun-sel' }));
    desenhaBolhas();

    const lista = [...fora.values()];
    $('#fora-mapa').innerHTML = lista.length
      ? `<span data-dica="Bens móveis (acervos, imagens) não têm endereço fixo. Os “Diversos” estão espalhados por várias cidades: clique num deles na lista para ver as cidades no mapa.">Fora do mapa:</span> ` +
        lista.map((f) => `<span class="pilula">${esc(f.rot)}: ${f.n}</span>`).join('')
      : '';
  }

  function desenhaBolhas() {
    gBolhas.innerHTML = '';
    const px = pxPorUnidade();
    const frag = document.createDocumentFragment();
    const ordem = [...contagem.entries()].sort((a, b) => b[1] - a[1]);
    for (const [cod, n] of ordem) {
      const m = MUN.get(cod);
      const r = raioPx(n);
      frag.append(novo('circle', { cx: m.x, cy: m.y, r: r / px, class: 'bolha', 'data-cod': cod }));
      if (r >= 10) {
        const t = novo('text', { x: m.x, y: m.y, class: 'bolha-txt', 'font-size': clamp(r * 0.72, 9, 15) / px });
        t.textContent = n;
        frag.append(t);
      }
    }
    gBolhas.append(frag);
  }

  function realcaCidades(cods) {
    gRealce.innerHTML = '';
    for (const c of cods) {
      const m = MUN.get(c);
      if (m) gRealce.append(novo('path', { d: m.d, class: 'mun-realce' }));
    }
  }
  const cidadesDo = (it) => (it.cod ? [it.cod, ...it.cidades.filter((c) => c !== it.cod)] : it.cidades);

  function ligaMapa() {
    let arrasto = null;
    let arrastou = false;
    area.addEventListener('pointerdown', (e) => {
      if (e.button !== 0) return;
      arrasto = { x: e.clientX, y: e.clientY, vb: { ...vb }, id: e.pointerId };
      arrastou = false;
    });
    area.addEventListener('pointermove', (e) => {
      if (arrasto) {
        const dx = e.clientX - arrasto.x;
        const dy = e.clientY - arrasto.y;
        if (!arrastou && Math.hypot(dx, dy) > 5) {
          arrastou = true;
          area.classList.add('arrastando');
          area.setPointerCapture(arrasto.id);
          escondeDica();
        }
        if (arrastou) {
          const k = vb.w / svg.clientWidth;
          vb = { ...vb, x: arrasto.vb.x - dx * k, y: arrasto.vb.y - dy * k };
          limitaVB();
          aplicaVB();
          return;
        }
      }
      if (e.pointerType === 'mouse') dicaMapa(e);
    });
    const solta = () => {
      arrasto = null;
      area.classList.remove('arrastando');
      setTimeout(() => { arrastou = false; }, 0);
    };
    area.addEventListener('pointerup', solta);
    area.addEventListener('pointercancel', solta);
    area.addEventListener('pointerleave', () => { if (!arrasto) escondeDica(); });
    area.addEventListener('wheel', (e) => {
      e.preventDefault();
      const r = svg.getBoundingClientRect();
      const cx = vb.x + ((e.clientX - r.left) / r.width) * vb.w;
      const cy = vb.y + ((e.clientY - r.top) / r.height) * vb.h;
      zoom(e.deltaY < 0 ? 1.3 : 1 / 1.3, cx, cy);
    }, { passive: false });
    svg.addEventListener('click', (e) => {
      if (arrastou) return;
      const alvo = e.target.closest('[data-cod]');
      if (!alvo) return;
      const cod = alvo.dataset.cod;
      if (!NA_BASE.has(cod)) {
        mostraDica(`<b>${esc(MUN.get(cod).n)}</b>Nenhum bem tombado desta cidade na base.`, e.clientX, e.clientY);
        return;
      }
      if (manual !== null) return;
      const nome = nomeFiltroDe(cod);
      mudar({ municipio: codigoDe(E.municipio) === cod ? '' : nome });
    });
    $('#z-mais').onclick = () => zoom(1.6);
    $('#z-menos').onclick = () => zoom(1 / 1.6);
    $('#z-tudo').onclick = () => { vb = { x: 0, y: 0, w: W, h: H }; aplicaVB(); };
    $('#z-gsp').onclick = () => centraEm('3550308', W / 7);
    new ResizeObserver(() => desenhaBolhas()).observe(area);
  }

  function dicaMapa(e) {
    const alvo = e.target.closest && e.target.closest('[data-cod]');
    if (!alvo) { escondeDica(); return; }
    const cod = alvo.dataset.cod;
    const m = MUN.get(cod);
    const naBase = NA_BASE.get(cod) || 0;
    const naBusca = contagem.get(cod) || 0;
    const linhas = naBase
      ? `${plural(naBase, 'bem tombado', 'bens tombados')} na base<br>${plural(naBusca, 'passou', 'passaram')} na sua busca` +
        (manual === null ? `<br><i>clique para ${codigoDe(E.municipio) === cod ? 'tirar o filtro' : 'filtrar'}</i>` : '')
      : 'Nenhum bem tombado desta cidade na base';
    mostraDica(`<b>${esc(m.n)}</b>${linhas}`, e.clientX, e.clientY);
  }

  /* ================================================================
   * Dica flutuante (mapa, barras, botões com data-dica)
   * ================================================================ */
  const dica = $('#dica');
  function mostraDica(html, x, y) {
    dica.innerHTML = html;
    dica.hidden = false;
    const r = dica.getBoundingClientRect();
    let left = x + 14;
    let top = y + 16;
    if (left + r.width > innerWidth - 8) left = x - r.width - 14;
    if (top + r.height > innerHeight - 8) top = y - r.height - 12;
    dica.style.left = `${Math.max(8, left)}px`;
    dica.style.top = `${Math.max(8, top)}px`;
  }
  function escondeDica() { dica.hidden = true; }
  function ligaDicas() {
    document.addEventListener('pointermove', (e) => {
      if (e.target.closest && e.target.closest('#mapa-area')) return;
      const el = e.target.closest && e.target.closest('[data-dica]');
      if (el) mostraDica(el.dataset.dica, e.clientX, e.clientY);
      else if (!dica.hidden && !e.target.closest('#mapa-area')) escondeDica();
    });
    document.addEventListener('focusin', (e) => {
      const el = e.target.closest && e.target.closest('[data-dica]');
      if (!el) return;
      const r = el.getBoundingClientRect();
      mostraDica(el.dataset.dica, r.left, r.bottom);
    });
    document.addEventListener('focusout', escondeDica);
  }

  /* ================================================================
   * Lista de resultados e detalhe
   * ================================================================ */
  function descricaoOrganizacao() {
    const q = ULTIMO.cfg.query;
    const t = E.resultado;
    if (t === 'lista_ord') {
      const c = q.ordem || 'similarity';
      return `ordenado por ${(ROTULO_CAMPO[c] || c).toLowerCase()} ${q.ordem ? (q.ordemDesc ? '↓' : '↑') : '↓'}`;
    }
    if (t === 'lista_gen') return q.grupo.length ? `agrupado por ${q.grupo.map((c) => (ROTULO_CAMPO[c] || c).toLowerCase()).join(' › ')}` : 'sem GROUP: tudo na raiz';
    if (t === 'lista_cruz') return q.cruz.length === 2 ? `percorrido por ${E.porColuna ? 'coluna' : 'linha'} (${(ROTULO_CAMPO[E.porColuna ? q.cruz[1] : q.cruz[0]] || '').toLowerCase()})` : 'sem CROSS';
    return INFO[t].ordem;
  }

  function chaveGrupo(it) {
    const q = ULTIMO.cfg.query;
    if (E.resultado === 'lista_gen' && q.grupo.length) return q.grupo.map((c) => Motor.chaveDoItem(it, c)).join(' › ');
    if (E.resultado === 'lista_cruz' && q.cruz.length === 2) return Motor.chaveDoItem(it, E.porColuna ? q.cruz[1] : q.cruz[0]);
    return null;
  }

  function renderLista() {
    const ol = $('#lista-resultados');
    if (!ULTIMO || ULTIMO.erro) {
      $('#lista-desc').textContent = 'Corrija o arquivo de consulta para ver o resultado.';
      ol.innerHTML = `<li class="vazio"><b>A consulta tem um erro</b>${esc(ULTIMO.erro.erro)} (linha ${ULTIMO.erro.linha})</li>`;
      return;
    }
    const { saida, casados } = ULTIMO;
    $('#lista-desc').innerHTML = `Guardado na <b>${INFO[E.resultado].nome.toLowerCase()}</b>, ${esc(descricaoOrganizacao())}. ` +
      (casados.length > saida.length ? `Mostrando ${saida.length} de ${NF.format(casados.length)}.` : `${plural(saida.length, 'bem', 'bens')}.`);
    if (!saida.length) {
      ol.innerHTML = '<li class="vazio"><b>Nenhum bem encontrado</b>Tente tirar algum filtro ou aumentar a faixa de anos.</li>';
      return;
    }
    const comSim = ULTIMO.cfg.query.sims.length > 0;
    let grupoAnterior = null;
    const html = [];
    saida.forEach((it, i) => {
      const g = chaveGrupo(it);
      if (g !== null && g !== grupoAnterior) {
        html.push(`<li class="grupo-sep" aria-hidden="true">${esc(g)}</li>`);
        grupoAnterior = g;
      }
      const tipo = campo(it, 'tipo_protecao') || '?';
      const sim = ULTIMO.sim.get(it.id) ?? 0;
      const medidor = comSim
        ? `<span class="medidor"><span class="medidor-trilho"><span style="width:${(sim * 100).toFixed(1)}%"></span></span>nota ${dec(sim)}</span>`
        : '';
      html.push(`<li class="res" tabindex="0" data-id="${it.id}"><span class="pos">${i + 1}</span><div class="res-corpo">` +
        `<span class="res-nome">${esc(nomeDo(it))}</span>` +
        `<span class="res-meta"><span>${esc(campo(it, 'municipio') || '—')}</span><span>${it.ano ?? '—'}</span><span class="tipo tipo-${esc(tipo)}">${esc(tipo)}</span></span>` +
        `${medidor}</div></li>`);
    });
    ol.innerHTML = html.join('');
  }

  function ligaLista() {
    const ol = $('#lista-resultados');
    ol.addEventListener('click', (e) => {
      const li = e.target.closest('.res');
      if (li) abreDetalhe(Number(li.dataset.id));
    });
    ol.addEventListener('keydown', (e) => {
      const li = e.target.closest('.res');
      if (li && (e.key === 'Enter' || e.key === ' ')) { e.preventDefault(); abreDetalhe(Number(li.dataset.id)); }
    });
    ol.addEventListener('pointerover', (e) => {
      const li = e.target.closest('.res');
      if (!li) return;
      const it = ITENS[Number(li.dataset.id)];
      realcaCidades(cidadesDo(it));
      gBolhas.querySelectorAll('.bolha').forEach((b) => b.classList.toggle('realce', b.dataset.cod === it.cod));
    });
    ol.addEventListener('pointerleave', () => {
      realcaCidades([]);
      gBolhas.querySelectorAll('.bolha.realce').forEach((b) => b.classList.remove('realce'));
    });
    $('#detalhe').addEventListener('close', () => realcaCidades([]));
    $('#detalhe').addEventListener('click', (e) => {
      const r = e.currentTarget.getBoundingClientRect();
      const fora = e.clientX < r.left || e.clientX > r.right || e.clientY < r.top || e.clientY > r.bottom;
      if (e.target === e.currentTarget && fora) e.currentTarget.close();
    });
  }

  function abreDetalhe(id) {
    const it = ITENS[id];
    const linhas = [
      ['Município', campo(it, 'municipio')],
      ['Cidades', it.cidades.length ? `${it.cidades.length} cidades (destacadas no mapa)` : null],
      ['Região', campo(it, 'regiao_administrativa')],
      ['Ano do tombamento', it.ano],
      ['Tipo de proteção', campo(it, 'tipo_protecao')],
      ['Livro do Tombo', campo(it, 'livro_tombo')],
      ['Nº do processo', campo(it, 'processo')],
      ['Resolução', campo(it, 'resolucao_tombamento')],
      ['Publicação no DOE', campo(it, 'publicacao_doe')],
      ['Área envoltória', [campo(it, 'tipo_area_envoltoria'), campo(it, 'resolucao_area_envoltoria')].filter(Boolean).join(' · ') || null],
      ['Observações', campo(it, 'observacoes')],
    ].filter(([, v]) => v !== null && v !== undefined && v !== '');
    $('#detalhe-corpo').innerHTML =
      `<h2 id="detalhe-titulo">${esc(nomeDo(it))}</h2>` +
      `<dl class="detalhe-campos">${linhas.map(([k, v]) => `<dt>${k}</dt><dd>${esc(v)}</dd>`).join('')}</dl>` +
      `<details class="linha-csv"><summary>Ver a linha ${it.id + 2} do arquivo CSV (o que o programa em C lê)</summary><pre class="codigo">${esc(it.linha)}</pre></details>`;
    realcaCidades(cidadesDo(it));
    $('#detalhe').showModal();
  }

  /* ================================================================
   * Por dentro do programa: animação passo a passo
   * ================================================================ */
  const A = { gen: null, ev: null, base: null, res: null, saida: [], avaliados: 0, terminou: false, tocando: false, timer: null };
  const ATRASOS = [1600, 1100, 750, 500, 320, 200, 110, 55, 20, 4];
  const FASES = [['carga', '1 · Carregar'], ['busca', '2 · Buscar'], ['escrita', '3 · Escrever']];
  const faseDe = (ev) => (!ev ? 'fim' : ev.tipo === 'inicio' ? 'pronto' : ev.tipo === 'carga' ? 'carga'
    : ev.tipo === 'avalia' || ev.tipo === 'insere' ? 'busca' : ev.tipo === 'escreve' ? 'escrita' : 'fim');

  function pararAnimacao(descartar) {
    A.tocando = false;
    clearTimeout(A.timer);
    if (descartar) A.gen = null;
    $('#a-play').classList.remove('tocando');
    $('#a-play-rot').textContent = A.gen && !A.terminou ? 'Continuar' : 'Passo a passo';
  }

  function prepararAnimacao() {
    pararAnimacao(true);
    if (!ULTIMO || ULTIMO.erro) return false;
    A.gen = Motor.executar(BASE, ULTIMO.cfg);
    const ini = A.gen.next().value;
    Object.assign(A, { ev: ini, base: ini.base, res: ini.resultado, saida: [], avaliados: 0, terminou: false });
    return true;
  }

  function passo() {
    if (!A.gen || A.terminou) return false;
    const { value, done } = A.gen.next();
    if (done) { A.terminou = true; return false; }
    A.ev = value;
    if (value.tipo === 'avalia') A.avaliados++;
    if (value.tipo === 'escreve') A.saida.push(value.item);
    if (value.tipo === 'fim') A.terminou = true;
    return true;
  }

  function agenda() {
    clearTimeout(A.timer);
    if (!A.tocando) return;
    A.timer = setTimeout(() => {
      passo();
      renderEstruturas();
      if (A.terminou) {
        pararAnimacao(false);
        $('#a-play-rot').textContent = 'Ver de novo';
      } else agenda();
    }, ATRASOS[Number($('#a-vel').value) - 1]);
  }

  function ligaAnimacao() {
    $('#a-play').addEventListener('click', () => {
      if (A.tocando) { pararAnimacao(false); return; }
      if (!A.gen || A.terminou) { if (!prepararAnimacao()) return; renderEstruturas(); }
      A.tocando = true;
      $('#a-play').classList.add('tocando');
      $('#a-play-rot').textContent = 'Pausar';
      agenda();
    });
    $('#a-passo').addEventListener('click', () => {
      pararAnimacao(false);
      if (!A.gen || A.terminou) prepararAnimacao(); else passo();
      $('#a-play-rot').textContent = A.terminou ? 'Ver de novo' : 'Continuar';
      renderEstruturas();
    });
    $('#a-reiniciar').addEventListener('click', () => {
      prepararAnimacao();
      $('#a-play-rot').textContent = 'Começar';
      renderEstruturas();
    });
    $('#a-fase').addEventListener('click', () => {
      pararAnimacao(false);
      if (!A.gen || A.terminou) prepararAnimacao();
      const atual = faseDe(A.ev) === 'pronto' ? 'carga' : faseDe(A.ev);
      while (!A.terminou && passo()) {
        const f = faseDe(A.ev);
        if (f !== atual && f !== 'pronto') break;
      }
      $('#a-play-rot').textContent = A.terminou ? 'Ver de novo' : 'Continuar';
      renderEstruturas();
    });
    $('#a-fim').addEventListener('click', () => {
      pararAnimacao(false);
      if (!A.gen) prepararAnimacao();
      while (passo()) { /* consome até o fim */ }
      $('#a-play-rot').textContent = 'Ver de novo';
      renderEstruturas();
    });
  }

  const marcaOk = '<span class="marca-ok" aria-label="passou">✓</span>';
  const marcaErro = '<span class="marca-erro" aria-label="falhou">✗</span>';
  const marcaNota = '<span class="marca-nota" aria-label="nota calculada">≈</span>';
  const marcaDe = (p) => (p.tipo === 'sim' && p.ok ? marcaNota : p.ok ? marcaOk : marcaErro);

  function descreveInsercao(tipo, info, campos) {
    switch (tipo) {
      case 'pilha': return 'Empilhado no <b>topo</b>: 1 passo.';
      case 'fila': return 'Entrou no <b>fim da fila</b>: 1 passo.';
      case 'lista_enc': return 'Ligado <b>depois do último nó</b>: 1 passo.';
      case 'lista_cabeca': return 'Ligado <b>depois do último nó</b>; a cabeça continua no começo: 1 passo.';
      case 'lista_seq': return `Guardado na posição <b>[${info.onde}]</b>.` + (info.cresceu
        ? ` O vetor estava cheio: dobrou de ${info.cresceu.de} para ${info.cresceu.para} posições e copiou ${info.cresceu.copias} ponteiros.` : ' 1 passo.');
      case 'lista_ord':
        if (info.caminho === 'vazia') return 'Primeiro nó da lista: nada para comparar.';
        if (info.caminho === 'fim') return `<b>Atalho:</b> comparou só com o último e entrou direto no fim (posição ${info.onde}).`;
        if (info.caminho === 'inicio') return `É o novo <b>primeiro</b> da lista (${plural(info.comparacoes, 'comparação', 'comparações')}).`;
        return `Andou pela lista e entrou na <b>posição ${info.onde}</b> depois de ${plural(info.comparacoes, 'comparação', 'comparações')}.`;
      case 'lista_gen':
        if (!info.caminho.length) return 'Sem GROUP: entra direto na raiz.';
        return info.caminho.map((c, i) => `<code>${esc(campos.grupo[i])}</code> = “${esc(c.chave)}”${c.criada ? ' <b>(sublista nova)</b>' : ''}`).join(' › ') +
          ` · ${plural(info.comparacoes, 'comparação', 'comparações')} para achar as sublistas.`;
      case 'lista_cruz':
        return `Linha “${esc(info.linha)}”${info.novaLinha ? ' <b>(nova)</b>' : ''} × coluna “${esc(info.coluna)}”${info.novaColuna ? ' <b>(nova)</b>' : ''}: ` +
          `o <b>mesmo nó</b> entra nas duas listas (${plural(info.comparacoes, 'comparação', 'comparações')} para achar os descritores).`;
    }
    return '';
  }

  function descreveChecagem(p) {
    if (p.tipo === 'filtro') {
      const f = p.filtro;
      const valor = p.valor === null ? '<i>(vazio)</i>' : typeof p.valor === 'number' ? p.valor : `“${esc(curto(String(p.valor), 40))}”`;
      const alvo = Number.isNaN(f.alvoNum) ? `“${esc(f.alvoTexto)}”` : esc(f.alvoTexto);
      return `<code>${esc(f.campo)}</code> ${valor} <b>${esc(f.op)}</b> ${alvo}`;
    }
    if (p.tipo === 'sim') {
      if (!p.ok) return `<code>${esc(p.sim.campo)}</code> sem número para comparar`;
      return `perto de ${p.sim.alvo}? ${p.valor} está a ${plural(p.diferenca, 'ano', 'anos')} → nota <b>${dec(p.nota)}</b>`;
    }
    if (p.tipo === 'aceite') return `nota final ${dec(p.similaridade)} ${p.ok ? '≥' : '<'} mínimo ${dec(p.limiar)} <code>ACCEPT</code>`;
    if (p.tipo === 'extra') return `${esc(p.descricao)} <span class="selo-extra">extra do site</span>`;
    return '';
  }

  function renderEvento(ev, st, q) {
    const el = $('#evento');
    const contas = (lidos, avaliados) => `<div class="ev-contas"><span>Lidos do arquivo</span><b>${lidos}</b><span>Avaliados na busca</span><b>${avaliados}</b>` +
      `<span>Aprovados</span><b>${st.casados}</b><span>Passos na base</span><b>${NF.format(st.passosBase)}</b><span>Passos no resultado</span><b>${NF.format(st.passosResultado)}</b>` +
      `<span>Escritos na saída</span><b>${st.saida}</b></div>`;
    if (!ev) {
      el.innerHTML = `<span class="ev-fase">Tudo pronto</span><p>O programa leu <b>${st.lidos}</b> bens, <b>${st.casados}</b> passaram na busca e <b>${st.saida}</b> foram escritos na saída.</p>` +
        `<p class="ev-sub">Aperte <b>Passo a passo</b> para ver cada etapa acontecendo. Dica: com <b>“Usar só 15 bens”</b> fica mais fácil acompanhar.</p>` + contas(st.lidos, st.lidos);
      return;
    }
    const it = ev.item;
    let corpo = '';
    if (ev.tipo === 'inicio') {
      corpo = '<span class="ev-fase">Começo</span><p>As duas estruturas foram criadas vazias. Aperte <b>▶</b> ou <b>próximo passo</b>.</p>';
    } else if (ev.tipo === 'carga') {
      corpo = `<span class="ev-fase">Fase 1 · carregando a base</span><p class="ev-sub">Lendo a linha ${it.id + 2} do arquivo:</p><p class="ev-item">${esc(nomeDo(it))}</p>` +
        `<p>${descreveInsercao(E.base, ev.info, q)}</p>`;
    } else if (ev.tipo === 'avalia') {
      const checks = ev.av.passos.length
        ? `<ul class="checagens">${ev.av.passos.map((p) => `<li>${marcaDe(p)}<span>${descreveChecagem(p)}</span></li>`).join('')}</ul>`
        : '<p class="ev-sub">A consulta não tem FILTER nem SIM: todo bem é aceito.</p>';
      const parou = !ev.av.aceito && ev.av.passos.length && ev.av.passos[ev.av.passos.length - 1].tipo === 'filtro' && ev.av.passos.length < q.filtros.length
        ? '<p class="ev-sub">Parou no primeiro filtro que falhou: os outros nem são testados.</p>' : '';
      corpo = `<span class="ev-fase">Fase 2 · buscando</span><p class="ev-sub">Visitando o próximo bem da base:</p><p class="ev-item">${esc(nomeDo(it))}</p>` +
        `<p class="ev-sub">${esc(campo(it, 'municipio') || '—')} · ${it.ano ?? '—'} · ${esc(campo(it, 'tipo_protecao') || '')}</p>${checks}${parou}` +
        `<div class="veredito ${ev.av.aceito ? 'ok' : 'erro'}">${ev.av.aceito ? marcaOk + ' Aprovado: vai para o resultado' : marcaErro + ' Descartado'}</div>`;
    } else if (ev.tipo === 'insere') {
      corpo = `<span class="ev-fase">Fase 2 · guardando no resultado</span><p class="ev-item">${esc(nomeDo(it))}</p><p>${descreveInsercao(E.resultado, ev.info, q)}</p>`;
    } else if (ev.tipo === 'escreve') {
      const percurso = E.resultado === 'lista_cruz' ? ` por ${ULTIMO.cfg.porColuna ? 'coluna' : 'linha'}` : '';
      corpo = `<span class="ev-fase">Fase 3 · escrevendo a saída</span><p class="ev-sub">Percorrendo a ${INFO[E.resultado].nome.toLowerCase()}${percurso}:</p>` +
        `<p class="ev-item">${ev.n}º · ${esc(nomeDo(it))}</p>`;
    } else if (ev.tipo === 'fim') {
      const cortou = ULTIMO.cfg.topK > 0 && st.casados > st.saida;
      corpo = `<span class="ev-fase">Fim</span><div class="veredito neutro">STATUS=OK</div>` +
        `<p>${cortou ? `Parou de escrever ao chegar no <code>--top-k ${ULTIMO.cfg.topK}</code>.` : 'Todos os aprovados foram escritos.'}</p>`;
    }
    el.innerHTML = corpo + contas(st.lidos, A.avaliados);
  }

  function renderPipeline(st, ev, avaliados) {
    const fase = faseDe(ev);
    const ativa = {
      arquivo: ev && ev.tipo === 'carga', base: ev && (ev.tipo === 'carga' || ev.tipo === 'avalia'),
      busca: ev && (ev.tipo === 'avalia' || ev.tipo === 'insere'), resultado: ev && (ev.tipo === 'insere' || ev.tipo === 'escreve'),
      saida: ev && (ev.tipo === 'escreve' || fase === 'fim'),
    };
    const etapas = [
      ['arquivo', 'Arquivo', 'patrimonio.csv', `${NF.format(E.limite || ITENS.length)} linhas a ler`, 'reader_proximo()'],
      ['base', 'Base', INFO[E.base].nome, `${NF.format(st.lidos)} guardados`, 'estrutura_inserir()'],
      ['busca', 'Busca', 'filtros da consulta', `✓ ${NF.format(st.casados)} · ✗ ${NF.format(avaliados - st.casados)}`, 'avalia_query()'],
      ['resultado', 'Resultado', INFO[E.resultado].nome, `${NF.format(st.casados)} guardados`, 'estrutura_inserir()'],
      ['saida', 'Saída', 'saida.csv', `${NF.format(st.saida)} linhas`, 'visitante_escrita()'],
    ];
    $('#pipeline').innerHTML = etapas.map(([id, rot, nome, num, fn], i) =>
      `<li class="etapa${ativa[id] ? ' ativa' : ''}"><span class="et-rot">${i + 1} · ${rot}</span><span class="et-nome">${esc(nome)}</span>` +
      `<span class="et-num">${num}</span><code>${fn}</code></li>`).join('');
    const ordem = ['carga', 'busca', 'escrita'];
    const idx = fase === 'fim' ? 3 : ordem.indexOf(fase);
    $('#fases').innerHTML = FASES.map(([f, rot], i) =>
      `<span class="fase${f === fase ? ' atual' : i < idx ? ' feita' : ''}">${rot}</span>`).join('');
  }

  function renderSaida(lista, ultimoNovo) {
    const linhas = [`<li class="cabecalho">${esc(BASE.cabecalho)}</li>`];
    const max = 200;
    lista.slice(0, max).forEach((it, i) => {
      linhas.push(`<li class="${ultimoNovo && i === lista.length - 1 ? 'nova' : ''}" title="${esc(it.linha)}">${esc(it.linha)}</li>`);
    });
    if (lista.length > max) linhas.push(`<li class="parou">… mais ${lista.length - max} linhas</li>`);
    const fim = !A.gen || A.terminou;
    if (fim && ULTIMO.cfg.topK > 0 && ULTIMO.st.casados > ULTIMO.st.saida) {
      linhas.push(`<li class="parou">parou aqui: --top-k ${ULTIMO.cfg.topK} (havia ${ULTIMO.st.casados} aprovados)</li>`);
    }
    $('#saida-linhas').innerHTML = linhas.join('');
    $('#saida-tam').textContent = plural(lista.length, 'linha', 'linhas');
    const ol = $('#saida-linhas');
    if (ultimoNovo) ol.scrollTop = ol.scrollHeight;
  }

  function renderEstruturas() {
    $('#estr-res-nome').textContent = `${INFO[E.resultado].nome} (${E.resultado})`;
    $('#viz-base-nome').textContent = INFO[E.base].nome;
    $('#viz-res-nome').textContent = INFO[E.resultado].nome;
    $('#viz-base-explica').innerHTML = `${INFO[E.base].analogia} ${INFO[E.base].custo}`;
    $('#viz-res-explica').innerHTML = `${INFO[E.resultado].analogia} ${INFO[E.resultado].custo}`;
    renderCodigo();
    if (!ULTIMO || ULTIMO.erro) {
      $('#evento').innerHTML = '<div class="veredito erro">A consulta tem um erro: corrija o arquivo de consulta.</div>';
      for (const id of ['#viz-base', '#viz-res', '#pipeline', '#saida-linhas', '#fases']) $(id).innerHTML = '';
      return;
    }
    const animando = A.gen !== null;
    const base = animando ? A.base : ULTIMO.base;
    const res = animando ? A.res : ULTIMO.resultado;
    const st = animando ? A.ev.st : ULTIMO.st;
    const ev = animando ? A.ev : null;
    const avaliados = animando ? A.avaliados : ULTIMO.st.lidos;
    const destaque = (modo, info) => ({ id: ev.item.id, item: ev.item, modo, info });
    let dBase = null;
    let dRes = null;
    if (ev && ev.item) {
      if (ev.tipo === 'carga') dBase = destaque('novo', ev.info);
      if (ev.tipo === 'avalia') dBase = destaque('percorre');
      if (ev.tipo === 'insere') { dBase = destaque('percorre'); dRes = destaque('novo', ev.info); }
      if (ev.tipo === 'escreve') dRes = destaque('percorre');
    }
    $('#viz-base').innerHTML = window.Viz.desenhar(E.base, base, dBase, { compacto: true, porColuna: false });
    $('#viz-res').innerHTML = window.Viz.desenhar(E.resultado, res, dRes, { porColuna: ULTIMO.cfg.porColuna });
    $('#viz-base-tam').textContent = plural(base.tamanho, 'item', 'itens');
    $('#viz-res-tam').textContent = plural(res.tamanho, 'item', 'itens');
    renderPipeline(st, ev, avaliados);
    renderEvento(ev, st, ULTIMO.cfg.query);
    renderSaida(animando ? A.saida : ULTIMO.saida, ev && ev.tipo === 'escreve');
    // durante a reprodução, mantém visível o nó que acabou de mudar
    if (A.tocando && ev) {
      const sel = ev.tipo === 'carga' ? '#viz-base .novo' : ev.tipo === 'insere' || ev.tipo === 'escreve' ? '#viz-res .novo, #viz-res .foco' : null;
      const foco = sel && $(sel);
      if (foco) foco.scrollIntoView({ block: 'nearest', inline: 'nearest' });
    }
  }

  function realcaC(src) {
    const re = /(\/\*[\s\S]*?\*\/|\/\/[^\n]*)|("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|\b(if|else|for|while|return|break|continue|switch|case|default|static|const|struct|typedef|sizeof|NULL|true|false)\b|\b(bool|int|double|char|void|size_t|long|ITEM|NoGen|NoOrd|NoCruzado|NoLista|NoCabeca|NoPilha|NoFila|Descritor|ListaSeq|ListaEnc|ListaCabeca|ListaOrd|ListaGen|ListaCruz|PILHA|FILA|Query|Filter|Sim|VisitanteItem|ContextoBusca|ContextoEscrita)\b/g;
    let out = '';
    let ult = 0;
    src.replace(re, (m, com, str, kw, tipo, pos) => {
      out += esc(src.slice(ult, pos));
      out += `<span class="c-${com ? 'com' : str ? 'str' : kw ? 'kw' : 'tipo'}">${esc(m)}</span>`;
      ult = pos + m.length;
      return m;
    });
    return out + esc(src.slice(ult));
  }
  const trechos = (lista) => lista.map((t) =>
    `<div class="trecho"><div class="trecho-cab"><code>${esc(t.arquivo)}</code> · função <code>${esc(t.funcao)}</code></div><pre class="codigo">${realcaC(t.codigo)}</pre></div>`).join('');
  let codigoMostrado = null;
  function renderCodigo() {
    if (codigoMostrado === E.resultado) return;
    codigoMostrado = E.resultado;
    $('#codigo-res').innerHTML = trechos(window.CODIGO_C[E.resultado] || []);
    $('#codigo-busca').innerHTML = trechos([...(window.CODIGO_C.main || []), ...(window.CODIGO_C.query || [])]);
  }

  /* ================================================================
   * Corrida
   * ================================================================ */
  function barras(el, linhas, { formato, destaque, dica: dicaDe }) {
    const max = Math.max(...linhas.map((l) => l.valor), 1e-9);
    el.innerHTML = linhas.map((l) =>
      `<div class="barra-linha${l.chave === destaque ? ' escolhida' : ''}" tabindex="0" data-dica="${esc(dicaDe(l))}">` +
      `<span class="barra-rot">${esc(l.rotulo)}<small>${esc(l.chave)}</small></span>` +
      `<span class="barra-trilho"><span class="barra" style="--p:${(l.valor / max).toFixed(4)}"></span><span class="barra-valor">${formato(l.valor)}</span></span></div>`).join('');
  }
  const ms = (v) => (v < 1 ? `${v.toLocaleString('pt-BR', { maximumFractionDigits: 3 })} ms` : `${v.toLocaleString('pt-BR', { maximumFractionDigits: 1 })} ms`);
  const segundos = (v) => (v >= 1000 ? `${(v / 1000).toLocaleString('pt-BR', { maximumFractionDigits: 1 })} s` : `${(v / 1000).toLocaleString('pt-BR', { maximumFractionDigits: 2 })} s`);

  function renderCorrida() {
    const alvo = $('#corrida-passos');
    if (!ULTIMO || ULTIMO.erro) {
      alvo.innerHTML = '<p class="vazio">Corrija o arquivo de consulta para ver a corrida.</p>';
    } else {
      const r = Motor.carregaQuery(manual ?? textoConsulta(E, { todas: true }));
      const q = r.ok ? r.query : ULTIMO.cfg.query;
      const linhas = Motor.NOMES_ESTRUTURAS.map((t) => {
        const fim = Motor.executarTudo(BASE, { ...ULTIMO.cfg, query: q, resultado: t, topK: 0 });
        return { chave: t, rotulo: INFO[t].nome, valor: fim.st.passosResultado, n: fim.st.casados };
      });
      const n = linhas[0].n;
      $('#corrida-n').textContent = plural(n, 'bem encontrado', 'bens encontrados');
      barras(alvo, linhas, {
        formato: (v) => `${NF.format(v)} passos`,
        destaque: E.resultado,
        dica: (l) => `<b>${l.rotulo}</b>${NF.format(l.valor)} passos para guardar ${plural(l.n, 'bem', 'bens')}` + (l.n ? ` (${dec(l.valor / l.n, 1)} por bem)` : ''),
      });
      const maior = linhas.reduce((a, b) => (b.valor > a.valor ? b : a));
      const ord = linhas.find((l) => l.chave === 'lista_ord');
      $('#corrida-nota').innerHTML = n === 0
        ? 'Nenhum bem passou na busca, então nenhuma estrutura teve trabalho.'
        : `Pilha, fila e as listas encadeadas gastam <b>1 passo por bem</b>: só ligam o nó novo numa ponta. ` +
          `A mais trabalhosa nesta busca foi a <b>${maior.rotulo.toLowerCase()}</b> (${NF.format(maior.valor)} passos). ` +
          `A lista ordenada usa a ordem <code>${esc(q.ordem || 'similarity')}</code>: experimente trocar a ordem e veja os passos mudarem` +
          (ord && ord.valor <= ord.n * 2 ? ' (agora quase todo bem caiu no atalho de entrar direto no fim).' : '.');
    }
    renderMedido();
  }

  function renderMedido() {
    const ex = window.EXPERIMENTOS || {};
    const graf = (id, sub, dados, unidade) => {
      if (!dados) { $(id).innerHTML = '<p class="nota">Sem dados de experimento.</p>'; return; }
      const linhas = dados.linhas.filter((r) => r.exp === 'C').map((r) => ({ chave: r.resultado, rotulo: INFO[r.resultado].nome, valor: r.insercao_ms, n: r.casados }));
      const n = linhas.length ? linhas[0].n : 0;
      $(sub).textContent = `Tempo para inserir os ${NF.format(n)} ${unidade} aprovados no resultado (RESULT_INSERT_MS, média de ${dados.execucoes || '?'} execuções).`;
      barras($(id), linhas, { formato: ms, destaque: E.resultado, dica: (l) => `<b>${l.rotulo}</b>${ms(l.valor)} para inserir ${NF.format(l.n)} itens` });
    };
    graf('#medido-pat', '#medido-pat-sub', ex.patrimonio, 'bens');
    graf('#medido-ast', '#medido-ast-sub', ex.asteroides, 'asteroides');

    const ast = ex.asteroides;
    const heroi = $('#heroi');
    const b = ast && ast.linhas.filter((r) => r.exp === 'B');
    const ord = b && b.find((r) => r.base === 'lista_ord');
    const fila = b && b.find((r) => r.base === 'fila');
    if (!ord || !fila) { heroi.hidden = true; return; }
    const vezes = Math.round(ord.carga_ms / fila.carga_ms);
    const lidos = ast.lidos ? `${NF.format(Math.round(ast.lidos / 1000))} mil` : 'milhares de';
    heroi.innerHTML =
      `<div class="heroi-num lento"><span class="valor">${segundos(ord.carga_ms)}</span><span class="rot">lista ordenada como base</span></div>` +
      `<div class="heroi-num"><span class="valor">${segundos(fila.carga_ms)}</span><span class="rot">fila como base</span></div>` +
      `<p class="heroi-texto">Na parte 1 do trabalho, carregar <b>${lidos} asteroides</b> numa <b>lista ordenada</b> levou <b>${segundos(ord.carga_ms)}</b> no programa em C: ` +
      `cada asteroide novo andava pela lista procurando o seu lugar. Na <b>fila</b>, que só liga no fim, levou <b>${segundos(fila.carga_ms)}</b>, cerca de <b>${NF.format(vezes)} vezes</b> mais rápido. ` +
      `Com poucos dados tudo parece instantâneo; com muitos, a escolha da estrutura faz toda a diferença.</p>`;
  }

  /* ================================================================
   * Desafios
   * ================================================================ */
  function rodarEstado(parcial) {
    const s = { ...PADRAO(), ...parcial };
    const r = Motor.carregaQuery(textoConsulta(s));
    const fim = Motor.executarTudo(BASE, { query: r.query, base: s.base, resultado: s.resultado, limite: s.limite, topK: s.topK, porColuna: s.porColuna, filtroExtra: null });
    return { ...fim, todos: [...fim.resultado.iterar()] };
  }
  const empatados = (lista) => lista.filter((i) => i.ano === lista[0].ano);
  const listaNomes = (l) => l.map((i) => `<b>${esc(nomeDo(i))}</b>`).join(', ');

  const DESAFIOS = [
    {
      titulo: 'Qual é o bem tombado há mais tempo em Campinas?',
      dica: 'Escolha Campinas (no mapa ou na lista) e guarde o resultado numa lista ordenada por ano, crescente.',
      estado: { municipio: 'Campinas', resultado: 'lista_ord', ordemCampo: 'ano_tombamento', ordemDesc: false, topK: 5 },
      aba: 'mapa',
      resposta: () => {
        const e = empatados(rodarEstado(DESAFIOS[0].estado).todos);
        return `${listaNomes(e)}, tombado${e.length > 1 ? 's' : ''} em ${e[0].ano}.`;
      },
    },
    {
      titulo: 'Quantos bens foram tombados nos anos 1980 (de 1980 a 1989)?',
      dica: 'Use os dois controles de ano: de 1980 até 1989. A resposta aparece em “Passaram na busca”.',
      estado: { anoMin: 1980, anoMax: 1989 },
      aba: 'mapa',
      resposta: () => {
        const f = rodarEstado(DESAFIOS[1].estado);
        const total = f.todos.length;
        return `<b>${total} bens</b>, quase ${Math.round((100 * total) / ITENS.length)}% de toda a base. Só em 1982 foram ${ITENS.filter((i) => i.ano === 1982).length}!`;
      },
    },
    {
      titulo: 'Sem contar a capital, qual cidade tem mais bens tombados?',
      dica: 'Tire a capital com um filtro “diferente de” editando o arquivo à mão, ou só compare o tamanho das bolhas no mapa.',
      estado: { resultado: 'lista_gen', grupo1: 'municipio', grupo2: '', topK: 0 },
      aba: 'mapa',
      resposta: () => {
        const m = new Map();
        for (const it of ITENS) if (it.cod && it.cod !== '3550308') m.set(it.cod, (m.get(it.cod) || 0) + 1);
        const [cod, n] = [...m.entries()].sort((a, b) => b[1] - a[1])[0];
        const seg = [...m.entries()].sort((a, b) => b[1] - a[1])[1];
        return `<b>${esc(MUN.get(cod).n)}</b>, com ${n} bens. Em segundo vem ${esc(MUN.get(seg[0]).n)}, com ${seg[1]}. A capital sozinha tem ${NA_BASE.get('3550308')}.`;
      },
    },
    {
      titulo: 'Qual foi o bem tombado mais recentemente na Baixada Santista?',
      dica: 'Escolha a região “Região Metropolitana da Baixada Santista” e ordene por ano, decrescente.',
      estado: { regiao: 'Região Metropolitana da Baixada Santista', resultado: 'lista_ord', ordemCampo: 'ano_tombamento', ordemDesc: true, topK: 5 },
      aba: 'mapa',
      resposta: () => {
        const e = empatados(rodarEstado(DESAFIOS[3].estado).todos);
        return `${listaNomes(e)} (${esc(campo(e[0], 'municipio'))}), tombado${e.length > 1 ? 's' : ''} em ${e[0].ano}.`;
      },
    },
    {
      titulo: 'Guarde o resultado numa fila e depois numa pilha. Por que o 1º bem muda?',
      dica: 'O arquivo CSV está em ordem alfabética. Pense em qual bem entra primeiro e em qual sai primeiro de cada estrutura.',
      estado: { resultado: 'pilha', topK: 5 },
      aba: 'mapa',
      resposta: () => {
        const fila = rodarEstado({ resultado: 'fila' }).saida[0];
        const pilha = rodarEstado({ resultado: 'pilha' }).saida[0];
        return `Na fila, o 1º é <b>${esc(nomeDo(fila))}</b>, a primeira linha do arquivo. Na pilha, é <b>${esc(nomeDo(pilha))}</b>, a última. ` +
          'A fila devolve primeiro quem entrou primeiro (FIFO); a pilha devolve primeiro quem entrou por último (LIFO), então a ordem alfabética sai de trás para frente.';
      },
    },
    {
      titulo: 'Na lista cruzada, quantos bens TEO estão no Livro do Tombo Histórico?',
      dica: 'Escolha a lista cruzada com linhas = tipo de proteção e colunas = Livro do Tombo, e olhe a aba “Por dentro do programa”.',
      estado: { resultado: 'lista_cruz', cruzLinha: 'tipo_protecao', cruzColuna: 'livro_tombo', topK: 10 },
      aba: 'estruturas',
      resposta: () => {
        const f = rodarEstado(DESAFIOS[5].estado);
        const linha = f.resultado.linhas.find((l) => l.chave === 'TEO');
        const n = linha ? linha.nos.filter((x) => x.coluna.chave === 'Livro do Tombo Histórico').length : 0;
        return `<b>${n} bens</b>. Cada um deles é um único nó, ligado ao mesmo tempo na linha “TEO” e na coluna “Livro do Tombo Histórico”.`;
      },
    },
    {
      titulo: 'Ligue o “Parecido com…”: ano 1975, tolerância 10, nota mínima 0,50. Quantos bens passam e de quais anos?',
      dica: 'A nota é 1 − |ano − 1975| ÷ 10. Para a nota ser pelo menos 0,50, quão longe de 1975 o ano pode estar?',
      estado: { simAtivo: true, simAlvo: 1975, simTol: 10, simAceite: 0.5, resultado: 'lista_ord', ordemCampo: 'similarity', ordemDesc: true, topK: 10 },
      aba: 'mapa',
      resposta: () => {
        const t = rodarEstado(DESAFIOS[6].estado).todos;
        const a = t.map((i) => i.ano);
        return `<b>${t.length} bens</b>, tombados de ${Math.min(...a)} a ${Math.max(...a)}: no máximo 5 anos longe de 1975 (5 ÷ 10 = 0,5, então a nota fica ≥ 0,50).`;
      },
    },
  ];

  function renderDesafios() {
    $('#desafios').innerHTML = DESAFIOS.map((d, i) =>
      `<article class="cartao desafio" data-i="${i}"><span class="desafio-num">Desafio ${i + 1}</span><h3>${esc(d.titulo)}</h3>` +
      `<p class="desafio-dica" hidden>${esc(d.dica)}</p><div class="desafio-resposta" hidden></div>` +
      `<div class="acoes"><button class="botao-mini" data-acao="dica">Dica</button><button class="botao-mini" data-acao="montar">Montar a busca</button>` +
      `<button class="botao-mini" data-acao="resposta">Ver resposta</button></div></article>`).join('');
    $('#desafios').addEventListener('click', (e) => {
      const b = e.target.closest('button[data-acao]');
      if (!b) return;
      const card = b.closest('.desafio');
      const d = DESAFIOS[Number(card.dataset.i)];
      if (b.dataset.acao === 'dica') $('.desafio-dica', card).hidden = !$('.desafio-dica', card).hidden;
      if (b.dataset.acao === 'resposta') {
        const r = $('.desafio-resposta', card);
        if (r.hidden) r.innerHTML = d.resposta();
        r.hidden = !r.hidden;
        b.textContent = r.hidden ? 'Ver resposta' : 'Esconder resposta';
      }
      if (b.dataset.acao === 'montar') {
        manual = null;
        mostraManual(false);
        E = { ...PADRAO(), base: E.base, ...d.estado };
        sincronizaControles();
        atualizar();
        mostraAba(d.aba);
      }
    });
  }

  /* ================================================================
   * Abas, tema e tamanho do texto
   * ================================================================ */
  const ABAS = ['mapa', 'estruturas', 'corrida', 'desafios'];
  function mostraAba(id) {
    if (!ABAS.includes(id)) id = 'mapa';
    abaAtual = id;
    for (const a of ABAS) {
      $(`#aba-${a}`).setAttribute('aria-selected', String(a === id));
      $(`#sec-${a}`).hidden = a !== id;
    }
    if (id !== 'estruturas') pararAnimacao(false);
    if (id === 'estruturas') renderEstruturas();
    if (id === 'corrida') renderCorrida();
    if (id === 'mapa') requestAnimationFrame(desenhaBolhas);
    $('main').scrollTop = 0;
    // âncora "#/aba" para não coincidir com ids da página (o navegador rolaria até eles)
    if (location.hash !== '#/' + id) {
      try { history.replaceState(null, '', '#/' + id); } catch (e) { /* file:// em alguns navegadores */ }
    }
  }

  const abaDaUrl = () => location.hash.replace(/^#\/?/, '') || 'mapa';

  const ESCALAS = [0.875, 1, 1.125, 1.25, 1.4];
  let escala = clamp(Number(guarda.ler('escala') ?? 1), 0, ESCALAS.length - 1);
  function aplicaEscala() {
    document.documentElement.style.setProperty('--escala', ESCALAS[escala]);
    guarda.gravar('escala', escala);
    requestAnimationFrame(desenhaBolhas);
  }

  function ligaTopo() {
    for (const a of ABAS) $(`#aba-${a}`).addEventListener('click', () => mostraAba(a));
    $('.abas').addEventListener('keydown', (e) => {
      if (e.key !== 'ArrowRight' && e.key !== 'ArrowLeft') return;
      const i = ABAS.indexOf(abaAtual) + (e.key === 'ArrowRight' ? 1 : -1);
      const id = ABAS[(i + ABAS.length) % ABAS.length];
      mostraAba(id);
      $(`#aba-${id}`).focus();
    });
    $('#fonte-menos').onclick = () => { escala = Math.max(0, escala - 1); aplicaEscala(); };
    $('#fonte-mais').onclick = () => { escala = Math.min(ESCALAS.length - 1, escala + 1); aplicaEscala(); };
    const tema = guarda.ler('tema');
    if (tema === 'light' || tema === 'dark') document.documentElement.dataset.theme = tema;
    $('#tema').onclick = () => {
      const atual = document.documentElement.dataset.theme ||
        (matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
      const proximo = atual === 'dark' ? 'light' : 'dark';
      document.documentElement.dataset.theme = proximo;
      guarda.gravar('tema', proximo);
    };
    window.addEventListener('hashchange', () => mostraAba(abaDaUrl()));
  }

  /* ================================================================
   * Início
   * ================================================================ */
  preencheControles();
  montaMapa();
  ligaControles();
  ligaMapa();
  ligaDicas();
  ligaLista();
  ligaAnimacao();
  ligaTopo();
  renderDesafios();
  aplicaEscala();
  sincronizaControles();
  atualizar();
  mostraAba(abaDaUrl());
})();
