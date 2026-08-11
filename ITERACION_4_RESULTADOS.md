# Iteración 4 — Carga mayor y medición de rendimiento

## Objetivo

Verificar que la coherencia multihilo se mantiene en una simulación más grande y obtener una primera referencia de tiempos.

## Configuración

```text
Filas: 80
Columnas: 80
Ticks: 20
Plantas iniciales: 1800
Herbívoros iniciales: 500
Carnívoros iniciales: 250
Semilla: 7777
Salida: archivo por ejecución
```

## Coherencia

Las salidas de 2, 4, 8 y 16 hilos coincidieron con la salida de 1 hilo, normalizando únicamente el número de hilos de la cabecera:

```text
2 hilos:  LARGE_MATCH
4 hilos:  LARGE_MATCH
8 hilos:  LARGE_MATCH
16 hilos: LARGE_MATCH
```

## Tiempos observados

Los tiempos fueron medidos con `Measure-Command` en el entorno actual:

| Hilos | Tiempo aproximado |
|---:|---:|
| 1 | 42.46 ms |
| 2 | 27.10 ms |
| 4 | 26.60 ms |
| 8 | 27.02 ms |
| 16 | 28.44 ms |

El mejor resultado observado fue con 4 hilos: aproximadamente `1.60x` de speedup frente a 1 hilo. A partir de 4 hilos, el tiempo se estabiliza o empeora ligeramente por overhead y por la resolución de conflictos que permanece secuencial.

## Interpretación

La paralelización sí aporta una mejora en esta carga, pero no escala linealmente. No debe afirmarse que 16 hilos sean mejores que 4; las mediciones indican que 4 hilos fueron el punto más favorable en esta ejecución.

Estas cifras son una referencia del entorno actual, no un resultado universal del hardware.

## Conclusión

La simulación sigue siendo coherente en una carga mayor y la medición confirma el comportamiento esperado: pocos hilos reducen tiempo, mientras que demasiados hilos no garantizan más rendimiento.

La corrección de reproducción continúa siendo el bloqueo funcional pendiente antes de cerrar las pruebas de reglas biológicas.
