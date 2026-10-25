#!/usr/bin/env python3
"""Compara los resultados de resumen.csv con el optimo EXACTO de cada instancia.

El optimo se calcula con programacion dinamica: f(t) = (1/N) * sum_clases S_c^2 / n_c - muT^2
es aditiva por clase, asi que se resuelve exacto en O(k * 256^2).

Uso (desde la carpeta del proyecto):
  python3 comparar_optimo.py [carpeta_instancias] [resumen.csv]
  python3 comparar_optimo.py imagenes_preprocesadas/instancias soluciones/resumen.csv
"""
import csv, glob, os, sys

carpeta = sys.argv[1] if len(sys.argv) > 1 else "imagenes_preprocesadas/instancias"
resumen = sys.argv[2] if len(sys.argv) > 2 else "soluciones/resumen.csv"


def leer_instancia(ruta):
    tok = open(ruta).read().split()
    k = int(tok[tok.index("k") + 1])
    h = [int(x) for x in tok[tok.index("histograma") + 1:][:256]]
    return k, h


def optimo(k, h):
    N = sum(h)
    P, S = [0] * 257, [0] * 257
    for g in range(256):
        P[g + 1] = P[g] + h[g]
        S[g + 1] = S[g] + g * h[g]
    muT = S[256] / N

    def costo(a, b):
        n = P[b] - P[a]
        return 0.0 if n == 0 else (S[b] - S[a]) ** 2 / n

    NEG = -1e18
    dp = [[NEG] * 257 for _ in range(k + 2)]
    arg = [[0] * 257 for _ in range(k + 2)]
    dp[0][0] = 0.0
    for m in range(1, k + 2):
        for b in range(1, 257):
            if m <= k and not (2 <= b <= 255):
                continue
            if m == k + 1 and b != 256:
                continue
            for a in range(b):
                if dp[m - 1][a] > NEG / 2 and (m == 1 or 2 <= a <= 255):
                    v = dp[m - 1][a] + costo(a, b)
                    if v > dp[m][b]:
                        dp[m][b], arg[m][b] = v, a
    b, ts = 256, []
    for m in range(k + 1, 1, -1):
        a = arg[m][b]
        ts.append(a - 1)
        b = a
    return dp[k + 1][256] / N - muT ** 2, sorted(ts)


filas = list(csv.DictReader(open(resumen)))
print(f"{'instancia':<14}{'f_final':>14}{'f_optimo':>14}{'diferencia':>12}  estado   umbrales optimos")
n_opt = 0
for r in filas:
    ruta = os.path.join(carpeta, r["instancia"] + ".txt")
    if not os.path.exists(ruta):
        print(f"{r['instancia']:<14} (no se encontro {ruta})")
        continue
    k, h = leer_instancia(ruta)
    fo, to = optimo(k, h)
    ff = float(r["f_final"])
    dif = fo - ff
    ok = dif < 1e-6
    n_opt += ok
    print(f"{r['instancia']:<14}{ff:>14.6f}{fo:>14.6f}{dif:>12.6f}  {'OPTIMO ' if ok else 'LOCAL  '}  {to}")
print(f"\nInstancias en el optimo exacto: {n_opt} de {len(filas)}")