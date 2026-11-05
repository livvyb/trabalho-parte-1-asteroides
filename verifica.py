import csv, sys

OPS = {'==': lambda a, b: a == b, '!=': lambda a, b: a != b,
       '<': lambda a, b: a < b, '<=': lambda a, b: a <= b,
       '>': lambda a, b: a > b, '>=': lambda a, b: a >= b}

PATRIMONIO = ["bem", "tipo_protecao", "processo", "municipio", "cidades",
              "regiao_administrativa", "resolucao_tombamento",
              "tipo_area_envoltoria", "resolucao_area_envoltoria",
              "processos_vinculados", "ano_tombamento", "livro_tombo",
              "publicacao_doe", "observacoes", "ata_data_so"]
PATRIMONIO_NUMERICOS = {"ano_tombamento"}


def val_bool(s):
    if s.lower() in ('true', '1'):
        return 1.0
    if s.lower() in ('false', '0'):
        return 0.0
    return float(s)


def tenta_numero(s):
    try:
        return float(s)
    except ValueError:
        return None


def pega(row, campo, reader, cabecalho):
    if reader == 'patrimonio':
        if campo not in PATRIMONIO:
            return None, None
        raw = row[PATRIMONIO.index(campo)].strip()
        if raw == '':
            return None, None
        if campo in PATRIMONIO_NUMERICOS:
            try:
                return 'num', float(int(raw))
            except ValueError:
                return None, None
        return 'text', raw
    if campo not in cabecalho:
        return None, None
    raw = row[cabecalho.index(campo)].strip()
    if raw == '':
        return None, None
    if raw in ('True', 'False', 'true', 'false'):
        return 'num', val_bool(raw)
    n = tenta_numero(raw)
    if n is not None and n == n and abs(n) != float('inf'):
        return 'num', n
    return 'text', raw


if len(sys.argv) < 4:
    print("uso: python3 verifica.py ARQUIVO.csv CONSULTA.txt LIMITE [nasa|patrimonio]")
    sys.exit(1)

csv_path, qpath, limit = sys.argv[1], sys.argv[2], int(sys.argv[3])
reader = sys.argv[4] if len(sys.argv) > 4 else 'nasa'

filters, sims, accept = [], [], 0.0
for l in open(qpath, encoding='utf-8'):
    l = l.strip()
    if not l or l.startswith('#'):
        continue
    p = [x.strip() for x in l.split('|')]
    if p[0] == 'FILTER':
        filters.append(p[1:])
    elif p[0] == 'SIM':
        sims.append((p[1], float(p[2]), float(p[3]), float(p[4])))
    elif p[0] == 'ACCEPT':
        accept = float(p[1])

n = m = puladas = 0
with open(csv_path, newline='', encoding='utf-8-sig') as f:
    linhas = csv.reader(f)
    cabecalho = [c.strip() for c in next(linhas)]
    ncols = len(cabecalho)
    for row in linhas:
        if not row:
            continue
        if len(row) != ncols:
            puladas += 1
            continue
        if limit and n >= limit:
            break
        n += 1

        ok = True
        for campo, op, alvo in filters:
            tipo, x = pega(row, campo, reader, cabecalho)
            if tipo is None:
                ok = False
                break
            if tipo == 'num':
                try:
                    ok = OPS[op](x, val_bool(alvo))
                except ValueError:
                    ok = False
            else:
                ok = OPS[op](x.lower(), alvo.lower())
            if not ok:
                break
        if not ok:
            continue

        if sims:
            soma = pesos = 0.0
            for campo, q, t, w in sims:
                tipo, x = pega(row, campo, reader, cabecalho)
                if tipo != 'num':
                    ok = False
                    break
                s = max(0.0, 1 - abs(x - q) / t)
                soma += s * w
                pesos += w
            if not ok or pesos <= 0 or soma / pesos < accept:
                continue
        m += 1

print(f"INPUT_RECORDS={n} MATCHED_RECORDS={m} SKIPPED={puladas}")