#!/usr/bin/env python3
"""Automatiza o relatorio experimental do Projeto 1.

Roda ./trabalho varias vezes, extrai LOAD_MS, SEARCH_MS, RESULT_INSERT_MS e
WRITE_MS da saida padrao, calcula a media de cada configuracao e gera:
  resultados/execucoes.csv  -> todas as execucoes individuais
  resultados/medias.csv     -> media por configuracao
  resultados/tabelas.md     -> quadros prontos para colar no relatorio
  resultados/ambiente.txt   -> CPU, memoria, SO e compilador
  resultados/*.png          -> graficos (se matplotlib estiver instalado)

Exemplos:
  python3 experimentos.py --query consultas/teste.txt
  python3 experimentos.py --query consultas/teste.txt --rapido
  python3 experimentos.py --reader patrimonio --input dados/patrimonio.csv \
        --query consultas/p2_adv.txt --tamanhos 100,200,300,400,0 --ks 10,50,100,200,0
"""
import argparse
import csv
import os
import platform
import statistics
import subprocess
import sys
import tempfile

ESTRUTURAS = ["pilha", "fila", "lista_seq", "lista_enc",
              "lista_cabeca", "lista_ord", "lista_gen", "lista_cruz"]
TEMPOS = ["LOAD_MS", "SEARCH_MS", "RESULT_INSERT_MS", "WRITE_MS"]


def roda_uma_vez(a, base, result, n, k):
    """Executa o programa uma vez e devolve os valores impressos."""
    with tempfile.NamedTemporaryFile(suffix=".csv", delete=False) as tmp:
        saida = tmp.name
    cmd = [a.exe, "--reader", a.reader, "--input", a.input,
           "--base", base, "--result", result, "--query", a.query,
           "--output", saida, "--top-k", str(k), "--limit", str(n)]
    # LC_ALL=C garante ponto decimal na saida
    env = dict(os.environ, LC_ALL="C")
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, env=env)
    finally:
        if os.path.exists(saida):
            os.remove(saida)
    d = {}
    for linha in p.stdout.splitlines():
        if "=" in linha:
            chave, valor = linha.split("=", 1)
            d[chave.strip()] = valor.strip()
    if p.returncode != 0 or d.get("STATUS") != "OK":
        raise RuntimeError("falha em: " + " ".join(cmd) + "\n" + p.stderr[-300:])
    return d


def mede(a, n, base, result, k):
    """Roda a configuracao a.runs vezes e devolve as medias."""
    execs = []
    for i in range(a.runs):
        d = roda_uma_vez(a, base, result, n, k)
        linha = {t: float(d[t]) for t in TEMPOS}
        linha["TOTAL"] = sum(linha[t] for t in TEMPOS)
        linha["INPUT"] = int(d["INPUT_RECORDS"])
        linha["MATCHED"] = int(d["MATCHED_RECORDS"])
        linha["OUTPUT"] = int(d["OUTPUT_RECORDS"])
        execs.append(linha)
    media = {t: statistics.mean(e[t] for e in execs) for t in TEMPOS + ["TOTAL"]}
    media["DESVIO_TOTAL"] = statistics.pstdev(e["TOTAL"] for e in execs)
    media["INPUT"] = execs[0]["INPUT"]
    media["MATCHED"] = execs[0]["MATCHED"]
    media["OUTPUT"] = execs[0]["OUTPUT"]
    return execs, media


def lista_int(texto):
    return [int(x) for x in texto.split(",") if x.strip() != ""]


def monta_experimentos(a):
    """Cada experimento: (id, titulo, lista de (n, base, result, k))."""
    ns, ks = a.tamanhos, a.ks
    n_fixo = a.n_fixo
    k_fixo = a.k_fixo
    exps = []

    # A: tabela do enunciado, N e k crescendo juntos
    exps.append(("A", "Variacao de N e de k (base lista_enc, resultado lista_ord)",
                 [(n, "lista_enc", "lista_ord", k) for n, k in zip(ns, ks)]))
    # B: mesma consulta, so muda a estrutura base
    exps.append(("B", "Estruturas como BASE (resultado lista_enc, N=%s, k=%d)"
                 % ("toda a base" if n_fixo == 0 else n_fixo, k_fixo),
                 [(n_fixo, b, "lista_enc", k_fixo) for b in ESTRUTURAS]))
    # C: mesma consulta, so muda a estrutura de resultado
    exps.append(("C", "Estruturas como RESULTADO (base lista_enc, N=%s, k=%d)"
                 % ("toda a base" if n_fixo == 0 else n_fixo, k_fixo),
                 [(n_fixo, "lista_enc", r, k_fixo) for r in ESTRUTURAS]))
    # D: so o top-k varia
    exps.append(("D", "Variacao de k (base lista_enc, resultado lista_ord, N=%s)"
                 % ("toda a base" if n_fixo == 0 else n_fixo),
                 [(n_fixo, "lista_enc", "lista_ord", k) for k in ks]))
    return exps


