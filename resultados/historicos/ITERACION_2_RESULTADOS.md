# Iteración 2 — Pruebas de límites y validación de configuración

## Objetivo

Verificar que la cuadrícula maneja esquinas y dimensiones mínimas, y que el programa rechaza poblaciones que no caben en el espacio disponible.

## Casos ejecutados

### Caso 1 — Cuadrícula mínima con las cuatro especies/celdas

```text
rows=2 cols=2 ticks=1 plants=2 herbivores=1 carnivores=1 threads=1 seed=1
```

Resultado: ejecución exitosa (`exit=0`). La salida mostró Tick 0 y Tick 1. No se observaron accesos fuera de rango ni celdas duplicadas.

### Caso 2 — Una sola celda

```text
rows=1 cols=1 ticks=0 plants=1 herbivores=0 carnivores=0 threads=1 seed=2
```

Resultado: ejecución exitosa (`exit=0`). La salida mostró una única planta en Tick 0.

### Caso 3 — Población mayor que la capacidad

```text
rows=2 cols=2 ticks=1 plants=3 herbivores=2 carnivores=0 threads=1 seed=3
```

Resultado: configuración rechazada correctamente (`exit=2`) con el mensaje `Configuración inválida.`

## Conclusión

La validación de dimensiones y capacidad funciona para estos casos. Esta iteración no modifica la lógica; sirve como línea base de robustez mientras el editor del entorno impide aplicar la corrección pendiente de reproducción.

## Pendiente

Aplicar `x.hunger == 0` en la condición de reproducción, recompilar y agregar una prueba que fuerce o confirme la creación de descendencia.
