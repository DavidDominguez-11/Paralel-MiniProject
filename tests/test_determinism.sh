#!/bin/sh
set -eu

program=${1:-./ecosystem}
temporary_directory=$(mktemp -d)
trap 'rm -rf "$temporary_directory"' 0 HUP INT TERM

set -- --rows 12 --cols 17 --ticks 8 --plants 60 --herbivores 20 \
    --carnivores 8 --seed 987654

# La igualdad prueba que este escenario produce el mismo estado observable con
# 1 y 4 hilos. No demuestra por sí sola corrección semántica ni ausencia de
# carreras para todas las entradas o planificaciones posibles.
"$program" "$@" --threads 1 --output "$temporary_directory/one.txt"
"$program" "$@" --threads 4 --output "$temporary_directory/four.txt"

if ! cmp -s "$temporary_directory/one.txt" "$temporary_directory/four.txt"; then
  printf '%s\n' \
    "Fallo: la salida cambió al ejecutar el mismo escenario con 1 y 4 hilos." >&2
  diff -u "$temporary_directory/one.txt" "$temporary_directory/four.txt" >&2
  exit 1
fi

printf '%s\n' "Determinismo verificado con 1 y 4 hilos."