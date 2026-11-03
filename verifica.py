import csv, sys
OPS = {'==':lambda a,b:a==b,'!=':lambda a,b:a!=b,'<':lambda a,b:a<b,
       '<=':lambda a,b:a<=b,'>':lambda a,b:a>b,'>=':lambda a,b:a>=b}

def val(s):
    if s in ('True', 'true'): return 1.0
    if s in ('False', 'false'): return 0.0
    return float(s)

csv_path, qpath, limit = sys.argv[1], sys.argv[2], int(sys.argv[3])
filters, sims, accept = [], [], 0.0
for l in open(qpath):
    l = l.strip()
    if not l or l.startswith('#'): continue
    p = [x.strip() for x in l.split('|')]
    if p[0] == 'FILTER': filters.append(p[1:])
    elif p[0] == 'SIM': sims.append((p[1], float(p[2]), float(p[3]), float(p[4])))
    elif p[0] == 'ACCEPT': accept = float(p[1])

n = m = 0
for row in csv.DictReader(open(csv_path, newline='', encoding='utf-8-sig')):
    if limit and n >= limit: break
    n += 1
    ok = True
    for f, op, v in filters:
        x = row[f]
        if x == '': ok = False; break
        try: ok = OPS[op](val(x), val(v))
        except ValueError: ok = OPS[op](x.lower(), v.lower())
        if not ok: break
    if not ok: continue
    if sims:
        sw = sp = 0.0
        for f, q, t, w in sims:
            if row[f] == '': ok = False; break
            s = max(0.0, 1 - abs(float(row[f]) - q) / t)
            sw += s * w; sp += w
        if not ok or sp <= 0 or sw / sp < accept: continue
    m += 1
print(f"INPUT_RECORDS={n} MATCHED_RECORDS={m}")
