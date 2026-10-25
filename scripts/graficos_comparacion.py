#!/usr/bin/env python3
"""Compara solucion inicial ALEATORIA vs GREEDY aleatorizado (m=3) con Hill Climbing.
Lee los dos resumen.csv, agrupa por k y grafica el promedio de f_inicial, f_final,
iteraciones y evaluaciones.

Uso: python3 graficos_comparacion.py [resumenAleatorio.csv] [resumenGreedy.csv] [carpeta_salida]
"""
import csv, os, sys
from collections import defaultdict
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ale = sys.argv[1] if len(sys.argv) > 1 else "resumenAleatorio.csv"
gre = sys.argv[2] if len(sys.argv) > 2 else "resumenGreedy.csv"
out = sys.argv[3] if len(sys.argv) > 3 else "figuras"
os.makedirs(out, exist_ok=True)

C_ALE, C_GRE, INK, GRID = "#c4c4c4", "#1f1f1f", "#1F1F1F", "#E4E4E4"
plt.rcParams.update({"font.family": "DejaVu Sans", "axes.edgecolor": "#C4C4C4",
                     "axes.spines.top": False, "axes.spines.right": False,
                     "text.color": INK, "axes.labelcolor": INK, "xtick.color": INK, "ytick.color": INK})


def leer(ruta):
    d = defaultdict(lambda: defaultdict(list))
    for r in csv.DictReader(open(ruta)):
        if r["valida"] != "OK":
            continue
        for c in ("f_inicial", "f_final", "iteraciones", "evaluaciones"):
            d[int(r["k"])][c].append(float(r[c]))
    return d


A, G = leer(ale), leer(gre)
KS = sorted(A)
prom = lambda D, k, c: sum(D[k][c]) / len(D[k][c])

METRICAS = [
    ("f_inicial", "f inicial promedio", "Calidad de la solución inicial", False),
    ("f_final", "f final promedio", "Calidad final (tras Hill Climbing)", False),
    ("iteraciones", "Iteraciones promedio", "Iteraciones hasta el óptimo local", False),
    ("evaluaciones", "Evaluaciones de f promedio", "Evaluaciones de f (escala log)", True),
]


def panel(ax, col, ylabel, titulo, log):
    w = 0.36
    for i, k in enumerate(KS):
        for off, D, c in ((-w / 2, A, C_ALE), (w / 2, G, C_GRE)):
            v = prom(D, k, col)
            ax.bar(i + off, v, w * 0.92, color=c, zorder=3)
            txt = f"{v:,.0f}" if col == "evaluaciones" or col == "iteraciones" and v >= 100 else (
                f"{v:.1f}" if col == "iteraciones" else f"{v:,.1f}")
            ax.annotate(txt, (i + off, v), ha="center", va="bottom", xytext=(0, 3),
                        textcoords="offset points", fontsize=8.5)
    ax.set_xticks(range(len(KS)))
    ax.set_xticklabels([f"k = {k}" for k in KS])
    ax.set_ylabel(ylabel)
    ax.set_title(titulo, loc="left", fontsize=11, fontweight="bold")
    ax.grid(axis="y", color=GRID, zorder=0)
    ax.set_axisbelow(True)
    if log:
        ax.set_yscale("log")
        ax.set_ylim(top=ax.get_ylim()[1] * 12)
    else:
        ax.set_ylim(0, ax.get_ylim()[1] * 1.3)
    ax.legend(handles=[plt.Rectangle((0, 0), 1, 1, color=C_ALE), plt.Rectangle((0, 0), 1, 1, color=C_GRE)],
              labels=["Aleatoria", "Greedy aleatorizado (m=3)"], frameon=False, fontsize=8.5, ncol=2,
              loc="upper center")


for col, yl, tit, lg in METRICAS:
    fig, ax = plt.subplots(figsize=(6.4, 4.2), dpi=200)
    panel(ax, col, yl, tit, lg)
    fig.tight_layout()
    fig.savefig(os.path.join(out, f"comp_{col}.png"))
    plt.close(fig)

fig, axs = plt.subplots(2, 2, figsize=(12, 8), dpi=200)
for ax, (col, yl, tit, lg) in zip(axs.flat, METRICAS):
    panel(ax, col, yl, tit, lg)
fig.suptitle("Solución inicial aleatoria vs. greedy aleatorizado (m=3) – promedio de 10 imágenes por k",
             x=0.01, ha="left", fontsize=13, fontweight="bold")
fig.tight_layout(rect=(0, 0, 1, 0.96))
fig.savefig(os.path.join(out, "comp_resumen.png"))

# Tabla resumen por consola
print(f"{'k':>2} {'metodo':<9}{'f_ini':>10}{'f_fin':>11}{'iter':>7}{'eval':>9}")
for k in KS:
    for n, D in (("aleatoria", A), ("greedy", G)):
        print(f"{k:>2} {n:<9}{prom(D,k,'f_inicial'):>10.2f}{prom(D,k,'f_final'):>11.2f}"
              f"{prom(D,k,'iteraciones'):>7.1f}{prom(D,k,'evaluaciones'):>9.0f}")
print("Figuras en:", out)
