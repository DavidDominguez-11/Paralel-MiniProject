{

OpenMP

for dummies

Ing. Juan Luis Garcia Zarceño
COMPUTACIÓN PARALELA Y DISTRIBUIDA SECCIÓN -
20

...

}

Introducción a OpenMP

¿Qué es OpenMP?

Es una API para programación paralela en sistemas de memoria
compartida, que facilita la escritura de aplicaciones paralelas
en C, C++, y Fortran.

Historia y evolución

Aplicaciones típicas

Iniciado a finales de los años 90, OpenMP ha evolucionado con
múltiples versiones que han añadido capacidades como paralelismo
anidado y soporte para arquitecturas heterogéneas.

Se utiliza en simulaciones científicas, análisis de datos, y
procesamiento de imágenes, entre otros.

Elementos Básicos de OpenMP

01

Directivas
Instrucciones que el compilador interpreta para gestionar
paralelismo.

02

Rutinas de biblioteca
Funciones que proporcionan control adicional

03

Variables de entorno
Configuran el comportamiento de OpenMP

Directiva #pragma omp parallel

{

Descripción

Crea una región paralela
donde múltiples hilos
ejecutan el mismo
código.

}

*

Control de Hilos

{

Descripción

Se puede especificar con
la cláusula num_threads
o la función
omp_set_num_threads.

}*

Directiva #pragma omp for

{

Descripción

Permite distribuir las
iteraciones de un bucle
entre los hilos

}

*

Cláusulas de OpenMP

private

shared

Cada hilo tiene
una copia de la
variable

Variable
compartida entre
hilos

firstprivate

lastprivate

Inicialización
de variables
private con el
valor del hilo
maestro.

Último valor de
la variable se
copia en la
variable de
fuera.

Cláusulas de OpenMP

{

}

*

Reducción reduction

{

Descripción

Permite realizar
operaciones acumulativas
seguras.

}

*

Secciones y Trabajo en Equipo

{

Descripción

sections y single:
Permiten dividir el
trabajo en tareas
distintas.

}

*

Sincronización en OpenMP

critical

atomic

Especifica una sección de
código que debe ser
ejecutada por un solo hilo
a la vez.

Realiza operaciones
atómicas en variables.

barrier

flush

Sincroniza todos los hilos,
haciendo que todos esperen
hasta que todos los hilos
hayan alcanzado la barrera
antes de continuar.

Asegura que todas las
variables compartidas en todos
los hilos sean coherentes.
Garantiza que las escrituras
realizadas por un hilo sean
visibles para otros hilos.

Sincronización en OpenMP

critical

Especifica una sección de código que debe ser ejecutada
por un solo hilo a la vez.

Sincronización en OpenMP

atomic

Realiza operaciones atómicas en variables.

Sincronización en OpenMP

barrier

Sincroniza todos los hilos, haciendo que todos esperen
hasta que todos los hilos hayan alcanzado la barrera antes
de continuar.

Sincronización en OpenMP

flush

Asegura que todas las variables compartidas en todos los hilos sean
coherentes. Garantiza que las escrituras realizadas por un hilo sean
visibles para otros hilos.

Variables de Entorno en OpenMP

OMP_NUM_THREADS

Esta variable de entorno especifica el número máximo de hilos que se deben utilizar en una
región paralela.

OMP_SCHEDULE

OMP_DYNAMIC

OMP_NESTED

Esta variable define la política de programación de las iteraciones de un bucle en regiones
paralelas.

Controla si el número de hilos puede variar dinámicamente durante la ejecución de una región
paralela.

Controla si se permite el paralelismo anidado. Si se establece en true, se pueden crear regiones
paralelas dentro de otras regiones paralelas.

OMP_MAX_ACTIVE_LEVELS

Define el número máximo de niveles de regiones paralelas activas que se pueden anidar.

OMP_THREAD_LIMIT

Especifica el número máximo de hilos que puede usar la aplicación en cualquier momento.

OMP_STACKSIZE

OMP_WAIT_POLICY

OMP_PROC_BIND

Especifica el tamaño de la pila de cada hilo en bytes. Puede ser útil para aplicaciones que
requieren un tamaño de pila mayor al predeterminado.

Define la política de espera para los hilos. Puede ser active para una espera activa o passive
para una espera pasiva, lo que puede influir en el consumo de energía y la latencia.

Controla si los hilos están unidos a procesadores específicos. Puede tomar valores como true,
false, o spread, entre otros, para especificar la afinidad de los hilos a los núcleos de la CPU.

Variables de Entorno en OpenMP

Programación Anidada y Dinámica

{

Descripción

Permite crear regiones
paralelas dentro de
otras

}

*

Estrategias de Balanceo de Carga

static

En el modo static, las iteraciones de un bucle se dividen
equitativamente entre los hilos de manera estática y
determinista. Esto significa que cada hilo recibe un bloque fijo
de iteraciones antes de comenzar la ejecución del bucle. Si se
especifica un tamaño de bloque (chunk size), cada hilo obtiene
bloques de ese tamaño; de lo contrario, las iteraciones se
dividen equitativamente.

