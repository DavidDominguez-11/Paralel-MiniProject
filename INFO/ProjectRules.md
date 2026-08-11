Universidad del Valle de Guatemala
Facultad de Ingeniería
Departamento de Ciencias de la Computación
Computación Paralela y Distribuida - Sección 20 - 2024 – 2

Semestre 2 – 2024

Proyecto: Simulación de Ecosistema con
OpenMP

Descripción del Proyecto
Este proyecto tiene como objetivo desarrollar una simulación de un ecosistema utilizando OpenMP
para paralelizar operaciones. El ecosistema incluirá diferentes especies con comportamientos
específicos, como reproducción, consumo de recursos y movimiento. Los estudiantes deberán
implementar la simulación y demostrar cómo se comportan las especies en un ambiente compartido.

Instrucciones del Proyecto
1. **Inicialización del Ecosistema**: Define una cuadrícula que represente el espacio del ecosistema.
Inicializa el ecosistema con un número determinado de cada especie (plantas, herbívoros,
carnívoros).

2. **Ciclo de Simulación**: La simulación debe avanzar en ticks de tiempo. En cada tick, actualiza el
estado del ecosistema, considerando los comportamientos de las especies (alimentación,
reproducción, movimiento).

3. **Paralelización con OpenMP**: Utiliza OpenMP para paralelizar el procesamiento de la cuadrícula,
asignando tareas a diferentes hilos.

4. **Interacciones entre Especies**: Implementa reglas de interacción entre especies, como
depredación y competencia por recursos.

5. **Visualización y Resultados**: Genera una salida que muestre el estado del ecosistema después de
cada tick, incluyendo la población de cada especie y la distribución en la cuadrícula.

Pseudocódigo del Sistema

Inicializar cuadrícula y especies
Para cada tick de la simulación:
    #pragma omp parallel for
    Para cada celda en la cuadrícula:
        Actualizar estado de las plantas
        Actualizar estado de los herbívoros
        Actualizar estado de los carnívoros
    Sincronizar datos de especies entre hilos
    Mostrar estado del ecosistema
Fin Para

Universidad del Valle de Guatemala
Facultad de Ingeniería
Departamento de Ciencias de la Computación
Computación Paralela y Distribuida - Sección 20 - 2024 – 2

Semestre 2 – 2024

Reglas del Ecosistema

Reproducción
- **Plantas**: Las plantas pueden reproducirse si hay espacio disponible en celdas adyacentes. Se
puede establecer una probabilidad de reproducción en cada tick.
  Ejemplo: Si una planta tiene espacio en una celda vecina, puede expandirse a esa celda con una
probabilidad del 30%.
- **Herbívoros y Carnívoros**: Pueden reproducirse si tienen suficiente comida y espacio disponible.
  Ejemplo: Un herbívoro puede reproducirse si ha consumido al menos una cierta cantidad de plantas
y hay una celda adyacente vacía.

Consumo de Recursos
- **Herbívoros**: Se mueven hacia celdas con plantas para alimentarse. Si encuentran una planta, la
consumen y ganan energía.
  Ejemplo: Un herbívoro consume una planta en una celda adyacente y gana una unidad de energía.
- **Carnívoros**: Cazan herbívoros moviéndose hacia celdas ocupadas por ellos. Si encuentran un
herbívoro, lo consumen y ganan energía.
  Ejemplo: Un carnívoro consume un herbívoro y gana dos unidades de energía.

Movimiento
- **Plantas**: No se mueven, pero pueden expandirse a celdas vecinas vacías.
- **Herbívoros y Carnívoros**: Pueden moverse a celdas adyacentes en busca de comida o para
escapar de depredadores.
  Ejemplo: Si un herbívoro detecta un carnívoro en una celda adyacente, puede moverse a una celda
vacía para escapar.

Muerte
- **Plantas**: Pueden morir si no tienen espacio suficiente para crecer o si son consumidas
completamente.
  Ejemplo: Una planta muere si está rodeada completamente por otras plantas o si es consumida por
herbívoros.
- **Herbívoros y Carnívoros**: Pueden morir por falta de comida, vejez o ser consumidos por un
depredador.
  Ejemplo: Un herbívoro muere si no encuentra plantas para comer en tres ticks consecutivos.

Interacción entre Especies
- **Depredación**: Los carnívoros cazan herbívoros, reduciendo la población de herbívoros.
- **Competencia por Recursos**: Tanto herbívoros como carnívoros pueden competir por recursos
(plantas para herbívoros, herbívoros para carnívoros).

Universidad del Valle de Guatemala
Facultad de Ingeniería
Departamento de Ciencias de la Computación
Computación Paralela y Distribuida - Sección 20 - 2024 – 2

Semestre 2 – 2024

Ejemplo de Salida
**Tick 1**
Plantas: 150
Herbívoros: 40
Carnívoros: 15
Distribución:
P P P H H C
P P H H C C
H H H C C C

**Tick 5**
Plantas: 120
Herbívoros: 35
Carnívoros: 18
Distribución:
P P H H H C
P H H C C C
H H C C C C

**Tick 10**
Plantas: 100
Herbívoros: 30
Carnívoros: 22
Distribución:
P H H H C C
H H C C C C
C C C C C C

**Tick 20**
Plantas: 80
Herbívoros: 20
Carnívoros: 30
Distribución:
H H H C C C
C C C C C C
C C C C C C

(P: Plantas, H: Herbívoros, C: Carnívoros)
Nota: Estos son ejemplos simplificados de salida. La distribución real dependerá de las reglas de
interacción y los parámetros de simulación definidos.

Universidad del Valle de Guatemala
Facultad de Ingeniería
Departamento de Ciencias de la Computación
Computación Paralela y Distribuida - Sección 20 - 2024 – 2

Semestre 2 – 2024

Entregables
1. Código fuente del proyecto, documentado adecuadamente.
2. Instrucciones para ejecutar la simulación y cualquier requisito de software necesario.
3. Informe con la descripción del diseño del sistema, decisiones de implementación y análisis de los
resultados obtenidos.
4. Un archivo de resultados que muestre el estado del ecosistema en varios puntos del tiempo.

Rúbrica de Evaluación
1. **Funcionalidad (40%)**: El sistema debe funcionar correctamente según las especificaciones,
incluyendo la correcta paralelización de tareas y la actualización del estado de las especies.
2. **Eficiencia (20%)**: Evaluación del rendimiento del sistema y el uso de OpenMP. Se espera un uso
eficiente de los recursos disponibles.
3. **Calidad del Código (20%)**: Claridad, organización y documentación del código fuente.
4. **Informe (10%)**: Claridad y profundidad del informe, explicando el diseño, decisiones tomadas,
y análisis de resultados.
5. **Creatividad (10%)**: Uso innovador de las técnicas de paralelización y simulación.

