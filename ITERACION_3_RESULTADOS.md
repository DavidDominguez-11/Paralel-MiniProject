# Iteración 3 — Comparación multihilo e invariantes

## Objetivo

Comprobar que aumentar el número de hilos no cambia la evolución del ecosistema y que los invariantes básicos se mantienen en todos los ticks.

## Configuración común

```text
Filas: 12
Columnas: 16
Ticks: 12
Plantas iniciales: 80
Herbívoros iniciales: 30
Carnívoros iniciales: 15
Semilla: 4242
```

## Hilos probados

Se ejecutó la misma configuración con:

```text
1, 2, 4, 8, 16, 32 y 64 hilos
```

Todas las ejecuciones terminaron con código de salida cero.

## Comparación de resultados

Se compararon los archivos completos después de normalizar únicamente el valor `threads=N` de la cabecera.

```text
2 hilos:  MATCH
4 hilos:  MATCH
8 hilos:  MATCH
16 hilos: MATCH
32 hilos: MATCH
64 hilos: MATCH
```

Esto confirma que, para esta configuración y semilla, la decisión paralela es determinista y no presenta divergencias entre hilos.

## Invariantes verificados

Para cada tick y cada cantidad de hilos se comprobó:

1. La población total no supera `12 × 16 = 192` celdas.
2. La cantidad de símbolos `P`, `H` y `C` en la distribución coincide con los conteos reportados.
3. No aparecen celdas con más de un organismo.

Resultado: `INVARIANTS_OK`.

## Conclusión

La paralelización actual es consistente incluso con sobreasignación de hilos hasta 64. Esto valida la ausencia de carreras visibles en la fase paralela de decisión para el escenario probado.

Esta prueba no demuestra todavía que todas las reglas biológicas sean correctas. La reproducción continúa pendiente de corregirse y validarse explícitamente.
