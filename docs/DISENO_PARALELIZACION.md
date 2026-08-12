# Diseño determinista y paralelización del ecosistema

El motor prioriza corrección reproducible sobre porcentaje paralelo: `CurrentGrid` es inmutable
durante un tick, las intenciones se calculan en paralelo por filas y un resolutor determinista
valida transacciones antes de que un único escritor construya `NextGrid`.

## Decisiones principales

| Tema | Decisión implementada | Origen |
|---|---|---|
| Dominio | Cuadrícula finita de `N × M`, sin envoltura | Decisión formal solicitada |
| Vecindad | Moore de radio 1, hasta ocho celdas | Decisión formal solicitada |
| Estado | Especie, energía, edad e inanición acumulada | Enunciado general + formalización |
| Plantas | Mueren si no existe ninguna celda vacía en su Moore actual | Regla formal solicitada; el PDF solo da un ejemplo de encierro por plantas |
| Reproducción vegetal | 30% por tick si existe espacio; un destino pseudoaleatorio | Ejemplo cuantitativo del PDF adoptado como regla |
| Herbívoros | Comen planta `+1` energía; mueren al tercer tick consecutivo sin comer | Ejemplo explícito del PDF adoptado como regla |
| Carnívoros | Comen herbívoro `+2`; mueren al cuarto tick consecutivo sin comer | Umbral elegido: el PDF no fija cantidad |
| Vejez | Herbívoro a edad 50; carnívoro a edad 60 | Decisión del modelo, no requisito del PDF |
| Energía | Sin comida pierde 1; muerte al quedar en 0; cría desde 8/12 | Decisión del modelo, no requisito del PDF |
| Cría animal | Probabilidad 20%/15%, solo con hambre previa 0 y energía 8/12 | Decisión del modelo, no requisito del PDF |
| Conflictos | Menor índice lineal de origen gana | Decisión determinista |

El PDF exige cuadrícula, ticks, alimentación, reproducción, movimiento, depredación, competencia,
salida de poblaciones/distribución y uso de OpenMP. No fija topología, desempates, edades máximas,
umbral carnívoro ni una semántica transaccional completa; por eso esas políticas se declaran aquí.

## Descomposición matemática D/R/V/C

Sea `G_t` la cuadrícula al inicio del tick `t`, `I_t[i]` la intención de la celda `i`, y `G_{t+1}`
la siguiente cuadrícula:

```text
G_t (inmutable)
      |
      v
D: I_t[i] = D(G_t, i, seed, t)       paralelo por filas
      |
      v
R: W_t = R(G_t, I_t)                 arbitraje estable de recursos
      |
      v
V: A_t = V(G_t, I_t, W_t)            consumo, supervivencia y fallback
      |
      v
C: G_(t+1) = C(G_t, I_t, W_t, A_t)   escritor único
```

`D` es una función pura respecto del estado compartido. La aleatoriedad también es función pura:

```text
random = mix(seed, tick, índice_lineal, salt_de_decisión)
```

No existe estado global de PRNG, por lo que el orden de ejecución no altera las decisiones.

## Propiedad de datos por fase

| Fase | Lee | Escribe | Propietario |
|---|---|---|---|
| D: decisión | Todo `CurrentGrid` | `Intent[i]` | Hilo dueño de la fila de `i` |
| R: resolución | Intenciones y `CurrentGrid` | ganadores, consumidos | Hilo principal, orden lineal |
| V: validación | Ganadores y estado actual | supervivencia y destino final | Hilo principal |
| C: construcción | Todo lo validado | `NextGrid` | Hilo principal, una asignación final por celda |
| Conteo | `CurrentGrid` | acumuladores privados | Cada hilo; combinación por reducción |

La separación importa: paralelizar escrituras conflictivas con `critical` o `atomic` ocultaría una
política de orden. Resolverlas en serie hace visible el contrato matemático y evita resultados
dependientes del planificador.

## Política de conflictos y fallback

La resolución sigue este orden total:

1. Se arbitra la caza de carnívoros por destino; gana el menor índice de origen.
2. Todo herbívoro consumido pierde movimiento, alimentación y reproducción propios.
3. Solo los herbívoros restantes arbitran plantas, también por menor origen.
4. Se valida edad, energía e inanición después de conocer si la alimentación tuvo éxito.
5. Se arbitran movimientos hacia celdas inicialmente vacías; gana el menor origen.
6. Un perdedor vivo permanece en su origen. Nunca desaparece por perder una competencia.
7. Se escriben los actores sobrevivientes y sus movimientos comprometidos.
8. Se arbitran nacimientos solo si el padre sobrevivió y el destino continúa vacío.

La precedencia de recursos es `alimentación > movimiento > nacimiento`. Una reproducción es una
transacción de dos condiciones: `(padre sobrevive) ∧ (hijo gana destino)`. Si falla cualquiera, no
nace el hijo y no queda reserva. El padre conserva su resultado primario.

