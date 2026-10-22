#!/usr/bin/env python3
"""Figura de una instancia: imagen original | histograma con umbrales | imagen segmentada.

Uso (desde la carpeta del proyecto):
  python3 figura_segmentacion.py imagen_01 4
  python3 figura_segmentacion.py imagen_01 4 soluciones/resumen.csv imagenes_preprocesadas/imagenes

Requiere: pip install numpy matplotlib pillow
Convencion de clases igual que mri::segmentar: [0,t1], [t1+1,t2], ..., [tk+1,255].
"""
import csv, sys
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from PIL import Image

nombre, k = sys.argv[1], int(sys.argv[2])
resumen = sys.argv[3] if len(sys.argv) > 3 else "soluciones/resumen.csv"
carpeta = sys.argv[4] if len(sys.argv) > 4 else "imagenes_preprocesadas/imagenes"

fila = next(r for r in csv.DictReader(open(resumen)) if r["instancia"] == f"{nombre}_k{k}")
t = [int(x) for x in fila["umbrales"].split()]
img = np.array(Image.open(f"{carpeta}/{nombre}.jpg").convert("L"))
h = np.bincount(img.ravel(), minlength=256)

clase = np.searchsorted(np.array(t), np.arange(256), side="left")
tonos = np.rint(255.0 * clase / len(t)).astype(np.uint8)
seg = tonos[img]

INK, MUTED, GRID = "#1f2933", "#6b7785", "#e3e7ec"
RAMP = ["#dbe7f5", "#b6cfeb", "#8fb5e0", "#689bd4", "#3f7cc0", "#2a5f9e", "#1a4378"]
plt.rcParams.update({"font.size": 12, "text.color": INK, "axes.labelcolor": INK,
                     "xtick.color": MUTED, "ytick.color": MUTED,
                     "axes.spines.top": False, "axes.spines.right": False})
fig, ax = plt.subplots(1, 3, figsize=(14, 4.6), gridspec_kw={"width_ratios": [1, 1.5, 1]})
ax[0].imshow(img, cmap="gray", vmin=0, vmax=255); ax[0].set_title("Imagen original"); ax[0].axis("off")
cols = [RAMP[int(round(c * (len(RAMP) - 1) / k))] for c in clase]
ax[1].bar(np.arange(256), np.maximum(h, 1), width=1.0, color=cols, linewidth=0)
ax[1].set_yscale("log"); ax[1].set_xlim(0, 255)
ax[1].grid(axis="y", color=GRID); ax[1].set_axisbelow(True)
for i, ti in enumerate(t, 1):
    ax[1].axvline(ti, color=INK, lw=1.3, ls=(0, (4, 3)))
    ax[1].text(ti + 2, ax[1].get_ylim()[1] * 0.6, f"$t_{i}$={ti}", fontsize=10, va="top")
ax[1].set_xlabel("Intensidad de gris"); ax[1].set_ylabel("Píxeles (log)")
ax[1].set_title(f"Histograma y umbrales (k={k})")
ax[2].imshow(seg, cmap="gray", vmin=0, vmax=255); ax[2].set_title(f"Imagen segmentada ({k+1} clases)"); ax[2].axis("off")
fig.suptitle(f"{nombre}   ·   f = {float(fila['f_final']):.2f}", x=0.01, ha="left", fontsize=13, fontweight="bold")
fig.tight_layout(rect=(0, 0, 1, 0.94))
salida = f"fig_{nombre}_k{k}.png"
fig.savefig(salida, dpi=200); print("Guardado:", salida)