#!/usr/bin/env python3
"""Gera os arquivos de dados do site (rodar a partir da pasta do trabalho):

    python3 site/ferramentas/gerar_dados.py

Saídas em site/:
  dados.js   -> o CSV de patrimônio embutido (texto original, linha a linha)
  mapa.js    -> contorno dos 645 municípios de SP já projetado em SVG
  codigo.js  -> trechos do código em C mostrados na interface

A malha vem do IBGE (servicodados.ibge.gov.br). Os arquivos baixados ficam em
site/ferramentas/ibge/; se não existirem, o script tenta baixar.
"""
import json
import math
import re
import sys
import unicodedata
import urllib.request
from pathlib import Path

RAIZ = Path(__file__).resolve().parents[2]       # pasta do trabalho
SITE = RAIZ / "site"
IBGE = SITE / "ferramentas" / "ibge"
CSV = RAIZ / "dados" / "patrimonio.csv"

URL_MALHA = ("https://servicodados.ibge.gov.br/api/v3/malhas/estados/35"
             "?formato=application/vnd.geo%2Bjson&qualidade=intermediaria&intrarregiao=municipio")
URL_NOMES = "https://servicodados.ibge.gov.br/api/v1/localidades/estados/35/municipios"

# Grafias do CSV que não batem com o nome oficial do IBGE
APELIDOS = {
    "embu": "embu das artes",
    "ilha bela": "ilhabela",
    "sao luis do paraitinga": "sao luiz do paraitinga",
    "santa rita do passo quatro": "santa rita do passa quatro",
    "nazareth paulista": "nazare paulista",
    "santa barbara": "santa barbara d oeste",
    "sao paulo (capital)": "sao paulo",
    "iguaracu do tiete": "igaracu do tiete",
    "sao manoel": "sao manuel",
}

LARGURA = 1000.0      # largura do viewBox do mapa
TOLERANCIA = 0.12     # simplificação (unidades do viewBox)


def normaliza(nome):
    s = unicodedata.normalize("NFD", nome)
    s = "".join(c for c in s if unicodedata.category(c) != "Mn")
    s = s.lower().replace("-", " ").replace("'", " ").replace("’", " ")
    return re.sub(r"\s+", " ", s).strip()


def baixa(url, destino):
    if destino.exists():
        return
    print(f"baixando {url} ...")
    destino.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url, timeout=120) as r:
        destino.write_bytes(r.read())


# ---------------------------------------------------------------- dados.js
def gera_dados(nome_para_cod):
    texto = CSV.read_bytes().decode("utf-8")
    js = ("// Gerado por site/ferramentas/gerar_dados.py a partir de dados/patrimonio.csv\n"
          "// O texto é o arquivo original: o leitor em motor.js faz o mesmo que reader.c\n"
          f"window.PATRIMONIO_CSV = {json.dumps(texto, ensure_ascii=False)};\n"
          f"window.NOME_PARA_IBGE = {json.dumps(nome_para_cod, ensure_ascii=False, sort_keys=True)};\n")
    (SITE / "dados.js").write_text(js, encoding="utf-8")


# ---------------------------------------------------------------- mapa.js
def dp(pontos, tol):
    """Douglas-Peucker iterativo."""
    if len(pontos) < 3:
        return pontos
    manter = [False] * len(pontos)
    manter[0] = manter[-1] = True
    pilha = [(0, len(pontos) - 1)]
    while pilha:
        a, b = pilha.pop()
        ax, ay = pontos[a]
        bx, by = pontos[b]
        dx, dy = bx - ax, by - ay
        n = math.hypot(dx, dy) or 1e-12
        pior, idx = -1.0, -1
        for i in range(a + 1, b):
            px, py = pontos[i]
            d = abs(dy * px - dx * py + bx * ay - by * ax) / n
            if d > pior:
                pior, idx = d, i
        if pior > tol:
            manter[idx] = True
            pilha.append((a, idx))
            pilha.append((idx, b))
    return [p for p, m in zip(pontos, manter) if m]


def caminho(aneis):
    """Path SVG com coordenadas relativas (arquivo menor)."""
    partes = []
    for anel in aneis:
        q = [(round(x * 10), round(y * 10)) for x, y in anel]   # décimos de unidade
        if len(q) < 3:
            continue
        s = [f"M{q[0][0]/10:g} {q[0][1]/10:g}"]
        px, py = q[0]
        rel = []
        for x, y in q[1:-1]:
            if (x, y) == (px, py):
                continue
            rel.append(f"{(x-px)/10:g} {(y-py)/10:g}")
            px, py = x, y
        s.append("l" + " ".join(rel) + "z")
        partes.append("".join(s))
    return "".join(partes).replace(" -", "-")


