#!/usr/bin/env node
/* Confere que o motor em JavaScript (site/motor.js) produz exatamente a mesma
 * saída que o programa em C, para várias consultas e combinações de estruturas.
 *
 * Uso (a partir da pasta do trabalho, depois de `make`):
 *   node site/ferramentas/verificar.js [caminho/do/trabalho] [pasta-temporaria]
 */
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { execFileSync } = require('child_process');

const RAIZ = path.resolve(__dirname, '..', '..');
const BINARIO = path.resolve(process.argv[2] || path.join(RAIZ, 'trabalho'));
const TMP = fs.mkdtempSync(path.join(process.argv[3] || os.tmpdir(), 'verifica-'));
const CSV = path.join(RAIZ, 'dados', 'patrimonio.csv');

const Motor = require(path.join(RAIZ, 'site', 'motor.js'));
const base = Motor.lerCSV(fs.readFileSync(CSV, 'utf8'));

const CONSULTAS = {
  tudo: 'FILTER|ano_tombamento|>=|0\n',
  vazia: 'FILTER|ano_tombamento|<|0\n',
  campinas: 'FILTER|municipio|==|Campinas\nFILTER|ano_tombamento|>=|1980\n',
  sp_grupos: 'FILTER|municipio|==|São Paulo\nFILTER|ano_tombamento|>=|1970\nORDER|ano_tombamento|ASC\nGROUP|tipo_protecao|livro_tombo\nCROSS|tipo_protecao|livro_tombo\n',
  adv: 'FILTER|ano_tombamento|>=|0\nORDER|ano_tombamento|ASC\nGROUP|tipo_protecao|municipio\nCROSS|tipo_protecao|ano_tombamento\n',
  sim: 'FILTER|ano_tombamento|<=|1990\nSIM|ano_tombamento|1975|15|1.0\nACCEPT|0.5\nORDER|similarity|DESC\n',
  sim2: 'SIM|ano_tombamento|2000|8|2\nSIM|ano_tombamento|1982|3|1\nACCEPT|0.2\nGROUP|regiao_administrativa|tipo_protecao|livro_tombo\nCROSS|regiao_administrativa|tipo_area_envoltoria\n',
  nome_desc: '# comentario\n  FILTER | tipo_protecao | != | t  \r\nORDER|bem|DESC\nGROUP|municipio\nCROSS|livro_tombo|municipio\n',
  texto_ord: 'FILTER|bem|>=|Igreja\nFILTER|bem|<|J\nORDER|municipio|ASC\nCROSS|ano_tombamento|regiao_administrativa\n',
  regiao: 'FILTER|regiao_administrativa|==|Região Metropolitana da Baixada Santista\nORDER|ano_tombamento|DESC\nGROUP|tipo_area_envoltoria|ano_tombamento\nCROSS|tipo_area_envoltoria|ano_tombamento\n',
  campo_inexistente: 'ORDER|nao_existe|ASC\nGROUP|nao_existe\nCROSS|nao_existe|tipo_protecao\n',
  sem_comandos: '',
  ord_texto_num: 'ORDER|observacoes|ASC\nGROUP|observacoes\n',
};
const ESTRUTURAS = Motor.NOMES_ESTRUTURAS;

function rodaC(nomeQ, cfg) {
  const qPath = path.join(TMP, `${nomeQ}.txt`);
  const out = path.join(TMP, 'saida.csv');
  fs.writeFileSync(qPath, CONSULTAS[nomeQ]);
  const args = ['--reader', 'patrimonio', '--input', CSV, '--base', cfg.base, '--result', cfg.resultado,
    '--query', qPath, '--output', out, '--top-k', String(cfg.topK), '--limit', String(cfg.limite),
    '--view', cfg.porColuna ? 'col' : 'row'];
  const stdout = execFileSync(BINARIO, args, { encoding: 'utf8' });
  const st = Object.fromEntries(stdout.trim().split('\n').map((l) => l.split('=')));
  return { st, csv: fs.readFileSync(out, 'utf8') };
}

let total = 0;
let falhas = 0;
for (const nomeQ of Object.keys(CONSULTAS)) {
  const carregada = Motor.carregaQuery(CONSULTAS[nomeQ]);
  if (!carregada.ok) { console.log(`consulta ${nomeQ} inválida no JS: ${carregada.erro}`); falhas++; continue; }
  for (const b of ESTRUTURAS) {
    for (const r of ESTRUTURAS) {
      for (const [topK, limite, porColuna] of [[0, 0, false], [7, 0, true], [0, 60, true]]) {
        const cfg = { query: carregada.query, base: b, resultado: r, topK, limite, porColuna };
        const c = rodaC(nomeQ, cfg);
        const fim = Motor.executarTudo(base, cfg);
        const csvJS = Motor.textoSaida(base, fim.saida);
        total++;
        const ok = c.st.STATUS === 'OK' &&
          Number(c.st.INPUT_RECORDS) === fim.st.lidos &&
          Number(c.st.MATCHED_RECORDS) === fim.st.casados &&
          Number(c.st.OUTPUT_RECORDS) === fim.st.saida &&
          c.csv === csvJS;
        if (!ok) {
          falhas++;
          if (falhas <= 10) {
            console.log(`DIFERENTE: ${nomeQ} base=${b} result=${r} top-k=${topK} limit=${limite} view=${porColuna ? 'col' : 'row'}`);
            console.log('  C :', c.st.INPUT_RECORDS, c.st.MATCHED_RECORDS, c.st.OUTPUT_RECORDS, ' JS:', fim.st.lidos, fim.st.casados, fim.st.saida);
          }
        }
      }
    }
  }
}

// consultas inválidas: o C deve recusar e o JS também
const INVALIDAS = ['FILTER|ano_tombamento|=>|3\n', 'ORDER|bem|asc\n', 'ACCEPT|1.5\n', 'SIM|ano_tombamento|1|0|1\n',
  'GROUP|a|b|c|d|e\n', 'CROSS|a\n', 'XYZ|1\n', 'FILTER|bem|==\n'];
for (const q of INVALIDAS) {
  total++;
  fs.writeFileSync(path.join(TMP, 'inv.txt'), q);
  let statusC;
  try {
    statusC = execFileSync(BINARIO, ['--reader', 'patrimonio', '--input', CSV, '--base', 'fila', '--result', 'fila',
      '--query', path.join(TMP, 'inv.txt'), '--output', path.join(TMP, 'x.csv')], { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] });
  } catch (e) { statusC = e.stdout; }
  const jsOk = Motor.carregaQuery(q).ok;
  if (statusC.includes('STATUS=OK') !== jsOk) { falhas++; console.log('validação diferente para', JSON.stringify(q)); }
}

fs.rmSync(TMP, { recursive: true, force: true });
console.log(`${total - falhas}/${total} execuções idênticas ao programa em C`);
process.exit(falhas ? 1 : 0);
