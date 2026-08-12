#ifndef ECOSYSTEM_H
#define ECOSYSTEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/** Presupuesto máximo para el estado dinámico administrado por el motor. */
#define ECOSYSTEM_MEMORY_BUDGET_BYTES                                          \
  (UINT64_C(512) * UINT64_C(1024) * UINT64_C(1024))

/** Especies representables; el valor vacío denota una celda sin ocupante. */
typedef enum {
  SPECIES_EMPTY,
  SPECIES_PLANT,
  SPECIES_HERBIVORE,
  SPECIES_CARNIVORE
} Species;

/** Estado completo de una celda durante un tick. */
typedef struct {
  Species species;
  int energy;
  int age;
  int hunger;
} Cell;

/**
 * Configuración inmutable del motor.
 *
 * `rows`, `cols` y `threads` deben ser positivos. La semilla identifica una
 * secuencia pseudoaleatoria reproducible e independiente del número de hilos.
 */
typedef struct {
  int rows;
  int cols;
  int threads;
  uint64_t seed;
} EcosystemConfig;

/** Cantidad de individuos por especie; todos los campos deben ser no negativos.
 */
typedef struct {
  int plants;
  int herbivores;
  int carnivores;
} Population;

/** Motor opaco propietario de sus cuadrículas y arreglos auxiliares. */
typedef struct Ecosystem Ecosystem;

/**
 * Crea un ecosistema vacío.
 *
 * La configuración debe ser válida, caber en índices `int` y no superar
 * `ECOSYSTEM_MEMORY_BUDGET_BYTES`. Devuelve un objeto propio del llamador o
 * `NULL` si la configuración o alguna reserva falla.
 */
Ecosystem *ecosystem_create(EcosystemConfig config);

/** Libera el ecosistema y todos sus recursos; acepta `NULL`. */
void ecosystem_destroy(Ecosystem *ecosystem);

/**
 * Calcula los bytes administrados por `ecosystem_create` sin reservar memoria.
 *
 * `required_bytes` debe apuntar a almacenamiento escribible. Devuelve `false`
 * para argumentos inválidos o si el cálculo excede los límites del motor; en
 * ese caso no debe usarse el valor de salida.
 */
bool ecosystem_required_bytes(EcosystemConfig config, uint64_t *required_bytes);

/**
 * Vacía y repuebla el ecosistema de forma determinista.
 *
 * `ecosystem` debe ser válido y la población no negativa debe caber en la
 * cuadrícula. Devuelve `false` sin modificarla si las precondiciones fallan.
 * El ecosistema conserva toda la memoria y no toma recursos del llamador.
 */
bool ecosystem_populate(Ecosystem *ecosystem, Population population);

/**
 * Avanza un tick completo mediante las fases D/R/V/C.
 *
 * `ecosystem` debe ser válido y `tick` positivo. Devuelve `false` solo por
 * precondiciones inválidas; al completar, reemplaza el estado visible.
 */
bool ecosystem_tick(Ecosystem *ecosystem, int tick);

/**
 * Sustituye una celda, principalmente para construir escenarios controlados.
 *
 * Las coordenadas deben pertenecer a la cuadrícula. La especie debe ser válida
 * y los contadores deben ser no negativos. Devuelve `false` y no modifica el
 * estado si algún argumento es inválido.
 */
bool ecosystem_set_cell(Ecosystem *ecosystem, int row, int col, Cell cell);

/**
 * Obtiene una copia de la celda indicada; no transfiere propiedad.
 *
 * Para un ecosistema nulo o coordenadas inválidas devuelve una celda vacía.
 */
Cell ecosystem_get_cell(const Ecosystem *ecosystem, int row, int col);

/**
 * Cuenta las especies del estado actual mediante una reducción OpenMP.
 * Para un ecosistema nulo devuelve todos los conteos en cero.
 */
Population ecosystem_count(const Ecosystem *ecosystem);

/**
 * Escribe conteos y cuadrícula para un tick no negativo.
 *
 * `ecosystem` y `output` deben ser válidos. El flujo sigue perteneciendo al
 * llamador. Devuelve `false` por argumentos inválidos o cualquier error de E/S.
 */
bool ecosystem_print(const Ecosystem *ecosystem, int tick, FILE *output);

#endif
