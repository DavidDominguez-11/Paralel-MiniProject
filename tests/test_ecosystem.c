#include "ecosystem.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "FALLO %s:%d: %s\n", __FILE__, __LINE__, #condition);    \
      ++failures;                                                              \
    }                                                                          \
  } while (false)

static Ecosystem *create_empty_ecosystem(int rows, int cols, uint64_t seed) {
  Ecosystem *ecosystem =
      ecosystem_create((EcosystemConfig){rows, cols, 1, seed});
  CHECK(ecosystem != NULL);
  return ecosystem;
}

static void test_moore_diagonal_visibility(void) {
  Ecosystem *ecosystem = create_empty_ecosystem(2, 2, UINT64_C(1));

  CHECK(
      ecosystem_set_cell(ecosystem, 0, 0, (Cell){SPECIES_HERBIVORE, 5, 0, 0}));
  CHECK(ecosystem_set_cell(ecosystem, 1, 1, (Cell){SPECIES_PLANT, 1, 0, 0}));
  CHECK(ecosystem_tick(ecosystem, 1));
  CHECK(ecosystem_get_cell(ecosystem, 1, 1).species == SPECIES_HERBIVORE);
  CHECK(ecosystem_get_cell(ecosystem, 1, 1).energy == 6);
  ecosystem_destroy(ecosystem);
}

static void test_consumed_prey_cannot_act(void) {
  Ecosystem *ecosystem = create_empty_ecosystem(2, 2, UINT64_C(2));

  CHECK(
      ecosystem_set_cell(ecosystem, 0, 0, (Cell){SPECIES_CARNIVORE, 7, 0, 0}));
  CHECK(
      ecosystem_set_cell(ecosystem, 0, 1, (Cell){SPECIES_HERBIVORE, 5, 0, 0}));
  CHECK(ecosystem_set_cell(ecosystem, 1, 1, (Cell){SPECIES_PLANT, 1, 0, 0}));
  CHECK(ecosystem_tick(ecosystem, 1));
  CHECK(ecosystem_get_cell(ecosystem, 0, 1).species == SPECIES_CARNIVORE);
  CHECK(ecosystem_get_cell(ecosystem, 1, 1).species == SPECIES_PLANT);
  CHECK(ecosystem_count(ecosystem).herbivores == 0);
  ecosystem_destroy(ecosystem);
}

static void test_movement_loser_remains(void) {
  Ecosystem *ecosystem = create_empty_ecosystem(1, 3, UINT64_C(3));

  CHECK(
      ecosystem_set_cell(ecosystem, 0, 0, (Cell){SPECIES_CARNIVORE, 7, 0, 0}));
  CHECK(
      ecosystem_set_cell(ecosystem, 0, 2, (Cell){SPECIES_CARNIVORE, 7, 0, 0}));
  CHECK(ecosystem_tick(ecosystem, 1));
  CHECK(ecosystem_get_cell(ecosystem, 0, 0).species == SPECIES_EMPTY);
  CHECK(ecosystem_get_cell(ecosystem, 0, 1).species == SPECIES_CARNIVORE);
  CHECK(ecosystem_get_cell(ecosystem, 0, 2).species == SPECIES_CARNIVORE);
  ecosystem_destroy(ecosystem);
}

static void test_starvation_after_three_failures(void) {
  Ecosystem *ecosystem = create_empty_ecosystem(1, 1, UINT64_C(4));

  CHECK(
      ecosystem_set_cell(ecosystem, 0, 0, (Cell){SPECIES_HERBIVORE, 5, 0, 0}));
  CHECK(ecosystem_tick(ecosystem, 1));
  CHECK(ecosystem_get_cell(ecosystem, 0, 0).hunger == 1);
  CHECK(ecosystem_tick(ecosystem, 2));
  CHECK(ecosystem_get_cell(ecosystem, 0, 0).hunger == 2);
  CHECK(ecosystem_tick(ecosystem, 3));
  CHECK(ecosystem_get_cell(ecosystem, 0, 0).species == SPECIES_EMPTY);
  ecosystem_destroy(ecosystem);
}

static void test_crowded_plant_dies(void) {
  Ecosystem *ecosystem = create_empty_ecosystem(1, 1, UINT64_C(5));

  CHECK(ecosystem_set_cell(ecosystem, 0, 0, (Cell){SPECIES_PLANT, 1, 0, 0}));
  CHECK(ecosystem_tick(ecosystem, 1));
  CHECK(ecosystem_get_cell(ecosystem, 0, 0).species == SPECIES_EMPTY);
  ecosystem_destroy(ecosystem);
}

static uint64_t find_reproducing_plant_seed(void) {
  /* El PRNG no tiene estado: este barrido fijo es determinista y está acotado a
   * 1000 ejecuciones de un tick en 2x2, sin depender de detalles internos. */
  for (uint64_t seed = 0; seed < UINT64_C(1000); ++seed) {
    Ecosystem *ecosystem = create_empty_ecosystem(2, 2, seed);
    Population population;

    CHECK(ecosystem_set_cell(ecosystem, 0, 0, (Cell){SPECIES_PLANT, 1, 0, 0}));
    CHECK(ecosystem_tick(ecosystem, 1));
    population = ecosystem_count(ecosystem);
    ecosystem_destroy(ecosystem);
    if (population.plants == 2) {
      return seed;
    }
  }
  CHECK(false);
  return 0;
}

