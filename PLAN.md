Sí. A partir de lo que exige el proyecto y del material de OpenMP, este sería el **plan maestro de desarrollo**, pensado para llevarnos desde cero hasta una entrega completa sin empezar a programar a ciegas. La especificación exige inicialización, simulación por ticks, paralelización, interacción entre especies, visualización/resultados y cuatro entregables; además, la rúbrica concentra 80 % en funcionalidad, eficiencia y calidad del código.  

# Plan maestro — Simulación de Ecosistema con OpenMP

```text
FASE 0  Comprensión                    LISTO
FASE 1  Especificación exacta
FASE 2  Arquitectura
FASE 3  Modelo de datos
FASE 4  Inicialización
FASE 5  Motor secuencial
FASE 6  Reglas de plantas
FASE 7  Reglas de herbívoros
FASE 8  Reglas de carnívoros
FASE 9  Resolución de conflictos
FASE 10 Paralelización OpenMP
FASE 11 Estadísticas y salida
FASE 12 Pruebas de correctitud
FASE 13 Rendimiento
FASE 14 Optimización
FASE 15 Archivo de resultados
FASE 16 Informe
FASE 17 Documentación/ejecución
FASE 18 Validación contra rúbrica
FASE 19 Entrega final
```

La regla durante todo el desarrollo será:

```text
CORRECTO
   ↓
PARALELO
   ↓
EFICIENTE
   ↓
OPTIMIZADO
```

No al revés. El propio material del curso indica que primero deben comprenderse las dependencias de datos y que abusar de la sincronización puede reducir el paralelismo. 

---

# FASE 0 — Comprensión del problema

## Estado: completada

Ya entendemos los tres componentes:

```text
P = Planta
H = Herbívoro
C = Carnívoro
```

y sus relaciones:

```text
Planta
  ↓ alimento
Herbívoro
  ↓ alimento
Carnívoro
```

Además comprendemos:

```text
cuadrícula
ticks
movimiento
alimentación
reproducción
muerte
depredación
competencia
OpenMP
condiciones de carrera
sincronización
balanceo de carga
```

No necesitamos más teoría general antes de empezar el diseño.

---

# FASE 1 — Crear nuestra especificación exacta

Esta es la **primera fase que debemos realizar ahora**.

El PDF describe el comportamiento general, pero deja parámetros y prioridades abiertos. 

Antes de escribir C debemos crear nuestra propia especificación.

## 1.1 Definir dimensiones

Decidir:

```text
filas
columnas
```

Preferiblemente configurables.

Por ejemplo:

```text
ROWS
COLS
```

No recomiendo que la lógica dependa de un tamaño concreto.

---

# 1.2 Definir poblaciones iniciales

Necesitamos parámetros para:

```text
PLANTAS_INICIALES
HERBIVOROS_INICIALES
CARNIVOROS_INICIALES
```

Y garantizar:

```text
P + H + C <= número de celdas
```

para evitar intentar colocar más organismos que espacios existentes.

---

# 1.3 Definir número de ticks

Algo conceptualmente equivalente a:

```text
TOTAL_TICKS
```

La simulación ejecutará:

```text
Tick 0
Tick 1
...
Tick N
```

---

# 1.4 Definir vecindad

Debemos elegir entre:

```text
4 vecinos
```

```text
    N
    |
W - X - E
    |
    S
```

o:

```text
8 vecinos
```

```text
NW N NE
W  X  E
SW S SE
```

El PDF solamente habla de celdas adyacentes, por lo que esta decisión nos corresponde.

---

# 1.5 Definir bordes

Una celda en:

```text
[0][0]
```

no tiene vecinos arriba ni a la izquierda.

Debemos garantizar que nunca intentemos consultar:

```text
[-1][0]
[0][-1]
```

La regla deberá ser:

```text
0 <= fila < ROWS
0 <= columna < COLS
```

---

# 1.6 Parámetros de plantas

Definir explícitamente:

```text
probabilidad de reproducción
condición de reproducción
condición de muerte
```

El PDF propone 30 % como ejemplo de reproducción, pero no lo convierte en un valor obligatorio. 