### Ejemplos

```text
Antes:   C H P       El carnívoro consume H.
Intenta: C->H, H->P  La intención H->P se invalida: una presa no actúa después de morir.
Después: . C P
```

```text
Antes:   C . C       Ambos quieren el centro.
Ganador: origen 0    El origen 2 pierde, pero permanece.
Después: . C C
```

```text
Padre P reserva x; un movimiento gana x.
Resultado: el movimiento ocupa x, no nace la planta y x no bloquea otro recurso futuro.
```

## OpenMP y sincronización

| Directiva | Ubicación | Motivo |
|---|---|---|
| `parallel for schedule(static)` | Generación de intenciones | Filas con costo similar; cada iteración tiene escritura exclusiva |
| `parallel for reduction(+) schedule(static)` | Conteo de especies | Acumulación asociativa sin atomics ni falso uso compartido intencional |
| `omp_set_dynamic(0)` | Inicio del tick | Solicitar el número configurado de hilos sin ajuste dinámico |

Cada `parallel for` incluye una barrera implícita al final. La primera garantiza que todas las
intenciones estén completas antes de R/V/C; la segunda garantiza conteos combinados antes de
imprimir. No se usa `nowait`, `critical`, `atomic` ni una barrera manual porque no existe una fase
paralela posterior dentro de la misma región que lo requiera.

## Argumento de ausencia de carreras

Durante D, todos los hilos leen bytes inmutables de `CurrentGrid`; la iteración `i` escribe solo
`Intent[i]`. Dos iteraciones no comparten destino de escritura. Al terminar la barrera implícita,
R/V/C se ejecutan serialmente y `NextGrid` parte vacío. El arbitraje asegura como máximo un actor
primario y un nacimiento validado por destino. Tras intercambiar punteros, la cuadrícula recién
construida pasa a ser el siguiente estado inmutable. En el conteo, OpenMP mantiene acumuladores
privados y los combina mediante reducción. Por tanto no hay accesos concurrentes donde al menos
uno sea escritura sobre la misma ubicación.

## Complejidad

Con `K = N × M`:

| Recurso | Complejidad |
|---|---|
| Decisión | `O(K)` trabajo, `O(K/T)` ideal con `T` hilos |
| Resolución y validación | `O(K)` |
| Construcción | `O(K)` |
| Conteo y salida | `O(K)`; imprimir domina en simulaciones pequeñas |
| Memoria | `O(K)` para dos cuadrículas y arreglos auxiliares |

La vecindad tiene tamaño máximo constante 8, por lo que no agrega un factor dependiente de `K`.
Antes de reservar, el motor calcula con aritmética segura los bytes de dos `Cell`, una intención,
dos índices y dos marcas por celda. Se aplica un presupuesto de 512 MiB a esos arreglos para fallar
rápido ante configuraciones imprácticas y evitar depender del overcommit del sistema operativo. El
límite expresa consumo de memoria del proyecto, no una dimensión artificial de la cuadrícula.

## Medición de rendimiento

No debe inferirse speedup midiendo la ejecución que imprime cada tick: formato, terminal o disco
pueden dominar el tiempo. Para un experimento defendible se debe usar la misma semilla y tamaño,
redireccionar salida a un archivo o separar medición del kernel, realizar calentamiento, repetir,
reportar mediana y registrar CPU, compilador, afinidad y `OMP_NUM_THREADS`. En cuadrículas pequeñas,
la creación de regiones paralelas puede hacer que 4 hilos sean más lentos que 1.

## Limitaciones

- La fase R/V/C es intencionalmente serial; privilegia semántica verificable sobre speedup máximo.
- Los animales eligen entre recursos visibles, no siguen gradientes más allá de radio 1.
- No hay fronteras toroidales, obstáculos, clima ni persistencia binaria de estados.
- Las constantes biológicas son parámetros de diseño compilados, no opciones de CLI.
- La salida completa cuesta `O(K)` por tick y puede ocultar el costo computacional del motor.

## Trazabilidad al PDF

| PDF | Implementación |
|---|---|
| Inicialización con cantidades por especie | `ecosystem_populate` y parámetros CLI |
| Ciclo por ticks | `ecosystem_tick` desde `main.c` |
| Procesamiento paralelo de cuadrícula | Fase D por filas con OpenMP |
| Alimentación y depredación | Arbitraje de `INTENT_EAT` |
| Movimiento y escape | Selección de vacíos Moore y fallback estable |
| Reproducción con espacio | Destino secundario validado después del padre |
| Muerte por comida, edad o depredación | Fase V y marcas de consumo |
| Población y distribución por tick | Reducción de conteos y `ecosystem_print` |

Fuente primaria: `Proyecto_Simulacion_Ecosistema_OpenMP_Reglas.pdf`, páginas 1 a 4. Las filas marcadas
como decisiones en la primera tabla completan ambigüedades del documento, no se presentan como
requisitos textuales.