def fmt(x):
    return "%.3f" % x


def tabela_md(titulo, linhas):
    out = ["### " + titulo, "",
           "| N | Base | Resultado | k | Casados | Carga (ms) | Busca (ms) | Insercao res. (ms) | Saida (ms) | Total (ms) | Desvio total |",
           "|---|---|---|---|---|---|---|---|---|---|---|"]
    for (n, base, result, k, m) in linhas:
        out.append("| %s | %s | %s | %s | %d | %s | %s | %s | %s | %s | %s |" % (
            "toda" if n == 0 else n, base, result, "todos" if k == 0 else k,
            m["MATCHED"], fmt(m["LOAD_MS"]), fmt(m["SEARCH_MS"]),
            fmt(m["RESULT_INSERT_MS"]), fmt(m["WRITE_MS"]), fmt(m["TOTAL"]),
            fmt(m["DESVIO_TOTAL"])))
    out.append("")
    return "\n".join(out)


def grava_ambiente(a, caminho):
    linhas = ["Sistema: " + platform.platform(), "Python: " + platform.python_version()]
    try:
        for l in open("/proc/cpuinfo"):
            if l.startswith("model name"):
                linhas.append("CPU: " + l.split(":", 1)[1].strip())
                break
        for l in open("/proc/meminfo"):
            if l.startswith("MemTotal"):
                kb = int(l.split()[1])
                linhas.append("Memoria: %.1f GB" % (kb / 1024 / 1024))
                break
    except OSError:
        pass
    try:
        v = subprocess.run(["gcc", "--version"], capture_output=True, text=True)
        linhas.append("gcc: " + v.stdout.splitlines()[0])
    except OSError:
        pass
    if os.path.exists("Makefile"):
        for l in open("Makefile"):
            if "CFLAGS" in l and "=" in l:
                linhas.append("Makefile: " + l.strip())
                break
    linhas.append("Consulta: " + a.query)
    linhas.append("Entrada: " + a.input + " (reader " + a.reader + ")")
    linhas.append("Execucoes por configuracao: %d" % a.runs)
    with open(caminho, "w") as f:
        f.write("\n".join(linhas) + "\n")