---

# 1.7 Parámetros de herbívoros

Necesitamos decidir:

```text
energía inicial
energía obtenida por planta
costo por tick/movimiento, si existe
límite de hambre
edad máxima
energía/requisito de reproducción
costo de reproducción, si existe
```

El PDF sí da como ejemplos:

```text
planta consumida → +1 energía
3 ticks sin plantas → muerte
```

pero no proporciona un modelo energético completo.  

---

# 1.8 Parámetros de carnívoros

De forma equivalente:

```text
energía inicial
energía obtenida por herbívoro
límite de hambre
edad máxima
requisito de reproducción
```

El documento utiliza como ejemplo:

```text
herbívoro consumido → +2 energía
```



---

# 1.9 Establecer prioridades

Necesitamos escribir una regla inequívoca.

Por ejemplo, para un herbívoro:

```text
1. verificar supervivencia
2. detectar depredador
3. escapar
4. buscar planta
5. comer/moverse
6. reproducirse
7. movimiento libre
```

El orden concreto tendremos que decidirlo nosotros.

---

# 1.10 Resolver múltiples opciones

Si un herbívoro tiene:

```text
P
|
H - P
|
P
```

¿cuál planta elige?

Tenemos que establecer:

```text
aleatoria
```

o alguna política determinista.

Lo mismo aplica a:

```text
múltiples espacios vacíos
múltiples presas
múltiples destinos
```

---

# FASE 2 — Diseñar la arquitectura

No programaremos todavía.

Primero diseñaremos las piezas.

Propongo conceptualizar:

```text
main
 |
 ├── configuración
 |
 ├── inicialización
 |
 ├── simulación
 |      |
 |      ├── plantas
 |      ├── herbívoros
 |      ├── carnívoros
 |      └── conflictos
 |
 ├── estadísticas
 |
 ├── visualización
 |
 └── resultados
```

---

# FASE 3 — Diseñar el modelo de datos

Aquí decidiremos exactamente qué representa una celda.

## 3.1 Tipo de organismo

Conceptualmente:

```text
VACIO
PLANTA
HERBIVORO
CARNIVORO
```

---

# 3.2 Estado individual

Necesitaremos evaluar qué atributos realmente hacen falta:

```text
tipo
energía
edad
ticks_sin_comer
```

No añadiremos datos que no aporten al modelo.

---

# 3.3 Posición

Hay dos alternativas conceptuales:

```text
la posición está implícita
por fila/columna
```

o almacenar:

```text
fila
columna
```

en cada organismo.

Evaluaremos cuál simplifica más la solución.

---

# 3.4 Doble estado

Evaluaremos seriamente trabajar con:

```text
grid_actual
grid_siguiente
```

en lugar de modificar el mismo grid mientras lo recorremos.

Objetivo:

```text
grid_actual
    ↓ lectura

procesamiento

    ↓ escritura
grid_siguiente
```

Esto reduce el riesgo de procesar dos veces un organismo durante un mismo tick.

---

# FASE 4 — Inicialización

Una vez definido el modelo:

## 4.1 Crear cuadrícula vacía

```text
. . . . .
. . . . .
. . . . .
. . . . .
```

## 4.2 Colocar plantas

Aleatoriamente sin colisiones.

## 4.3 Colocar herbívoros

Solo en celdas libres.

## 4.4 Colocar carnívoros

Solo en celdas libres.

## 4.5 Inicializar estados

Por ejemplo:

```text
energía
edad
hambre
```

según nuestras reglas.

## 4.6 Validar población

Después:

```text
P == P iniciales
H == H iniciales
C == C iniciales
```

---

# FASE 5 — Motor de simulación secuencial

Esta fase es crítica.

**Antes de paralelizar, construiremos una versión conceptualmente correcta con un solo hilo.**

Estructura:

```text
for cada tick
    |
    ├── procesar estado
    ├── resolver acciones
    ├── crear siguiente estado
    ├── contar poblaciones
    └── mostrar resultado
```

El material recomienda precisamente entender dependencias antes de paralelizar. 

