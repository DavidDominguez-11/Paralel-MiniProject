# Iteración 5 — Pruebas de reglas biológicas

## Objetivo

Evaluar por separado expansión de plantas, consumo de plantas por herbívoros y depredación de herbívoros por carnívoros.

## Caso A — Expansión de plantas

```text
10x10, 20 ticks, P=1, H=0, C=0, seed=101
```

La población vegetal evolucionó de 1 a 36 plantas. Se observaron aumentos graduales en varios ticks, por lo que la reproducción de plantas está activa y respeta celdas disponibles.

## Caso B — Herbívoros con plantas

```text
6x6, 8 ticks, P=20, H=5, C=0, seed=202
```

La población vegetal bajó de 20 a 12 en el transcurso de la simulación, mientras los herbívoros bajaron de 5 a 2. Esto es compatible con consumo de plantas y muerte por hambre/energía. También se observaron plantas nuevas al final, confirmando que ambas reglas pueden coexistir.

## Caso C — Carnívoros con herbívoros

```text
6x6, 8 ticks, P=0, H=12, C=4, seed=303
```

Los herbívoros bajaron de 12 a 11 en el primer tick y posteriormente desaparecieron; los carnívoros bajaron de 4 a 2 inicialmente y sobrevivieron varios ticks antes de morir por falta de alimento. El resultado es coherente con depredación y hambre.

## Hallazgos

- La expansión de plantas funciona.
- El consumo de plantas por herbívoros se observa en los conteos.
- La depredación de herbívoros por carnívoros se observa en los conteos.
- Las muertes por falta de alimento aparecen cuando una especie no tiene recursos.
- La reproducción de herbívoros y carnívoros no puede considerarse validada todavía: la condición pendiente en `main.c` usa el hambre incrementado en lugar del hambre inicial.

## Estado

La iteración valida tres reglas funcionales, pero no cierra la validación biológica completa hasta corregir y probar reproducción animal.
