/* viz.js — desenha cada estrutura de dados a partir do estado real do motor.
 *
 * Viz.desenhar(tipo, estrutura, destaque, opcoes) devolve HTML.
 *   destaque = { id, modo: 'novo' | 'percorre', info }   (opcional)
 *   opcoes   = { compacto, porColuna }
 */
(function () {
  'use strict';

  const esc = (s) => String(s ?? '').replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  const nomeDo = (item) => item.texto[0] || '(sem nome)';
  const curto = (s, n) => (s.length > n ? s.slice(0, n - 1).trimEnd() + '…' : s);
  const plural = (n, um, varios) => `${n} ${n === 1 ? um : varios}`;

  const INFO = {
    pilha: {
      nome: 'Pilha', curto: 'Pilha',
      analogia: 'Como uma <b>pilha de pratos</b>: o último que entra fica em cima e é o primeiro a sair.',
      regra: 'Último a entrar, primeiro a sair (LIFO).',
      custo: 'Inserir custa 1 passo: o novo nó vira o topo.',
      ordem: 'o último encontrado aparece primeiro',
    },
    fila: {
      nome: 'Fila', curto: 'Fila',
      analogia: 'Como a <b>fila da cantina</b>: quem chega vai para o fim; quem está no começo sai primeiro.',
      regra: 'Primeiro a entrar, primeiro a sair (FIFO).',
      custo: 'Inserir custa 1 passo: a fila guarda quem é o último.',
      ordem: 'na ordem em que foram encontrados',
    },
    lista_seq: {
      nome: 'Lista sequencial', curto: 'Sequencial',
      analogia: 'Como uma <b>estante de gavetas numeradas</b> lado a lado. Quando enche, arruma uma estante com o dobro de gavetas e muda tudo para lá.',
      regra: 'Os itens ficam colados na memória: posição [0], [1], [2]…',
      custo: 'Inserir costuma custar 1 passo; quando enche, copia todos para um vetor 2× maior.',
      ordem: 'na ordem em que foram encontrados',
    },
    lista_enc: {
      nome: 'Lista encadeada', curto: 'Encadeada',
      analogia: 'Como uma <b>caça ao tesouro</b>: cada pista (nó) diz onde está a próxima.',
      regra: 'Cada nó aponta para o próximo; o último aponta para NULL.',
      custo: 'Inserir custa 1 passo: a lista guarda um ponteiro para o fim.',
      ordem: 'na ordem em que foram encontrados',
    },
    lista_cabeca: {
      nome: 'Lista com cabeça', curto: 'Com cabeça',
      analogia: 'Como um <b>trem com locomotiva</b>: a cabeça nunca leva passageiro, mas sempre existe, então a lista nunca fica “sem começo”.',
      regra: 'Um nó sentinela (cabeça), sem dado, fica sempre no início.',
      custo: 'Inserir custa 1 passo e não precisa testar “a lista está vazia?”.',
      ordem: 'na ordem em que foram encontrados',
    },
    lista_ord: {
      nome: 'Lista ordenada', curto: 'Ordenada',
      analogia: 'Como uma <b>fila por altura</b>: cada um que chega anda pela fila comparando até achar o seu lugar.',
      regra: 'A ordem do ORDER é mantida a cada inserção.',
      custo: 'Inserir pode custar até N comparações. Atalho: se for o último da ordem, entra direto no fim.',
      ordem: 'ordenado',
    },
    lista_gen: {
      nome: 'Lista generalizada', curto: 'Generalizada',
      analogia: 'Como <b>pastas dentro de pastas</b>: uma pasta para cada valor do 1º campo, e dentro dela uma para cada valor do 2º.',
      regra: 'Listas que guardam outras listas (comando GROUP).',
      custo: 'Inserir procura a sublista certa em cada nível (e cria se não existir).',
      ordem: 'agrupado',
    },
    lista_cruz: {
      nome: 'Lista cruzada', curto: 'Cruzada',
      analogia: 'Como uma <b>tabela de dupla entrada</b>: cada bem está numa linha e numa coluna ao mesmo tempo, mas existe uma vez só.',
      regra: 'Cada nó tem dois ponteiros: próximo da linha e próximo da coluna (comando CROSS).',
      custo: 'Inserir acha (ou cria) a linha e a coluna e liga o mesmo nó nas duas.',
      ordem: 'percorrido',
    },
  };

  // Ícones dos botões de estrutura (24×24, traço em currentColor)
  const ICONES = {
    pilha: '<rect x="5" y="4" width="14" height="4" rx="1.5"/><rect x="5" y="10" width="14" height="4" rx="1.5"/><rect x="5" y="16" width="14" height="4" rx="1.5"/>',
    fila: '<rect x="2" y="8" width="5" height="8" rx="1.5"/><rect x="9.5" y="8" width="5" height="8" rx="1.5"/><rect x="17" y="8" width="5" height="8" rx="1.5"/>',
    lista_seq: '<rect x="2" y="8" width="20" height="8" rx="1.5"/><path d="M7 8v8M12 8v8M17 8v8"/>',
    lista_enc: '<circle cx="4.5" cy="12" r="2.5"/><circle cx="12" cy="12" r="2.5"/><circle cx="19.5" cy="12" r="2.5"/><path d="M7 12h2.5M14.5 12h2.5"/>',
    lista_cabeca: '<rect x="1.5" y="9" width="6" height="6" rx="1" fill="currentColor"/><circle cx="13" cy="12" r="2.5"/><circle cx="20" cy="12" r="2.5"/><path d="M7.5 12h3M15.5 12h2"/>',
    lista_ord: '<rect x="3" y="14" width="4" height="6" rx="1"/><rect x="10" y="10" width="4" height="10" rx="1"/><rect x="17" y="5" width="4" height="15" rx="1"/>',
    lista_gen: '<rect x="9" y="2.5" width="6" height="4.5" rx="1"/><rect x="2.5" y="16" width="6" height="4.5" rx="1"/><rect x="15.5" y="16" width="6" height="4.5" rx="1"/><path d="M12 7v4.5M5.5 16v-4.5h13V16"/>',
    lista_cruz: '<rect x="3" y="3" width="18" height="18" rx="2"/><path d="M3 9h18M3 15h18M9 3v18M15 3v18"/>',
  };
  const icone = (tipo) => `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" aria-hidden="true">${ICONES[tipo]}</svg>`;

  // ------------------------------------------------------------ peças
  function no(item, o = {}) {
    const cls = ['no', ...(o.cls || [])].join(' ');
    const chave = o.chave !== undefined ? o.chave : item.ano ?? '—';
    const indice = o.indice !== undefined ? `<span class="no-indice">[${o.indice}]</span>` : '';
    return `<div class="${cls}" data-id="${item.id}" title="${esc(nomeDo(item))}">${indice}` +
      `<span class="no-nome">${esc(curto(nomeDo(item), 26))}</span><span class="no-chave">${esc(chave)}</span></div>`;
  }

  /* Quais posições mostrar numa lista longa: começo, fim e uma janela em
     volta do foco. Devolve índices e objetos {gap: n} para os trechos ocultos. */
  function janela(n, foco, max) {
    if (n <= max) return [...Array(n).keys()];
    const s = new Set([0, n - 1]);
    if (foco !== null && foco >= 0) {
      s.add(1); s.add(n - 2);
      for (let i = foco - 2; i <= foco + 2; i++) if (i >= 0 && i < n) s.add(i);
    } else {
      for (let i = 0; i < max - 3; i++) s.add(i);
      s.add(n - 2);
    }
    const ord = [...s].sort((a, b) => a - b);
    const out = [];
    ord.forEach((v, k) => {
      if (k > 0 && v - ord[k - 1] > 1) out.push({ gap: v - ord[k - 1] - 1 });
      out.push(v);
    });
    return out;
  }

  const lacuna = (g) => `<span class="gap-nos" title="nós escondidos para caber na tela">··· ${plural(g, 'nó', 'nós')} ···</span>`;
  const seta = '<span class="seta" aria-hidden="true">→</span>';

  function indiceDo(itens, d) {
    if (!d) return -1;
    return itens.findIndex((it) => it.id === d.id);
  }

  // ------------------------------------------------------------ cadeias lineares
  function cadeia(itens, o) {
    const n = itens.length;
    const p = [];
    p.push(`<span class="ponteiro">${o.rotuloInicio}</span>${seta}`);
    if (o.sentinela) {
      p.push(`<div class="no sentinela" title="nó cabeça (sentinela)"><span class="no-nome">cabeça</span><span class="no-chave">sem dado</span></div>${seta}`);
    }
    for (const pos of janela(n, o.foco, o.compacto ? 7 : 9)) {
      if (typeof pos === 'object') { p.push(lacuna(pos.gap) + seta); continue; }
      const cls = [];
      if (pos === o.foco) cls.push(o.modo === 'percorre' ? 'foco' : 'novo');
      if (o.comparados && o.comparados.has(pos) && pos !== o.foco) cls.push('comparado');
      let html = no(itens[pos], { cls, chave: o.chave ? o.chave(pos) : undefined });
      if (pos === n - 1 && o.marcaFim) html = `<div class="marcador">${html}<span class="ponteiro">fim</span></div>`;
      p.push(html + seta);
    }
    p.push('<span class="nulo">NULL</span>');
    return `<div class="cadeia">${p.join('')}</div>`;
  }

  function linear(e, d, opc, extra = {}) {
    const itens = [...e.iterar()];
    const foco = indiceDo(itens, d);
    return cadeia(itens, {
      rotuloInicio: 'início', marcaFim: true, foco: foco >= 0 ? foco : null,
      modo: d && d.modo, compacto: opc.compacto, ...extra,
    });
  }

  // ------------------------------------------------------------ pilha
  function pilha(e, d, opc) {
    const itens = [...e.iterar()]; // do topo para o fundo
    const n = itens.length;
    const foco = indiceDo(itens, d);
    if (n === 0) return vazia('topo → NULL');
    const linhas = janela(n, foco >= 0 ? foco : null, opc.compacto ? 5 : 7).map((pos) => {
      if (typeof pos === 'object') return lacuna(pos.gap);
      const cls = pos === foco ? [d.modo === 'percorre' ? 'foco' : 'novo'] : [];
      return no(itens[pos], { cls });
    });
    return `<div class="pilha-viz"><span class="ponteiro">topo ↓</span>` +
      `<div class="pilha-coluna">${linhas.join('')}</div><div class="pilha-fundo"></div>` +
      `<span class="pilha-rot">fundo: o primeiro que entrou</span></div>`;
  }

  // ------------------------------------------------------------ lista sequencial
  function seq(e, d, opc) {
    const n = e.tamanho;
    const cap = e.capacidade;
    let foco = -1;
    if (d) for (let i = 0; i < n; i++) if (e.itens[i].id === d.id) { foco = i; break; }
    const cresceu = d && d.modo === 'novo' && d.info && d.info.cresceu;
    const celulas = [];
    for (let i = 0; i < cap; i++) {
      const cls = ['cel'];
      if (i < n) cls.push('cheia');
      else if (cresceu && i >= cresceu.de) cls.push('nova-cap');
      if (i === foco) cls.push('foco');
      const t = i < n ? `[${i}] ${nomeDo(e.itens[i])}` : `[${i}] livre`;
      celulas.push(`<span class="${cls.join(' ')}" title="${esc(t)}"></span>`);
    }
    const faixa = n === 0 ? '' : janela(n, foco >= 0 ? foco : null, opc.compacto ? 6 : 8).map((pos) => {
      if (typeof pos === 'object') return lacuna(pos.gap);
      const cls = pos === foco ? [d.modo === 'percorre' ? 'foco' : 'novo'] : [];
      return no(e.itens[pos], { cls, indice: pos });
    }).join('');
    const aviso = cresceu
      ? `<div class="aviso-cresceu">Encheu! A capacidade passou de ${cresceu.de} para ${cresceu.para}: o realloc copiou ${plural(cresceu.copias, 'ponteiro', 'ponteiros')} para o vetor novo.</div>`
      : '';
    return `<div class="seq-numeros"><span><b>${n}</b> ocupadas</span><span><b>${cap}</b> de capacidade</span><span><b>${cap - n}</b> livres</span></div>` +
      aviso + `<div class="seq-grade" aria-label="Vetor com ${cap} posições, ${n} ocupadas">${celulas.join('')}</div>` +
      `<div class="seq-faixa">${faixa}</div>`;
  }

  // ------------------------------------------------------------ lista ordenada
  function ord(e, d, opc) {
    const nos = [...e.nos()];
    const itens = nos.map((x) => x.item);
    const n = nos.length;
    const foco = indiceDo(itens, d);
    const fmtChave = (x) => {
      if (!x.tem) return '(sem chave)';
      if (x.texto !== null) return curto(x.texto, 18);
      return e.usaSimilaridade ? `nota ${x.chave.toFixed(2).replace('.', ',')}` : String(x.chave);
    };
    // Nós comparados durante a última inserção (posições já na lista final)
    let comparados = null;
    if (d && d.modo === 'novo' && d.info && foco >= 0) {
      comparados = new Set();
      const c = d.info.caminho;
      if (c === 'fim') comparados.add(foco - 1);
      else if (c === 'inicio') { comparados.add(n - 1); comparados.add(1); }
      else if (c === 'meio') {
        comparados.add(n - 1); comparados.add(0);
        for (let i = 1; i < foco; i++) comparados.add(i);
        comparados.add(foco + 1);
      }
    }
    const campo = e.usaSimilaridade ? 'similarity' : e.campo;
    const legenda = `<div class="viz-legenda"><span>ordenada por <code>${esc(campo)}</code> ${e.desc ? '↓ DESC' : '↑ ASC'}</span>` +
      (comparados ? `<span><span class="amostra novo"></span>novo nó</span><span><span class="amostra comparado"></span>comparado</span>` : '') + '</div>';
    if (n === 0) return legenda + vazia('início → NULL');
    return legenda + cadeia(itens, {
      rotuloInicio: 'início', marcaFim: true, foco: foco >= 0 ? foco : null, modo: d && d.modo,
      compacto: opc.compacto, comparados, chave: (pos) => fmtChave(nos[pos]),
    });
  }

  // ------------------------------------------------------------ lista generalizada
  function gen(e, d, opc) {
    const campos = e.campos;
    // chaves do caminho destacado (inserção nova ou item sendo percorrido)
    let caminho = null;
    let novas = [];
    if (d) {
      const item = d.item;
      caminho = campos.map((c) => window.Motor.chaveDoItem(item, c));
      if (d.modo === 'novo' && d.info && d.info.caminho) novas = d.info.caminho.map((x) => x.criada);
    }
    const maxSub = opc.compacto ? 6 : 10;
    const maxAtomos = opc.compacto ? 3 : 5;

    function desenha(no, nivel, noCaminho) {
      const subs = no.filhos.filter((f) => f.sub);
      const atomos = no.filhos.filter((f) => !f.sub);
      const partes = [];
      let mostradas = subs.slice(0, maxSub);
      if (noCaminho && caminho && nivel < campos.length) {
        const alvo = subs.find((s) => s.chave === caminho[nivel]);
        if (alvo && !mostradas.includes(alvo)) mostradas = [...mostradas.slice(0, maxSub - 1), alvo];
      }
      for (const s of mostradas) {
        const naTrilha = noCaminho && caminho && s.chave === caminho[nivel];
        const nova = naTrilha && novas[nivel];
        partes.push(`<li class="${naTrilha ? 'caminho' : ''}"><div class="gen-no"><span class="gen-campo">${esc(campos[nivel])} =</span>` +
          `<span class="gen-chave" title="${esc(s.chave)}">${esc(curto(s.chave, 34))}</span><span class="cont">${s.total}</span>` +
          `${nova ? '<span class="selo-novo">nova</span>' : ''}</div>${desenha(s, nivel + 1, naTrilha)}</li>`);
      }
      if (subs.length > mostradas.length) partes.push(`<li><span class="mais-sub">+ ${plural(subs.length - mostradas.length, 'sublista', 'sublistas')}</span></li>`);
      if (atomos.length) {
        let vis = atomos.slice(0, maxAtomos);
        const alvo = d && noCaminho ? atomos.find((a) => a.item.id === d.id) : null;
        if (alvo && !vis.includes(alvo)) vis = [...vis.slice(0, maxAtomos - 1), alvo];
        const chips = vis.map((a) => {
          const cls = alvo === a ? (d.modo === 'percorre' ? 'foco' : 'novo') : '';
          return `<span class="atomo ${cls}" title="${esc(nomeDo(a.item))}">${esc(curto(nomeDo(a.item), 22))}</span>`;
        });
        if (atomos.length > vis.length) chips.push(`<span class="atomo mais">+${atomos.length - vis.length}</span>`);
        partes.push(`<li><div class="atomos">${chips.join('')}</div></li>`);
      }
      return partes.length ? `<ul class="${nivel === 0 ? 'arvore' : ''}">${partes.join('')}</ul>` : '';
    }

    const niveis = campos.length
      ? `níveis: ${campos.map((c) => `<code>${esc(c)}</code>`).join(' › ')}`
      : 'sem GROUP na consulta: todos os itens ficam direto na raiz';
    if (e.tamanho === 0) return `<div class="viz-legenda">${niveis}</div>` + vazia('raiz sem filhos');
    return `<div class="viz-legenda">${niveis}</div><div class="gen-viz"><div class="gen-no raiz">raiz <span class="cont">${e.tamanho}</span></div>${desenha(e.raiz, 0, true)}</div>`;
  }

  // ------------------------------------------------------------ lista cruzada
  function cruz(e, d, opc) {
    const L = e.linhas;
    const C = e.colunas;
    const semCross = e.campoLinha === '';
    const titulo = semCross
      ? 'sem CROSS na consulta: tudo cai numa única linha e coluna “N/A”'
      : `linhas: <code>${esc(e.campoLinha)}</code> · colunas: <code>${esc(e.campoColuna)}</code> · percorrer por <b>${opc.porColuna ? 'coluna ↓' : 'linha →'}</b> <code>--view ${opc.porColuna ? 'col' : 'row'}</code>`;
    if (e.tamanho === 0) return `<div class="viz-legenda">${titulo}</div>` + vazia('nenhuma linha nem coluna ainda');

    const idxCol = new Map(C.map((c, i) => [c, i]));
    const mat = L.map((l) => {
      const linha = new Array(C.length).fill(0);
      for (const nn of l.nos) linha[idxCol.get(nn.coluna)]++;
      return linha;
    });
    let maior = 1;
    for (const r of mat) for (const v of r) if (v > maior) maior = v;

    let fl = -1;
    let fc = -1;
    if (d) {
      const kl = window.Motor.chaveDoItem(d.item, e.campoLinha);
      const kc = window.Motor.chaveDoItem(d.item, e.campoColuna);
      fl = L.findIndex((l) => l.chave === kl);
      fc = C.findIndex((c) => c.chave === kc);
    }
    const novo = d && d.modo === 'novo';
    const percorre = d && d.modo === 'percorre';

    const cabCols = C.map((c, j) => {
      const cls = ['desc-col'];
      if (novo && j === fc && d.info && d.info.novaColuna) cls.push('nova');
      if (percorre && opc.porColuna && j === fc) cls.push('percorre');
      return `<th class="${cls.join(' ')}" title="${esc(c.chave)} (${c.nos.length})">${esc(curto(c.chave, 40))}</th>`;
    }).join('');
    const corpo = L.map((l, i) => {
      const clsL = ['desc-lin'];
      if (novo && i === fl && d.info && d.info.novaLinha) clsL.push('nova');
      if (percorre && !opc.porColuna && i === fl) clsL.push('percorre');
      const tds = mat[i].map((v, j) => {
        const cls = [v ? 'cheia' : 'zero'];
        const p = v / maior;
        if (v && p > 0.62) cls.push('escura');
        if (i === fl && j === fc) cls.push(novo ? 'novo' : 'foco');
        if (percorre && ((opc.porColuna && j === fc) || (!opc.porColuna && i === fl))) cls.push('trilha');
        return `<td class="${cls.join(' ')}" style="--p:${p.toFixed(3)}" title="${esc(l.chave)} × ${esc(C[j].chave)}: ${plural(v, 'bem', 'bens')}">${v || '·'}</td>`;
      }).join('');
      return `<tr><th class="${clsL.join(' ')}" title="${esc(l.chave)} (${l.nos.length})">${esc(curto(l.chave, 22))}</th>${tds}</tr>`;
    }).join('');
    const canto = semCross ? '' : `${esc(e.campoLinha)} →<br>${esc(e.campoColuna)} ↓`;
    return `<div class="viz-legenda">${titulo}</div><div class="cruz-viz"><table>` +
      `<thead><tr><th class="canto">${canto}</th>${cabCols}</tr></thead><tbody>${corpo}</tbody></table></div>`;
  }

  function vazia(txt) {
    return `<div class="viz-vazia"><span>(vazia)</span><span class="ponteiro">${esc(txt)}</span></div>`;
  }

  function desenhar(tipo, e, d, opc = {}) {
    switch (tipo) {
      case 'pilha': return pilha(e, d, opc);
      case 'fila': return e.tamanho ? linear(e, d, opc) : vazia('início → NULL');
      case 'lista_enc': return e.tamanho ? linear(e, d, opc) : vazia('início → NULL');
      case 'lista_cabeca': return linear(e, d, opc, { sentinela: true, marcaFim: e.tamanho > 0 });
      case 'lista_seq': return seq(e, d, opc);
      case 'lista_ord': return ord(e, d, opc);
      case 'lista_gen': return gen(e, d, opc);
      case 'lista_cruz': return cruz(e, d, opc);
    }
    return '';
  }

  window.Viz = { desenhar, INFO, icone, esc, curto, nomeDo, plural };
})();