def graficos(pasta, resultados):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib nao instalado: graficos ignorados (pip install matplotlib)")
        return
    nomes = {"LOAD_MS": "Carga", "SEARCH_MS": "Busca",
             "RESULT_INSERT_MS": "Insercao res.", "WRITE_MS": "Saida"}

    # A: tempo x N (so as linhas com N > 0)
    if "A" in resultados:
        linhas = [l for l in resultados["A"] if l[0] > 0]
        if linhas:
            plt.figure(figsize=(7, 4.5))
            xs = [l[0] for l in linhas]
            for t in TEMPOS + ["TOTAL"]:
                plt.plot(xs, [l[4][t] for l in linhas], marker="o",
                         label=nomes.get(t, "Total"))
            plt.xlabel("N (registros carregados)")
            plt.ylabel("tempo medio (ms)")
            plt.title("Experimento A: tempo x N")
            plt.legend()
            plt.grid(True, alpha=0.3)
            plt.tight_layout()
            plt.savefig(os.path.join(pasta, "grafico_A_tempo_por_N.png"), dpi=150)
            plt.close()

    # B e C: barras empilhadas por estrutura
    for exp, indice, rotulo in (("B", 1, "base"), ("C", 2, "resultado")):
        if exp not in resultados:
            continue
        linhas = resultados[exp]
        plt.figure(figsize=(8, 4.5))
        fundo = [0.0] * len(linhas)
        for t in TEMPOS:
            vals = [l[4][t] for l in linhas]
            plt.bar([l[indice] for l in linhas], vals, bottom=fundo, label=nomes[t])
            fundo = [f + v for f, v in zip(fundo, vals)]
        plt.ylabel("tempo medio (ms)")
        plt.title("Experimento %s: estrutura como %s" % (exp, rotulo))
        plt.xticks(rotation=30, ha="right")
        plt.legend()
        plt.tight_layout()
        plt.savefig(os.path.join(pasta, "grafico_%s_%s.png" % (exp, rotulo)), dpi=150)
        plt.close()

    # D: tempo x k
    if "D" in resultados:
        linhas = [l for l in resultados["D"] if l[3] > 0]
        if linhas:
            plt.figure(figsize=(7, 4.5))
            xs = [l[3] for l in linhas]
            plt.plot(xs, [l[4]["WRITE_MS"] for l in linhas], marker="o", label="Saida")
            plt.plot(xs, [l[4]["TOTAL"] for l in linhas], marker="o", label="Total")
            plt.xlabel("top-k")
            plt.ylabel("tempo medio (ms)")
            plt.title("Experimento D: tempo x k")
            plt.legend()
            plt.grid(True, alpha=0.3)
            plt.tight_layout()
            plt.savefig(os.path.join(pasta, "grafico_D_tempo_por_k.png"), dpi=150)
            plt.close()


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--exe", default="./trabalho")
    p.add_argument("--reader", default="nasa")
    p.add_argument("--input", default="dados/asteroids.csv")
    p.add_argument("--query", default="consultas/teste.txt")
    p.add_argument("--runs", type=int, default=3, help="execucoes por configuracao (minimo 3)")
    p.add_argument("--tamanhos", type=lista_int, default=[1000, 5000, 10000, 25000, 50000, 0],
                   help="valores de N do experimento A (0 = toda a base)")
    p.add_argument("--ks", type=lista_int, default=[10, 50, 100, 500, 1000, 0],
                   help="valores de k (0 = todos); mesmo tamanho de --tamanhos")
    p.add_argument("--n-fixo", dest="n_fixo", type=int, default=0,
                   help="N usado nos experimentos B, C e D (0 = toda a base)")
    p.add_argument("--k-fixo", dest="k_fixo", type=int, default=100,
                   help="k usado nos experimentos B e C")
    p.add_argument("--pasta", default="resultados")
    p.add_argument("--rapido", action="store_true",
                   help="teste rapido: N pequenos e 1 execucao (so para ver se funciona)")
    a = p.parse_args()

    if a.rapido:
        a.tamanhos, a.ks, a.runs, a.n_fixo = [200, 500, 1000], [10, 50, 100], 1, 1000
    if len(a.tamanhos) != len(a.ks):
        sys.exit("erro: --tamanhos e --ks precisam ter o mesmo numero de valores")
    if a.runs < 3 and not a.rapido:
        sys.exit("erro: o enunciado pede pelo menos 3 execucoes por configuracao")
    if not os.path.exists(a.exe):
        sys.exit("erro: executavel '%s' nao encontrado (rode make)" % a.exe)

    os.makedirs(a.pasta, exist_ok=True)
    grava_ambiente(a, os.path.join(a.pasta, "ambiente.txt"))

    # aquecimento: a primeira leitura do arquivo enche o cache do sistema
    roda_uma_vez(a, "lista_enc", "fila", min(a.n_fixo or 1000, 1000), 10)

    exps = monta_experimentos(a)
    resultados = {}
    f_exec = open(os.path.join(a.pasta, "execucoes.csv"), "w", newline="")
    f_med = open(os.path.join(a.pasta, "medias.csv"), "w", newline="")
    w_exec, w_med = csv.writer(f_exec), csv.writer(f_med)
    w_exec.writerow(["exp", "N", "base", "resultado", "k", "execucao"] + TEMPOS + ["TOTAL", "MATCHED", "OUTPUT"])
    w_med.writerow(["exp", "N", "base", "resultado", "k"] + TEMPOS + ["TOTAL", "DESVIO_TOTAL", "MATCHED", "OUTPUT"])
    md = []

    for exp_id, titulo, configs in exps:
        print("\n== Experimento %s: %s" % (exp_id, titulo))
        linhas = []
        for (n, base, result, k) in configs:
            try:
                execs, m = mede(a, n, base, result, k)
            except RuntimeError as e:
                print("   FALHOU: %s" % e)
                continue
            for i, e in enumerate(execs, 1):
                w_exec.writerow([exp_id, n, base, result, k, i] +
                                [fmt(e[t]) for t in TEMPOS] +
                                [fmt(e["TOTAL"]), e["MATCHED"], e["OUTPUT"]])
            w_med.writerow([exp_id, n, base, result, k] +
                           [fmt(m[t]) for t in TEMPOS] +
                           [fmt(m["TOTAL"]), fmt(m["DESVIO_TOTAL"]), m["MATCHED"], m["OUTPUT"]])
            linhas.append((n, base, result, k, m))
            print("   N=%-6s %-12s -> %-12s k=%-5s total=%9.3f ms (casados=%d)" % (
                n, base, result, k, m["TOTAL"], m["MATCHED"]))
        resultados[exp_id] = linhas
        md.append(tabela_md("Experimento %s: %s" % (exp_id, titulo), linhas))

    f_exec.close()
    f_med.close()
    with open(os.path.join(a.pasta, "tabelas.md"), "w") as f:
        f.write("\n".join(md))
    graficos(a.pasta, resultados)
    print("\nPronto. Arquivos em '%s/': tabelas.md, medias.csv, execucoes.csv, ambiente.txt e graficos .png" % a.pasta)


if __name__ == "__main__":
    main()