static void test_consumed_parent_cancels_offspring(void) {
  uint64_t seed = find_reproducing_plant_seed();
  Ecosystem *ecosystem = create_empty_ecosystem(2, 2, seed);

  /* La semilla ya probó reproducción; el consumo debe cancelar esa reserva. */
  CHECK(ecosystem_set_cell(ecosystem, 0, 0, (Cell){SPECIES_PLANT, 1, 0, 0}));
  CHECK(
      ecosystem_set_cell(ecosystem, 0, 1, (Cell){SPECIES_HERBIVORE, 5, 0, 0}));
  CHECK(ecosystem_tick(ecosystem, 1));
  CHECK(ecosystem_count(ecosystem).plants == 0);
  CHECK(ecosystem_get_cell(ecosystem, 0, 0).species == SPECIES_HERBIVORE);
  ecosystem_destroy(ecosystem);
}

static void test_competing_births_share_one_destination(void) {
  bool found = false;

  /* Se busca un caso observable de competencia en un dominio finito y fijo; el
   * resultado no depende del orden de ejecución ni del entorno. */
  for (uint64_t seed = 0; seed < UINT64_C(10000) && !found; ++seed) {
    Ecosystem *ecosystem = create_empty_ecosystem(1, 3, seed);

    CHECK(ecosystem_set_cell(ecosystem, 0, 0, (Cell){SPECIES_PLANT, 1, 0, 0}));
    CHECK(ecosystem_set_cell(ecosystem, 0, 2, (Cell){SPECIES_PLANT, 1, 0, 0}));
    CHECK(ecosystem_tick(ecosystem, 1));
    if (ecosystem_get_cell(ecosystem, 0, 1).species == SPECIES_PLANT) {
      CHECK(ecosystem_count(ecosystem).plants == 3);
      CHECK(ecosystem_get_cell(ecosystem, 0, 0).species == SPECIES_PLANT);
      CHECK(ecosystem_get_cell(ecosystem, 0, 2).species == SPECIES_PLANT);
      found = true;
    }
    ecosystem_destroy(ecosystem);
  }
  CHECK(found);
}

static void test_movement_precedes_birth_without_ghosts(void) {
  uint64_t seed = find_reproducing_plant_seed();
  Ecosystem *ecosystem = create_empty_ecosystem(1, 3, seed);

  CHECK(ecosystem_set_cell(ecosystem, 0, 0, (Cell){SPECIES_PLANT, 1, 0, 0}));
  CHECK(
      ecosystem_set_cell(ecosystem, 0, 2, (Cell){SPECIES_CARNIVORE, 7, 0, 0}));
  CHECK(ecosystem_tick(ecosystem, 1));
  CHECK(ecosystem_get_cell(ecosystem, 0, 0).species == SPECIES_PLANT);
  CHECK(ecosystem_get_cell(ecosystem, 0, 1).species == SPECIES_CARNIVORE);
  CHECK(ecosystem_get_cell(ecosystem, 0, 2).species == SPECIES_EMPTY);
  CHECK(ecosystem_count(ecosystem).plants == 1);
  CHECK(ecosystem_count(ecosystem).carnivores == 1);
  ecosystem_destroy(ecosystem);
}

static void test_oversized_grid_is_rejected(void) {
  EcosystemConfig config = {46341, 46340, 1, UINT64_C(1)};
  uint64_t required_bytes = 0;

  CHECK(ecosystem_required_bytes(config, &required_bytes));
  CHECK(required_bytes > ECOSYSTEM_MEMORY_BUDGET_BYTES);
  CHECK(ecosystem_create(config) == NULL);
}

static void test_counters_saturate_without_overflow(void) {
  Ecosystem *ecosystem = create_empty_ecosystem(1, 2, UINT64_C(6));

  /* La alimentación y la edad pueden alcanzar el límite representable; desde
   * allí deben saturarse, no provocar overflow con signo. */
  CHECK(ecosystem_set_cell(ecosystem, 0, 0,
                           (Cell){SPECIES_CARNIVORE, INT_MAX, 0, 0}));
  CHECK(
      ecosystem_set_cell(ecosystem, 0, 1, (Cell){SPECIES_HERBIVORE, 5, 0, 0}));
  CHECK(ecosystem_tick(ecosystem, 1));
  CHECK(ecosystem_get_cell(ecosystem, 0, 1).energy == INT_MAX);
  ecosystem_destroy(ecosystem);

  ecosystem = create_empty_ecosystem(1, 2, UINT64_C(7));
  CHECK(ecosystem_set_cell(ecosystem, 0, 0,
                           (Cell){SPECIES_PLANT, 1, INT_MAX, 0}));
  CHECK(ecosystem_tick(ecosystem, 1));
  CHECK(ecosystem_get_cell(ecosystem, 0, 0).age == INT_MAX);
  ecosystem_destroy(ecosystem);
}

int main(void) {
  test_moore_diagonal_visibility();
  test_consumed_prey_cannot_act();
  test_movement_loser_remains();
  test_starvation_after_three_failures();
  test_crowded_plant_dies();
  test_consumed_parent_cancels_offspring();
  test_competing_births_share_one_destination();
  test_movement_precedes_birth_without_ghosts();
  test_oversized_grid_is_rejected();
  test_counters_saturate_without_overflow();

  if (failures != 0) {
    fprintf(stderr, "%d comprobaciones fallaron.\n", failures);
    return EXIT_FAILURE;
  }
  puts("Todas las pruebas unitarias pasaron.");
  return EXIT_SUCCESS;
}