def area_centroide(anel):
    a = cx = cy = 0.0
    for (x0, y0), (x1, y1) in zip(anel, anel[1:] + anel[:1]):
        f = x0 * y1 - x1 * y0
        a += f
        cx += (x0 + x1) * f
        cy += (y0 + y1) * f
    a /= 2
    if abs(a) < 1e-9:
        return 0.0, anel[0][0], anel[0][1]
    return abs(a), cx / (6 * a), cy / (6 * a)


def gera_mapa():
    malha = json.loads((IBGE / "malha.json").read_text(encoding="utf-8"))
    nomes = {str(m["id"]): m["nome"] for m in json.loads((IBGE / "municipios.json").read_text(encoding="utf-8"))}

    # limites em lon/lat
    lons, lats = [], []
    for f in malha["features"]:
        g = f["geometry"]
        polys = [g["coordinates"]] if g["type"] == "Polygon" else g["coordinates"]
        for p in polys:
            for x, y in p[0]:
                lons.append(x)
                lats.append(y)
    lon0, lon1, lat0, lat1 = min(lons), max(lons), min(lats), max(lats)
    k = math.cos(math.radians((lat0 + lat1) / 2))
    escala = LARGURA / ((lon1 - lon0) * k)
    altura = (lat1 - lat0) * escala

    def proj(lon, lat):
        return ((lon - lon0) * k * escala, (lat1 - lat) * escala)

    municipios = []
    for f in malha["features"]:
        cod = f["properties"]["codarea"]
        g = f["geometry"]
        polys = [g["coordinates"]] if g["type"] == "Polygon" else g["coordinates"]
        aneis, melhor = [], (0, 0, 0)
        for p in polys:
            for i, anel in enumerate(p):
                pts = [proj(x, y) for x, y in anel]
                simp = dp(pts, TOLERANCIA)
                if len(simp) < 4:
                    simp = pts
                aneis.append(simp)
                if i == 0:
                    ac = area_centroide(pts[:-1])
                    if ac[0] > melhor[0]:
                        melhor = ac
        municipios.append({
            "c": cod,
            "n": nomes.get(cod, cod),
            "d": caminho(aneis),
            "x": round(melhor[1], 1),
            "y": round(melhor[2], 1),
        })

    js = ("// Gerado por site/ferramentas/gerar_dados.py a partir da malha municipal do IBGE\n"
          f"window.MAPA_SP = {json.dumps({'largura': LARGURA, 'altura': round(altura, 1), 'municipios': municipios}, ensure_ascii=False, separators=(',', ':'))};\n")
    (SITE / "mapa.js").write_text(js, encoding="utf-8")

    nome_para_cod = {normaliza(m["n"]): m["c"] for m in municipios}
    for apelido, oficial in APELIDOS.items():
        if oficial in nome_para_cod:
            nome_para_cod[normaliza(apelido)] = nome_para_cod[oficial]
    return nome_para_cod


# ---------------------------------------------------------------- codigo.js
def extrai_funcao(arquivo, nome):
    """Recorta a definição da função 'nome' (do comentário acima até a chave final)."""
    texto = (RAIZ / arquivo).read_text(encoding="utf-8")
    m = re.search(r"^[^\n;{}]*\b" + re.escape(nome) + r"\s*\([^;{]*\)\s*\{", texto, re.M)
    if not m:
        return f"/* {nome} não encontrada em {arquivo} */"
    inicio, i, nivel = m.start(), m.end() - 1, 0
    while i < len(texto):
        if texto[i] == "{":
            nivel += 1
        elif texto[i] == "}":
            nivel -= 1
            if nivel == 0:
                break
        i += 1
    # inclui comentários de linha logo acima
    linhas_antes = texto[:inicio].split("\n")
    coment = []
    while linhas_antes and linhas_antes[-1] == "":
        linhas_antes.pop()
    while linhas_antes and linhas_antes[-1].lstrip().startswith("//"):
        coment.insert(0, linhas_antes.pop())
    corpo = texto[inicio:i + 1]
    return ("\n".join(coment) + "\n" if coment else "") + corpo