---

# FASE 6 — Implementar completamente las plantas

Orden:

```text
detectar planta
     ↓
buscar espacios
     ↓
evaluar reproducción
     ↓
generar acción
     ↓
evaluar muerte
```

Casos de prueba mínimos:

```text
P aislada
P con un espacio
P completamente rodeada
P junto a herbívoro
dos P queriendo reproducirse
P en esquina
P en borde
```

No avanzaremos hasta que todos tengan un resultado definido.

---

# FASE 7 — Implementar completamente los herbívoros

Casos:

```text
H aislado
H junto a P
H con varias P
H junto a C
H con P y C simultáneamente
H sin alimento
H llegando al límite de hambre
H en edad máxima
H con condiciones de reproducción
H sin espacio para reproducirse
```

Para cada uno verificaremos:

```text
posición
energía
hambre
edad
reproducción
muerte
```

---

# FASE 8 — Implementar completamente los carnívoros

Casos:

```text
C aislado
C junto a H
C con varios H
C sin presas
C llegando al límite de hambre
C llegando a edad máxima
C reproduciéndose
C sin espacio
```

Y verificaremos los mismos estados.

---

# FASE 9 — Sistema de conflictos

Esta será probablemente la parte más importante de la arquitectura.

Debemos identificar todas las categorías.

## Conflicto A

Dos organismos quieren una celda vacía:

```text
H → .
    ↑
H ──┘
```

## Conflicto B

Dos herbívoros quieren una planta:

```text
H → P ← H
```

## Conflicto C

Dos carnívoros quieren un herbívoro:

```text
C → H ← C
```

## Conflicto D

Dos organismos quieren reproducirse en la misma celda.

## Conflicto E

Un organismo quiere moverse a una celda mientras otro quiere consumirlo.

## Conflicto F

Un herbívoro quiere comer mientras un carnívoro quiere consumirlo.

Para **cada conflicto** escribiremos una regla antes de paralelizar.

---

# FASE 10 — Preparación para OpenMP

Solo después de que la versión lógica funcione.

El PDF de OpenMP presenta como herramientas centrales `parallel`, `for`, las cláusulas de datos, reducción y mecanismos de sincronización. 

Primero clasificaremos todas las variables.

## Compartidas

Posiblemente:

```text
grid_actual
grid_siguiente
configuración
```

## Privadas

Posiblemente:

```text
fila
columna
vecinos
destino
acción
variables temporales
```

Después verificaremos explícitamente cada una.

---

# FASE 11 — Primera paralelización

No paralelizaremos todo simultáneamente.

Empezaremos con el recorrido principal:

```text
#pragma omp parallel for
```

El material define `omp for` precisamente como la distribución de iteraciones de un bucle entre los hilos. 

Primera meta:

```text
1 thread → correcto
2 threads → correcto
4 threads → correcto
```

No nos importará todavía si 4 threads son más rápidos.

Primero:

```text
CORRECCIÓN
```

---

# FASE 12 — Sincronización

Identificaremos **cada escritura compartida**.

Para cada una preguntaremos:

```text
¿dos threads pueden llegar aquí?
```

Si no:

```text
no sincronizar
```

Si sí:

```text
¿podemos rediseñarlo para evitarlo?
```

Si tampoco:

```text
seleccionar mecanismo
```

entre los conceptos estudiados:

```text
critical
atomic
barrier
reduction
```

No usaremos `critical` indiscriminadamente porque el material advierte que la sincronización excesiva crea cuellos de botella. 

---

# FASE 13 — Barreras entre fases

Si nuestra arquitectura utiliza:

```text
DECISIONES
    ↓
RESOLUCIÓN
    ↓
APLICACIÓN
```

debemos garantizar que ningún hilo entre prematuramente a la siguiente fase.

Conceptualmente:

```text
Threads
  ↓
DECIDIR
  ↓
BARRIER
  ↓
RESOLVER/APLICAR
```

`barrier` hace precisamente que todos esperen hasta que todos los miembros del equipo alcancen el punto de sincronización. 

