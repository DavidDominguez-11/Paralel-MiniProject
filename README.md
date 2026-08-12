# Simulación paralela de ecosistema

Simulación determinista de plantas, herbívoros y carnívoros sobre una cuadrícula finita. El motor
usa C11 y OpenMP con un ciclo por fases: genera intenciones en paralelo, resuelve conflictos en un
orden estable y construye la siguiente cuadrícula sin carreras de datos.

## Ruta rápida

```sh
make
make test
./ecosystem --rows 20 --cols 40 --ticks 20 --threads 4 --seed 20240820
```

## Requisitos

| Herramienta | Uso |
|---|---|
| Compilador C11 | GCC o Clang con soporte de OpenMP |
| OpenMP | En GCC se enlaza mediante `-fopenmp` |
| `make` | Construcción y ejecución de pruebas |
| Utilidades POSIX | El test de determinismo usa `sh`, `mktemp`, `cmp` y `diff` |

En Debian o Ubuntu, GCC y las herramientas básicas se instalan con `build-essential`. En macOS,
Clang requiere instalar y enlazar `libomp` por separado.

## Construcción y pruebas

```sh
make clean
make
make test
```

`make test` ejecuta diez escenarios unitarios del motor y compara byte por byte una simulación
completa con 1 y 4 hilos. Los escenarios cubren vecindad diagonal de Moore, depredación,
fallback de movimiento, inanición, muerte vegetal por encierro, transacciones de reproducción y
rechazo rápido de cuadrículas que exceden el presupuesto de memoria. También verifica que los
contadores saturen en el límite de `int` sin desbordamiento con signo.

Si `clang-format` está instalado, `make format` aplica el estilo a las fuentes C y
`make check-format` lo verifica sin modificar archivos. El formateador es opcional y no participa
en `make` ni en `make test`.

## Ejecución

Todos los parámetros son opcionales:

```sh
./ecosystem \
  --rows 30 --cols 50 --ticks 100 \
  --plants 300 --herbivores 80 --carnivores 20 \
  --threads 4 --seed 42 --output resultados.txt
```

| Parámetro | Predeterminado | Restricción |
|---|---:|---|
| `--rows` | 20 | Entero positivo |
| `--cols` | 40 | Entero positivo |
| `--ticks` | 20 | Entero no negativo |
| `--plants` | 150 | Entero no negativo |
| `--herbivores` | 40 | Entero no negativo |
| `--carnivores` | 15 | Entero no negativo |
| `--threads` | 1 | Entero positivo |
| `--seed` | 20240820 | Entero sin signo de 64 bits |
| `--output` | salida estándar | Ruta de archivo no vacía |

La suma de poblaciones no puede superar `rows × cols`. Además, el motor rechaza antes de reservar
una configuración cuyos arreglos requieran más de 512 MiB. El cálculo usa los bytes reales de dos
cuadrículas, intenciones, ganadores, destinos y marcas por celda; no impone una dimensión máxima
arbitraria. Un argumento desconocido, incompleto, mal formado, fuera de rango o que exceda ese
presupuesto produce código de salida 2. Un fallo efectivo de memoria o escritura produce código 1.

## Salida

Cada tick informa las poblaciones y luego imprime la cuadrícula completa:

```text
Tick 0
Plantas: 2
Herbívoros: 1
Carnívoros: 1
Distribución:
P . H
. P C
```

`P`, `H`, `C` y `.` representan planta, herbívoro, carnívoro y celda vacía. La cantidad de hilos
no forma parte del archivo para que dos ejecuciones equivalentes tengan exactamente la misma
salida observable.

## Diseño

Las reglas precisas, decisiones agregadas al enunciado, arbitraje, propiedad de datos y argumento
de ausencia de carreras se documentan en
[`docs/DISENO_PARALELIZACION.md`](docs/DISENO_PARALELIZACION.md). El enunciado original se conserva
en `docs/Proyecto_Simulacion_Ecosistema_OpenMP_Reglas.pdf`.

## Limpieza

```sh
make clean
```