TRECHOS = {
    "pilha":        [("pilha.c", "pilha_inserir"), ("pilha.c", "pilha_percorrer")],
    "fila":         [("fila.c", "fila_inserir"), ("fila.c", "fila_percorrer")],
    "lista_seq":    [("lista_seq.c", "lista_seq_inserir")],
    "lista_enc":    [("lista_enc.c", "lista_enc_inserir"), ("lista_enc.c", "lista_enc_percorrer")],
    "lista_cabeca": [("lista_cabeca.c", "criar_lista_cabeca"), ("lista_cabeca.c", "lista_cabeca_inserir")],
    "lista_ord":    [("lista_ord.c", "vem_antes"), ("lista_ord.c", "lista_ord_inserir")],
    "lista_gen":    [("lista_gen.c", "lista_gen_inserir"), ("lista_gen.c", "percorre_sub")],
    "lista_cruz":   [("lista_cruz.c", "lista_cruz_inserir"), ("lista_cruz.c", "lista_cruz_percorrer_por_coluna")],
    "query":        [("query.c", "avalia_filtro"), ("query.c", "avalia_query")],
    "main":         [("main.c", "visitante_busca"), ("main.c", "visitante_escrita")],
}


def gera_codigo():
    saida = {}
    for chave, funcs in TRECHOS.items():
        saida[chave] = [{"arquivo": a, "funcao": f, "codigo": extrai_funcao(a, f)} for a, f in funcs]
    js = ("// Gerado por site/ferramentas/gerar_dados.py: trechos copiados dos arquivos .c do trabalho\n"
          f"window.CODIGO_C = {json.dumps(saida, ensure_ascii=False, indent=1)};\n")
    (SITE / "codigo.js").write_text(js, encoding="utf-8")


def confere_nomes(nome_para_cod):
    import csv
    faltando = set()
    with CSV.open(encoding="utf-8", newline="") as f:
        linhas = list(csv.reader(f))[1:]
    for r in linhas:
        mun = normaliza(r[3])
        if mun not in nome_para_cod and mun not in ("bem movel", "diversos", "n/c"):
            faltando.add(r[3].strip())
        if r[4].strip() != "Não se aplica":
            for parte in r[4].split(";"):
                nome = re.split(r"\s[–-]\s", parte)[0]
                n = normaliza(nome)
                if n and n not in nome_para_cod:
                    faltando.add(nome.strip())
    if faltando:
        print("AVISO: nomes sem município no mapa:", sorted(faltando), file=sys.stderr)


# ---------------------------------------------------------------- experimentos.js
def gera_experimentos():
    """Médias medidas pelo experimentos.py (tempos reais do programa em C)."""
    import csv
    saida = {}
    for pasta, chave in (("resultados_p2", "patrimonio"), ("resultados_p1", "asteroides")):
        arq = RAIZ / pasta / "medias.csv"
        if not arq.exists():
            continue
        with arq.open(encoding="utf-8", newline="") as f:
            linhas = [r for r in csv.DictReader(f) if r["exp"] in ("B", "C")]
        ambiente = (RAIZ / pasta / "ambiente.txt")
        m = re.search(r"Execucoes por configuracao:\s*(\d+)", ambiente.read_text(encoding="utf-8")) if ambiente.exists() else None
        lidos = re.search(r"INPUT_RECORDS=(\d+)", (RAIZ / "exemplos" / "ex1.txt").read_text(encoding="utf-8")) \
            if chave == "asteroides" and (RAIZ / "exemplos" / "ex1.txt").exists() else None
        saida[chave] = {
            "execucoes": int(m.group(1)) if m else None,
            "lidos": int(lidos.group(1)) if lidos else None,
            "linhas": [{
                "exp": r["exp"], "base": r["base"], "resultado": r["resultado"],
                "carga_ms": float(r["LOAD_MS"]), "insercao_ms": float(r["RESULT_INSERT_MS"]),
                "casados": int(r["MATCHED"]),
            } for r in linhas],
        }
    js = ("// Gerado por site/ferramentas/gerar_dados.py a partir de resultados_p1/ e resultados_p2/medias.csv\n"
          f"window.EXPERIMENTOS = {json.dumps(saida, ensure_ascii=False)};\n")
    (SITE / "experimentos.js").write_text(js, encoding="utf-8")


def main():
    baixa(URL_MALHA, IBGE / "malha.json")
    baixa(URL_NOMES, IBGE / "municipios.json")
    nome_para_cod = gera_mapa()
    gera_dados(nome_para_cod)
    gera_codigo()
    gera_experimentos()
    confere_nomes(nome_para_cod)
    for nome in ("dados.js", "mapa.js", "codigo.js", "experimentos.js"):
        print(f"site/{nome}: {(SITE / nome).stat().st_size / 1024:.0f} KB")


if __name__ == "__main__":
    main()
