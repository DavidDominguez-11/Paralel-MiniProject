# Iteración 8 — Regresión multihilo después de corregir energía

## Objetivo

Confirmar que el cambio que habilitó la reproducción animal no alteró el determinismo de la simulación.

## Configuración

```text
20x20, 80 ticks, 120 plantas, 8 herbívoros, 0 carnívoros
Semilla: 16
Hilos: 1, 2, 4 y 8
```

## Resultados

```text
2 hilos: MATCH contra 1 hilo
4 hilos: MATCH contra 1 hilo
8 hilos: MATCH contra 1 hilo
```

La población máxima de herbívoros fue 9 en todas las ejecuciones, frente a 8 iniciales. Por tanto, la reproducción se mantiene activa y determinista después de la corrección energética.

## Conclusión

El cambio de energía no introdujo diferencias entre ejecuciones secuenciales y paralelas. La corrección queda validada funcionalmente para el escenario reproductivo probado.
