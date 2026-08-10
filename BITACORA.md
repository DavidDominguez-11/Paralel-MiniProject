# Bitácora de desarrollo y experimentación

Este documento registra cada iteración del proyecto: hipótesis, cambios, pruebas, resultados y decisiones. Las ejecuciones se realizan con semillas explícitas para que los resultados puedan reproducirse.

## Estado inicial del repositorio

- Rama de trabajo: `Tests`.
- Remoto: `origin/Tests`.
- Base: simulador C/OpenMP con doble búfer, decisiones paralelas y salida por tick.
- Requisitos de referencia: `INFO/ProjectRules.md`.
- Estrategia experimental: corregir primero la lógica y después comparar ejecuciones secuenciales, paralelas y de rendimiento.

## Iteración 1 — Corregir reproducción y documentar la ejecución

### Hipótesis

La reproducción no puede ocurrir con la condición original porque la función incrementaba el hambre antes de comprobar `hunger == 0`. Si el organismo comenzaba con hambre cero, la variable local pasaba a uno y la condición nunca se cumplía.

### Cambios realizados

1. La condición de reproducción debe consultar `x.hunger`, que representa el estado al inicio del tick.
2. Se separará la liberación de memoria y el cierre del archivo para eliminar una advertencia de formato.
3. Se documentarán compilación, ejecución y parámetros.

### Pruebas

- Compilación con GCC y `-fopenmp`.
- Cuadrícula pequeña con pocos organismos.
- Salida con Tick 0 y todos los ticks solicitados.
- Comparación de una ejecución con un hilo contra cuatro hilos con la misma semilla.

### Resultado

Pendiente de registrar después de compilar y ejecutar esta iteración.

### Decisión

Mantener la semilla determinista por celda y tick para comparar hilos sin un generador aleatorio global compartido.