---

# FASE 14 — Estadísticas paralelas

Necesitamos obtener:

```text
plantas
herbívoros
carnívoros
```

Después de cada tick.

Aquí evaluaremos:

```text
reduction
```

porque está diseñada para acumulaciones seguras. 

Salida:

```text
Tick 50

Plantas:       XXX
Herbívoros:    XXX
Carnívoros:    XXX
```

---

# FASE 15 — Visualización de cuadrícula

El proyecto exige mostrar la distribución. 

Definiremos:

```text
. = vacío
P = planta
H = herbívoro
C = carnívoro
```

Ejemplo:

```text
P . . H C
P P . H .
. H P . C
C . . P .
```

Para grids enormes, probablemente no será conveniente imprimir absolutamente todos los ticks en consola; separaremos visualización de pruebas y archivo de resultados.

---

# FASE 16 — Archivo de resultados

El cuarto entregable exige un archivo con el estado del ecosistema en varios momentos. 

Planearemos guardar:

```text
configuración inicial

Tick 0
P:
H:
C:
Grid:

Tick X
...

Tick Y
...

Tick final
...
```

Posiblemente también:

```text
tiempo de ejecución
número de threads
schedule utilizado
```

para facilitar el análisis posterior.

---

# FASE 17 — Reproducibilidad

Como existen probabilidades, necesitamos considerar la semilla aleatoria.

Para depuración queremos poder hacer:

```text
misma configuración
+
misma semilla
```

y reproducir lo máximo posible el mismo escenario lógico.

Esto facilitará muchísimo investigar errores.

---

# FASE 18 — Suite de pruebas pequeñas

Antes de probar un grid enorme utilizaremos escenarios mínimos.

### Prueba 1

```text
P .
```

¿puede reproducirse?

### Prueba 2

```text
H P
```

¿H consume P?

### Prueba 3

```text
C H
```

¿C consume H?

### Prueba 4

```text
H P H
```

¿se consume una sola vez?

### Prueba 5

```text
C H C
```

¿solo un C obtiene la presa?

### Prueba 6

```text
P H C
```

¿se respeta nuestra prioridad?

### Prueba 7

```text
H . H
```

¿se resuelve correctamente una celda disputada?

---

# FASE 19 — Pruebas de bordes

Necesitamos probar organismos en:

```text
esquina superior izquierda
esquina superior derecha
esquina inferior izquierda
esquina inferior derecha

borde superior
borde inferior
borde izquierdo
borde derecho
```

Objetivo:

```text
0 accesos fuera de matriz
```

---

# FASE 20 — Pruebas de invariantes

Después de cada tick podremos comprobar propiedades que **nunca deberían romperse**.

Por ejemplo:

```text
P >= 0
H >= 0
C >= 0
```

y:

```text
P + H + C <= ROWS * COLS
```

Además:

```text
una celda contiene como máximo
un organismo
```

Estas pruebas son extremadamente útiles para encontrar errores paralelos.

---

# FASE 21 — Prueba con un hilo

Ejecutaremos:

```text
threads = 1
```

y verificaremos:

```text
reglas
movimientos
energía
reproducción
muerte
estadísticas
```

Esta será nuestra referencia.

---

# FASE 22 — Pruebas multihilo

Después:

```text
2
4
8
...
```

threads.

Buscaremos:

```text
crashes
datos corruptos
conteos imposibles
organismos duplicados
recursos consumidos dos veces
deadlocks
resultados inconsistentes
```

Las diapositivas identifican precisamente condiciones de carrera, deadlocks y livelocks como problemas comunes en programas OpenMP. 

---

# FASE 23 — Depuración

Si algo falla:

```text
1. reducir grid
2. reducir organismos
3. fijar semilla
4. fijar threads
5. reproducir
6. identificar acción
7. identificar memoria compartida
8. revisar sincronización
```

Esto coincide con las recomendaciones del material: aislar y reproducir el problema, revisar sincronización y utilizar condiciones de ejecución controladas. 

---

# FASE 24 — Medición de rendimiento

Solo cuando sea correcto.

