# Iteración 6 — Diagnóstico de reproducción animal

## Objetivo

Confirmar que la corrección de la condición de hambre permite la reproducción de herbívoros y carnívoros.

## Prueba

Se buscaron semillas favorables para herbívoros usando:

```text
30x30, 150 ticks, 150 plantas, 12 herbívoros, 0 carnívoros
100 semillas distintas
```

Resultado: en ninguna semilla la población de herbívoros superó 12.

## Diagnóstico

La condición de reproducción ya consulta correctamente `x.hunger`, pero existe un segundo problema en el modelo energético. En `apply_actions` el organismo pierde 1 energía por tick y luego recibe solo 1 energía al comer una planta:

```text
energía inicial del herbívoro: 5
costo por tick: -1
ganancia por planta: +1
ganancia neta al comer: 0
umbral de reproducción: 8
```

Por lo tanto, un herbívoro no puede alcanzar el umbral de reproducción.

## Corrección propuesta

El costo temporal debe aplicarse únicamente cuando el organismo no come. La lógica debe quedar conceptualmente así:

```c
if (s->species != PLANT) {
    r.hunger++;
    if (a->kind != ACT_EAT) r.energy--;
}
```

Al comer, el herbívoro ganará 1 energía neta y el carnívoro 2 energías netas, de acuerdo con las reglas del proyecto.

## Estado

Iteración no cerrada: el diagnóstico está confirmado, pero la corrección no se ha aplicado porque el editor del entorno continúa sin poder modificar `main.c`.
