#!/bin/bash
# Corre hill_climbing sobre todas las instancias y valida cada solucion.
# Uso: ./probar_todo.sh [carpeta_instancias] [carpeta_salida] [semilla]
#   Por defecto: imagenes_preprocesadas/instancias  soluciones  42

INSTANCIAS=${1:-imagenes_preprocesadas/instancias}
SALIDA=${2:-soluciones}
SEMILLA=${3:-42}

if [ ! -x ./hill_climbing ]; then
    echo "No existe ./hill_climbing. Compilar con:"
    echo "  g++ -std=c++17 -O2 -Wall -o hill_climbing hill_climbing.cpp"
    exit 1
fi

mkdir -p "$SALIDA"
RESUMEN="$SALIDA/resumen.csv"
echo "instancia,k,f_inicial,f_final,iteraciones,evaluaciones,umbrales,valida" > "$RESUMEN"

total=0
ok=0
for inst in "$INSTANCIAS"/*.txt; do
    [ -e "$inst" ] || { echo "No hay instancias en $INSTANCIAS"; exit 1; }
    nombre=$(basename "$inst" .txt)
    total=$((total + 1))

    if ! salida=$(./hill_climbing "$inst" "$SALIDA" "$SEMILLA" 2>&1); then
        echo "[FALLO] $nombre: $salida"
        echo "$nombre,,,,,,,ERROR" >> "$RESUMEN"
        continue
    fi

    k=$(echo "$salida"  | sed -n 's/.*k=\([0-9]*\),.*/\1/p')
    fi=$(echo "$salida" | awk '/f inicial/ {print $3}')
    ff=$(echo "$salida" | awk '/f final/ {print $3}')
    it=$(echo "$salida" | sed -n 's/Iteraciones: \([0-9]*\),.*/\1/p')
    ev=$(echo "$salida" | sed -n 's/.*evaluaciones: \([0-9]*\)/\1/p')
    um=$(echo "$salida" | sed -n 's/^Umbrales: //p')

    # Validacion: k umbrales, enteros, 1 <= t1 < ... < tk <= 254
    valida=$(echo "$um" | awk -v k="$k" '
        { n = NF; v = (n == k)
          prev = 0
          for (i = 1; i <= n; i++) {
              if ($i !~ /^[0-9]+$/ || $i <= prev || $i > 254) v = 0
              prev = $i
          }
          print (v ? "OK" : "INVALIDA") }')

    printf "%-16s k=%s  f=%-14s umbrales: %-28s %s\n" "$nombre" "$k" "$ff" "$um" "$valida"
    echo "$nombre,$k,$fi,$ff,$it,$ev,\"$um\",$valida" >> "$RESUMEN"
    [ "$valida" = "OK" ] && ok=$((ok + 1))
done

echo
echo "Instancias procesadas: $total   Soluciones validas: $ok"
echo "Resumen: $RESUMEN"
[ "$ok" -eq "$total" ] && [ "$total" -gt 0 ]