Estableceremos una configuración suficientemente grande para que la paralelización tenga sentido.

Mediremos:

```text
Threads     Tiempo

1             ?
2             ?
4             ?
8             ?
```

Nunca compararemos ejecuciones con parámetros completamente diferentes.

---

# FASE 25 — Speedup

A partir de los tiempos podremos calcular conceptualmente:

```text
Speedup = tiempo con 1 hilo
          ----------------
          tiempo con N hilos
```

Por ejemplo, si eventualmente:

```text
1 thread = 8 segundos
4 threads = 2.5 segundos
```

el speedup sería:

```text
8 / 2.5 = 3.2x
```

Esos números son únicamente ilustrativos; los valores del informe deberán provenir de mediciones reales.

---

# FASE 26 — Comparar schedules

Las diapositivas presentan:

```text
static
dynamic
guided
auto
```



Podremos experimentar:

| Threads | static | dynamic | guided |
| ------: | -----: | ------: | -----: |
|       1 |      ? |       ? |      ? |
|       2 |      ? |       ? |      ? |
|       4 |      ? |       ? |      ? |
|       8 |      ? |       ? |      ? |

No asumiremos de antemano cuál gana.

---

# FASE 27 — Analizar balance de carga

El ecosistema puede tener zonas:

```text
muy ocupadas
```

y zonas:

```text
casi vacías
```

Así que podremos investigar si un esquema dinámico ayuda cuando distintas iteraciones requieren diferentes cantidades de trabajo, que es precisamente la motivación de `schedule(dynamic)` presentada en el material. 

---

# FASE 28 — Optimización de sincronización

Buscaremos:

```text
critical demasiado grande

critical dentro de loops muy frecuentes

atomic innecesarios

barriers innecesarias

trabajo duplicado

recorridos redundantes
```

La meta:

```text
menos sincronización
sin sacrificar corrección
```

---

# FASE 29 — Optimización del número de hilos

No asumiremos:

```text
más threads = mejor
```

El material señala explícitamente que demasiados hilos pueden aumentar contención y overhead. 

Determinaremos experimentalmente el comportamiento.

---

# FASE 30 — Calidad del código

Esto vale **20 %**. 

Antes de finalizar revisaremos:

```text
nombres claros
funciones pequeñas
responsabilidades separadas
constantes/parámetros claros
comentarios útiles
sin código duplicado
sin variables globales innecesarias
manejo correcto de memoria
validaciones
```

No llenaremos el código de comentarios que simplemente repitan la instrucción.

Los comentarios deben explicar:

```text
por qué
```

especialmente en sincronización.

---

# FASE 31 — Documentar OpenMP

Cada región paralela importante deberá poder justificarse.

Por ejemplo, debemos ser capaces de explicar:

```text
¿Por qué este loop es paralelo?

¿Qué variables son compartidas?

¿Qué variables son privadas?

¿Por qué existe esta sincronización?

¿Qué condición de carrera evita?

¿Por qué usamos este schedule?
```

Eso ayudará tanto a calidad del código como al informe.

---

# FASE 32 — Informe

El PDF exige descripción del diseño, decisiones de implementación y análisis de resultados. 

Planearemos aproximadamente:

```text
1. Introducción

2. Objetivos

3. Diseño del ecosistema

4. Representación de datos

5. Reglas
   5.1 Plantas
   5.2 Herbívoros
   5.3 Carnívoros

6. Ciclo de simulación

7. Problemas de concurrencia

8. Estrategia OpenMP

9. Sincronización

10. Balanceo de carga

11. Metodología de pruebas

12. Resultados

13. Rendimiento

14. Análisis

15. Conclusiones
```

---

# FASE 33 — Resultados para el informe

Guardaremos desde el principio datos útiles.

No queremos terminar el código y después descubrir que nunca medimos nada.

Registraremos:

```text
configuración
semilla
threads
schedule
ticks
P inicial/final
H inicial/final
C inicial/final
tiempo
```

---

# FASE 34 — Instrucciones de ejecución

Es otro entregable obligatorio. 

Debe quedar documentado:

