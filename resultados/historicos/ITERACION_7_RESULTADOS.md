# Iteración 7 — Corrección y validación de reproducción animal

## Cambios aplicados

Se corrigieron dos problemas en `main.c`:

1. La reproducción consulta el hambre del estado inicial del organismo mediante `x.hunger == 0`.
2. El costo de energía solo se aplica cuando el organismo no come. Comer ahora produce la ganancia neta indicada por las reglas del proyecto.
3. Se separó la liberación final de memoria para eliminar la advertencia `-Wmisleading-indentation`.

## Compilación

```text
gcc -O2 -Wall -Wextra -std=c11 -fopenmp main.c -o ecosystem.exe
```

Resultado: compilación exitosa sin advertencias.

## Reproducción de herbívoros

Configuración de búsqueda:

```text
20x20, 80 ticks, 120 plantas, 8 herbívoros, 0 carnívoros
```

Con la semilla `16`, la población máxima de herbívoros fue 9, superando la población inicial de 8. Esto confirma que la reproducción de herbívoros puede activarse.

## Reproducción de carnívoros

Configuración de búsqueda:

```text
15x15, 120 ticks, 100 plantas, 20 herbívoros, 5 carnívoros
```

Se probaron 150 semillas. Con la semilla `95`, la población máxima de carnívoros fue 6, superando la población inicial de 5. Esto confirma que la reproducción de carnívoros también puede activarse, aunque depende de conseguir suficiente alimento y espacio.

## Reproducibilidad multihilo

Se compararon los escenarios reproductivos con 1 y 4 hilos:

```text
Caso herbívoros: MATCH 1vs4
Caso carnívoros: MATCH 1vs4
```

Las salidas completas coincidieron después de normalizar únicamente el número de hilos de la cabecera.

## Conclusión

La reproducción animal queda validada funcionalmente y mantiene determinismo entre ejecución secuencial y paralela para los escenarios probados.
