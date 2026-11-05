import csv, sys

def carrega(caminho):
    with open(caminho, newline='', encoding='utf-8-sig') as f:
        linhas = list(csv.reader(f))
    return [c.strip() for c in linhas[0]], linhas[1:]

def indice(cab, col):
    return int(col) if col.isdigit() else cab.index(col)   # nome ou posicao (0 = 1a coluna)

cmd, arq = sys.argv[1], sys.argv[2]
cab, linhas = carrega(arq)

if cmd == "ord":                       # confere.py ord ARQ COLUNA asc|desc
    i = indice(cab, sys.argv[3])
    v = []
    for r in linhas:
        try: v.append(float(r[i]))
        except ValueError: pass        # sem valor: fica no fim, fora da conferencia
    esperado = sorted(v, reverse=(sys.argv[4] == "desc"))
    print("ORDENADO:", v == esperado, "| registros:", len(v))

elif cmd == "grupo":                   # confere.py grupo ARQ COL1 [COL2 ...]
    idx = [indice(cab, c) for c in sys.argv[3:]]
    ok = True
    for nivel in range(1, len(idx) + 1):
        chaves = [tuple(r[j].strip() for j in idx[:nivel]) for r in linhas]
        blocos = sum(1 for k, c in enumerate(chaves) if k == 0 or c != chaves[k - 1])
        print(f"nivel {nivel}: blocos={blocos} distintos={len(set(chaves))}")
        ok = ok and blocos == len(set(chaves))
    print("AGRUPADO:", ok)

elif cmd == "mesmos":                  # confere.py mesmos A.csv B.csv
    _, outras = carrega(sys.argv[3])
    print("MESMOS REGISTROS:", sorted(map(tuple, linhas)) == sorted(map(tuple, outras)),
          "| ordem igual:", linhas == outras)