Estrategias de Balanceo de Carga

dynamic

En el modo dynamic, las iteraciones se asignan a los hilos de
manera dinámica en bloques (chunk size). Un hilo toma un bloque de
iteraciones y, una vez que termina, solicita otro bloque hasta que
no queden más iteraciones por realizar. Esto permite un mejor
balanceo de carga cuando las iteraciones tienen tiempos de
ejecución variables.

Estrategias de Balanceo de Carga

guided

En el modo guided, las iteraciones se asignan a los hilos en bloques
cuyo tamaño disminuye exponencialmente a medida que avanza la
ejecución del bucle. Inicialmente, se asignan bloques grandes para
minimizar la sobrecarga, y luego los bloques se reducen en tamaño
para permitir un balanceo de carga más fino.

Estrategias de Balanceo de Carga

auto

El modo auto permite que el sistema decida la estrategia de
programación más adecuada en función del entorno de ejecución y del
hardware disponible. OpenMP selecciona automáticamente el método de
programación que considera más eficiente para el bucle dado, basado
en las características de la carga de trabajo y la plataforma.

Estrategias de Balanceo de Carga
guided

dynamic

static

auto

Consideraciones y Buenas Prácticas en OpenMP

Consideraciones y Buenas Prácticas en OpenMP

Comprensión de las
Dependencias de Datos

Antes de paralelizar un código, es crucial entender las dependencias de datos. Las dependencias
de datos ocurren cuando una instrucción depende de los datos de una instrucción anterior.
Ignorar estas dependencias puede llevar a condiciones de carrera y resultados incorrectos.
Identificar y resolver estas dependencias es fundamental para una paralelización efectiva y
correcta.

Maximizar la eficiencia del uso de recursos es clave en la programación paralela. Esto incluye:

Uso Eficiente de Recursos

• Evitar sobrecargar con demasiados hilos: Más hilos no siempre equivalen a un mejor
rendimiento. Demasiados hilos pueden causar contención de recursos y sobrecarga de
administración.

• Afinidad de hilos: Establecer afinidad de hilos para que se ejecuten en núcleos específicos
puede mejorar el rendimiento, especialmente en sistemas NUMA (Non-Uniform Memory Access).

Manejo Adecuado de la
Sincronización

Las herramientas de sincronización como critical, atomic, y barrier deben usarse de manera
adecuada para evitar condiciones de carrera. Sin embargo, el uso excesivo de estas herramientas
puede llevar a cuellos de botella y reducir el paralelismo. Por lo tanto, es importante
encontrar un equilibrio y minimizar la sincronización innecesaria.

Prueba y Depuración
Exahustiva

Las aplicaciones paralelas pueden comportarse de manera inesperada debido a la concurrencia. Por
lo tanto, es esencial realizar pruebas exhaustivas y utilizar herramientas de depuración para
detectar y corregir errores. Herramientas como GDB y Valgrind pueden ser útiles, pero también es
importante emplear técnicas específicas para paralelismo, como el análisis de condiciones de
carrera.

Depuración de Programas con OpenMP

La depuración de programas paralelos presenta desafíos únicos debido a la concurrencia y la
sincronización. Algunos problemas comunes incluyen:

Condiciones de carrera

Ocurren cuando múltiples hilos acceden a datos compartidos sin la adecuada sincronización, lo
que lleva a resultados no deterministas.

Bloqueos mutuos
(deadlocks)

Situaciones en las que dos o más hilos se bloquean entre sí esperando recursos.

Interbloqueos (livelocks)

Situaciones en las que dos o más hilos están ocupados cambiando de estado en respuesta a otros
hilos, sin avanzar.

Herramientas y Técnicas de Depuración

GDB (GNU Debugger)

Puede utilizarse para depurar programas OpenMP. Aunque no está específicamente diseñado para el
paralelismo, permite inspeccionar el estado de los hilos y las variables.

Intel Inspector

Una herramienta avanzada que detecta errores de memoria y condiciones de carrera en programas
paralelos.

Valgrind

Aunque tradicionalmente se usa para detectar fugas de memoria, Valgrind también puede
identificar errores de acceso a la memoria que podrían estar relacionados con el paralelismo.

Estrategias para Detectar y Resolver Problemas

Aislar y reproducir el
problema

Intenta aislar el problema en una porción más pequeña del código y reproducirlo de manera
consistente.

Revisar la sincronización

Asegúrate de que todas las operaciones críticas estén adecuadamente sincronizadas.

Utilizar condiciones de
ejecución controladas

Configura las condiciones de ejecución para que sean controlables, como fijar el número de hilos
o desactivar ciertas partes del código, para facilitar la depuración.

Thanks!

< Do you have any questions? >

jlgarcia@uvg.edu.gt
+502 4974 0808
Uvg.edu.gt

CREDITS: This presentation template was created by
Slidesgo, and includes icons by Flaticon, and
infographics & images by Freepik

