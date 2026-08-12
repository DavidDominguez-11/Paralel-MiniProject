# Iteración 1 — Resultados de validación

## Objetivo

Comprobar que la implementación base compila, produce una salida completa por tick y conserva el comportamiento entre ejecuciones con distinto número de hilos.

## Configuración

```text
Filas: 8
Columnas: 8
Ticks: 5
Plantas: 12
Herbívoros: 6
Carnívoros: 3
Semilla: 99
Comparación: 1 hilo contra 4 hilos
```

## Resultado de compilación

La compilación con:

```text
gcc -O2 -Wall -Wextra -std=c11 -fopenmp main.c -o ecosystem.exe
```

terminó correctamente. La versión original produjo una advertencia de indentación en la línea final, sin impedir la generación del ejecutable.

## Resultado de ejecución

La prueba generó Tick 0, Tick 1, Tick 2, Tick 3, Tick 4 y Tick 5, con población y distribución de cuadrícula en cada punto.

La ejecución pequeña también mantuvo poblaciones válidas y no mostró celdas con más de un organismo.

## Reproducibilidad multihilo

Se ejecutó la misma configuración con 1 y 4 hilos. Las salidas coincidieron al normalizar únicamente el texto de la cabecera que informa el número de hilos.

Resultado: `REPRODUCIBLE`.

Esto confirma que las decisiones aleatorias dependen de semilla, tick, celda y operación, y no de un generador global cuyo orden pueda cambiar entre hilos.

## Hallazgo pendiente

La condición de reproducción actual compara la variable local `hunger` después de incrementarla. Para representar correctamente “el organismo no tenía hambre al inicio del tick”, debe comparar `x.hunger == 0`. La modificación fue identificada, pero el helper de edición del entorno no permitió reabrir `main.c`; por tanto, esta corrección todavía no está aplicada ni validada.

## Decisión

No se cierra esta iteración como completamente aprobada. La reproducibilidad y la salida funcionan, pero la reproducción requiere aplicar y probar la corrección antes de continuar con nuevas reglas.