```text
requisitos
compilador
soporte OpenMP
comando de compilación
comando de ejecución
parámetros
formato de salida
```

Las diapositivas muestran como ejemplo la compilación GCC con `-fopenmp`. 

---

# FASE 35 — Validación final contra la rúbrica

No terminaremos simplemente cuando “compile”.

Haremos una revisión exacta.

### Funcionalidad — 40 %

Comprobar:

```text
grid
P/H/C
ticks
alimentación
movimiento
reproducción
muerte
depredación
competencia
OpenMP
salida
```

### Eficiencia — 20 %

Comprobar:

```text
mediciones
varios threads
speedup
schedule
sincronización razonable
```

### Calidad — 20 %

Comprobar:

```text
organización
claridad
documentación
modularidad
```

### Informe — 10 %

Comprobar:

```text
diseño
decisiones
resultados
análisis
```

### Creatividad — 10 %

Podemos aprovechar elementos justificables como:

```text
comparación de schedules
configuración flexible
estadísticas
reproducibilidad
análisis de rendimiento
```

sin añadir complejidad gratuita.

La distribución anterior corresponde exactamente a la rúbrica del documento. 

---

# FASE 36 — Limpieza final

Antes de entregar:

```text
eliminar prints de debugging
eliminar código muerto
eliminar archivos temporales
comprobar warnings
comprobar nombres
comprobar comentarios
comprobar resultados
```

Y hacer una compilación limpia desde cero.

---

# FASE 37 — Simulación final de entrega

Finalmente fingiremos que somos el profesor.

Tomaremos solamente:

```text
código
instrucciones
informe
archivo de resultados
```

y comprobaremos si podemos ejecutar todo sin conocimiento adicional.

Si necesitamos recordar algún comando o configuración que no aparece en la documentación:

```text
documentación incompleta
```

y lo corregimos.

---

# Orden real de trabajo

Aunque son muchas fases, **no significa que debamos hacerlas todas simultáneamente**.

Nuestra ruta inmediata debería ser:

```text
AHORA
  |
  v
1. Cerrar reglas exactas
  |
  v
2. Diseñar estructuras
  |
  v
3. Diseñar algoritmo de un tick
  |
  v
4. Diseñar conflictos
  |
  v
5. Crear versión secuencial
  |
  v
6. Probar
  |
  v
7. Introducir OpenMP
  |
  v
8. Probar concurrencia
  |
  v
9. Medir
  |
  v
10. Optimizar
  |
  v
11. Documentar
  |
  v
12. Entregar
```

## Los cinco checkpoints principales

Para evitar avanzar con problemas ocultos, establecería cinco puntos donde **no seguimos hasta que lo anterior esté sólido**:

```text
CHECKPOINT 1
Especificación cerrada
        ↓
sabemos exactamente qué hace P/H/C

CHECKPOINT 2
Secuencial correcto
        ↓
el ecosistema funciona con 1 hilo

CHECKPOINT 3
Paralelo correcto
        ↓
funciona con múltiples hilos sin corrupción

CHECKPOINT 4
Rendimiento analizado
        ↓
tenemos tiempos y comparación

CHECKPOINT 5
Entrega completa
        ↓
código + instrucciones + informe + resultados
```

La idea central del plan es **separar la simulación de la paralelización**. Si mezclamos desde el primer día reproducción, movimiento, aleatoriedad, `parallel for`, `critical`, `atomic`, estadísticas y optimización, cuando algo falle será difícil saber dónde está el problema. Si primero conseguimos un modelo correcto y después introducimos OpenMP de forma controlada, podremos identificar exactamente qué cambia al añadir concurrencia.

**Nuestro siguiente paso concreto es la Fase 1: cerrar la especificación.** Ahí definiremos uno por uno todos los parámetros y reglas que el PDF deja abiertos —vecindad, energía, hambre, edades, prioridades, reproducción, movimiento y resolución de conflictos— hasta tener una especie de “contrato” del ecosistema. Después de eso podremos diseñar las estructuras de C sin improvisar ninguna regla durante la implementación.
