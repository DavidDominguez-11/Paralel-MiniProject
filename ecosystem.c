#include "ecosystem.h"

#include <limits.h>
#include <omp.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum {
  /* Reglas biológicas compiladas; no son parámetros de ejecución. */
  PLANT_REPRODUCTION_PERCENT = 30,
  HERBIVORE_REPRODUCTION_PERCENT = 20,
  CARNIVORE_REPRODUCTION_PERCENT = 25,
  HERBIVORE_REPRODUCTION_ENERGY = 8,
  CARNIVORE_REPRODUCTION_ENERGY = 10,
  HERBIVORE_MAX_AGE = 50,
  CARNIVORE_MAX_AGE = 60,
  HERBIVORE_STARVATION_TICKS = 3,
  CARNIVORE_STARVATION_TICKS = 6
};

typedef enum { INTENT_STAY, INTENT_MOVE, INTENT_EAT, INTENT_DIE } IntentKind;

typedef struct {
  IntentKind kind;
  int destination;
  int child_destination;
} Intent;

struct Ecosystem {
  EcosystemConfig config;
  size_t total;
  Cell *current;
  Cell *next;
  Intent *intents;
  int *resource_winner;
  int *final_destination;
  unsigned char *consumed;
  unsigned char *alive;
  double *thread_seconds;
};

static const int ROW_OFFSET[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static const int COL_OFFSET[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

static uint64_t mix64(uint64_t value) {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value = (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value = (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

static uint64_t random_value(uint64_t seed, int tick, size_t index,
                             uint64_t salt) {
  uint64_t tick_key = (uint64_t)(unsigned int)tick + UINT64_C(1);
  uint64_t index_key = (uint64_t)index + UINT64_C(1);

  return mix64(seed ^ tick_key * UINT64_C(0x632be59bd9b4e019) ^
               index_key * UINT64_C(0x8cb92baa3f3a5b1d) ^ salt);
}

static bool random_chance(const Ecosystem *ecosystem, int tick, size_t index,
                          uint64_t salt, int percent) {
  return random_value(ecosystem->config.seed, tick, index, salt) %
             UINT64_C(100) <
         (uint64_t)(unsigned int)percent;
}

static int neighbor_index(const Ecosystem *ecosystem, size_t index,
                          int direction) {
  int row = (int)(index / (size_t)ecosystem->config.cols);
  int col = (int)(index % (size_t)ecosystem->config.cols);
  int neighbor_row = row + ROW_OFFSET[direction];
  int neighbor_col = col + COL_OFFSET[direction];

  if (neighbor_row < 0 || neighbor_row >= ecosystem->config.rows ||
      neighbor_col < 0 || neighbor_col >= ecosystem->config.cols) {
    return -1;
  }
  return neighbor_row * ecosystem->config.cols + neighbor_col;
}

static int choose_neighbor(const Ecosystem *ecosystem, size_t index,
                           Species wanted, int excluded, int tick,
                           uint64_t salt) {
  int candidates[8];
  int count = 0;

  for (int direction = 0; direction < 8; ++direction) {
    int candidate = neighbor_index(ecosystem, index, direction);
    if (candidate >= 0 && candidate != excluded &&
        ecosystem->current[candidate].species == wanted) {
      candidates[count] = candidate;
      ++count;
    }
  }
  if (count == 0) {
    return -1;
  }
  return candidates[random_value(ecosystem->config.seed, tick, index, salt) %
                    (uint64_t)(unsigned int)count];
}

static int choose_carnivore_step(const Ecosystem *ecosystem, size_t index,
                                 int tick, uint64_t salt) {
  int candidates[8];
  int count = 0;
  int best_score = -1;

  for (int direction = 0; direction < 8; ++direction) {
    int candidate = neighbor_index(ecosystem, index, direction);
    if (candidate < 0 || ecosystem->current[candidate].species != SPECIES_EMPTY) {
      continue;
    }

    int score = 0;
    for (int look = 0; look < 8; ++look) {
      int neighbor = neighbor_index(ecosystem, (size_t)candidate, look);
      if (neighbor >= 0 &&
          ecosystem->current[neighbor].species == SPECIES_HERBIVORE) {
        ++score;
      }
    }

    if (score > best_score) {
      best_score = score;
      count = 0;
    }
    if (score == best_score) {
      candidates[count] = candidate;
      ++count;
    }
  }

  if (count == 0) {
    return -1;
  }
  return candidates[random_value(ecosystem->config.seed, tick, index, salt) %
                    (uint64_t)(unsigned int)count];
}

static Cell initial_cell(Species species) {
  int energy = 0;
  if (species == SPECIES_PLANT) {
    energy = 1;
  } else if (species == SPECIES_HERBIVORE) {
    energy = 5;
  } else if (species == SPECIES_CARNIVORE) {
    energy = 9;
  }
  return (Cell){species, energy, 0, 0};
}

static bool valid_cell(Cell cell) {
  return cell.species >= SPECIES_EMPTY && cell.species <= SPECIES_CARNIVORE &&
         cell.energy >= 0 && cell.age >= 0 && cell.hunger >= 0;
}

bool ecosystem_required_bytes(EcosystemConfig config,
                              uint64_t *required_bytes) {
  uint64_t cells;
  uint64_t bytes_per_cell = UINT64_C(2) * (uint64_t)sizeof(Cell) +
                            (uint64_t)sizeof(Intent) +
                            UINT64_C(2) * (uint64_t)sizeof(int) +
                            UINT64_C(2) * (uint64_t)sizeof(unsigned char);

  if (required_bytes == NULL || config.rows <= 0 || config.cols <= 0 ||
      config.threads <= 0) {
    return false;
  }
  cells =
      (uint64_t)(unsigned int)config.rows * (uint64_t)(unsigned int)config.cols;
  if (cells > (uint64_t)INT_MAX ||
      cells > (UINT64_MAX - (uint64_t)sizeof(Ecosystem)) / bytes_per_cell) {
    return false;
  }
  *required_bytes = (uint64_t)sizeof(Ecosystem) + cells * bytes_per_cell +
                    (uint64_t)(unsigned int)config.threads * (uint64_t)sizeof(double);
  return true;
}

static void decide_plant(const Ecosystem *ecosystem, size_t index, int tick,
                         Intent *intent) {
  int empty =
      choose_neighbor(ecosystem, index, SPECIES_EMPTY, -1, tick, UINT64_C(10));

  /* La regla formal elegida hace del espacio de Moore una condición de vida, no
   * solo de cría. */
  if (empty < 0) {
    intent->kind = INTENT_DIE;
    return;
  }
  if (random_chance(ecosystem, tick, index, UINT64_C(11),
                    PLANT_REPRODUCTION_PERCENT)) {
    intent->child_destination = empty;
  }
}

static void decide_animal(const Ecosystem *ecosystem, size_t index, int tick,
                          Intent *intent) {
  Cell cell = ecosystem->current[index];
  Species food =
      cell.species == SPECIES_HERBIVORE ? SPECIES_PLANT : SPECIES_HERBIVORE;
  int food_destination =
      choose_neighbor(ecosystem, index, food, -1, tick, UINT64_C(20));
  int movement_destination;
  int reproduction_energy;
  int reproduction_percent;

  if (food_destination >= 0) {
    intent->kind = INTENT_EAT;
    intent->destination = food_destination;
  } else {
    movement_destination = cell.species == SPECIES_CARNIVORE
                               ? choose_carnivore_step(ecosystem, index, tick,
                                                       UINT64_C(21))
                               : choose_neighbor(ecosystem, index, SPECIES_EMPTY, -1,
                                           tick, UINT64_C(21));
    if (movement_destination >= 0) {
      intent->kind = INTENT_MOVE;
      intent->destination = movement_destination;
    }
  }

  reproduction_energy = cell.species == SPECIES_HERBIVORE
                            ? HERBIVORE_REPRODUCTION_ENERGY
                            : CARNIVORE_REPRODUCTION_ENERGY;
  reproduction_percent = cell.species == SPECIES_HERBIVORE
                             ? HERBIVORE_REPRODUCTION_PERCENT
                             : CARNIVORE_REPRODUCTION_PERCENT;
  if (cell.energy >= reproduction_energy && cell.hunger == 0 &&
      random_chance(ecosystem, tick, index, UINT64_C(22),
                    reproduction_percent)) {
    intent->child_destination =
        choose_neighbor(ecosystem, index, SPECIES_EMPTY, intent->destination,
                        tick, UINT64_C(23));
  }
}

static void generate_intent(const Ecosystem *ecosystem, size_t index,
                            int tick) {
  Cell cell = ecosystem->current[index];
  Intent *intent = &ecosystem->intents[index];

  *intent = (Intent){INTENT_STAY, (int)index, -1};
  if (cell.species == SPECIES_EMPTY) {
    intent->kind = INTENT_DIE;
  } else if (cell.species == SPECIES_PLANT) {
    decide_plant(ecosystem, index, tick, intent);
  } else {
    decide_animal(ecosystem, index, tick, intent);
  }
}

/* Fase R: reinicia y arbitra recursos por menor índice lineal de origen. */
static void reset_resolution(Ecosystem *ecosystem) {
  memset(ecosystem->next, 0, ecosystem->total * sizeof(*ecosystem->next));
  memset(ecosystem->consumed, 0,
         ecosystem->total * sizeof(*ecosystem->consumed));
  memset(ecosystem->alive, 0, ecosystem->total * sizeof(*ecosystem->alive));
  for (size_t index = 0; index < ecosystem->total; ++index) {
    ecosystem->resource_winner[index] = -1;
    ecosystem->final_destination[index] = -1;
  }
}

static void resolve_predation(Ecosystem *ecosystem) {
  /* Los carnívoros resuelven primero para invalidar por completo a sus presas.
   */
  for (size_t source = 0; source < ecosystem->total; ++source) {
    Intent intent = ecosystem->intents[source];
    if (ecosystem->current[source].species == SPECIES_CARNIVORE &&
        intent.kind == INTENT_EAT) {
      int winner = ecosystem->resource_winner[intent.destination];
      if (winner < 0 || source < (size_t)winner) {
        ecosystem->resource_winner[intent.destination] = (int)source;
      }
    }
  }
  for (size_t prey = 0; prey < ecosystem->total; ++prey) {
    if (ecosystem->resource_winner[prey] >= 0) {
      ecosystem->consumed[prey] = 1U;
    }
  }

  /* Solo herbívoros no consumidos pueden comprometer una planta. */
  for (size_t source = 0; source < ecosystem->total; ++source) {
    Intent intent = ecosystem->intents[source];
    if (ecosystem->current[source].species == SPECIES_HERBIVORE &&
        ecosystem->consumed[source] == 0U && intent.kind == INTENT_EAT) {
      int winner = ecosystem->resource_winner[intent.destination];
      if (winner < 0 || source < (size_t)winner) {
        ecosystem->resource_winner[intent.destination] = (int)source;
      }
    }
  }
  for (size_t prey = 0; prey < ecosystem->total; ++prey) {
    int winner = ecosystem->resource_winner[prey];
    if (winner >= 0 &&
        ecosystem->current[winner].species == SPECIES_HERBIVORE) {
      ecosystem->consumed[prey] = 1U;
    }
  }
}

static bool survives_tick(const Ecosystem *ecosystem, size_t source, bool ate) {
  Cell cell = ecosystem->current[source];
  int new_age = cell.age < INT_MAX ? cell.age + 1 : INT_MAX;
  int new_hunger = ate ? 0 : cell.hunger < INT_MAX ? cell.hunger + 1 : INT_MAX;
  int new_energy = ate ? cell.energy : cell.energy - 1;
  int max_age =
      cell.species == SPECIES_HERBIVORE ? HERBIVORE_MAX_AGE : CARNIVORE_MAX_AGE;
  int starvation = cell.species == SPECIES_HERBIVORE
                       ? HERBIVORE_STARVATION_TICKS
                       : CARNIVORE_STARVATION_TICKS;

  return new_age < max_age && new_hunger < starvation && new_energy > 0;
}

/* Fase V: valida supervivencia y destino; perder movimiento conserva origen. */
static void validate_survivors(Ecosystem *ecosystem) {
  for (size_t source = 0; source < ecosystem->total; ++source) {
    Cell cell = ecosystem->current[source];
    Intent intent = ecosystem->intents[source];
    bool ate = intent.kind == INTENT_EAT &&
               ecosystem->resource_winner[intent.destination] == (int)source;

    if (cell.species == SPECIES_EMPTY || intent.kind == INTENT_DIE ||
        ecosystem->consumed[source] != 0U) {
      continue;
    }
    if (cell.species == SPECIES_PLANT ||
        survives_tick(ecosystem, source, ate)) {
      ecosystem->alive[source] = 1U;
    }
  }
}

static void resolve_movements(Ecosystem *ecosystem) {
  for (size_t source = 0; source < ecosystem->total; ++source) {
    Intent intent = ecosystem->intents[source];
    if (ecosystem->alive[source] != 0U && intent.kind == INTENT_MOVE) {
      int winner = ecosystem->resource_winner[intent.destination];
      if (winner < 0 || source < (size_t)winner) {
        ecosystem->resource_winner[intent.destination] = (int)source;
      }
    }
  }

  for (size_t source = 0; source < ecosystem->total; ++source) {
    Intent intent = ecosystem->intents[source];
    if (ecosystem->alive[source] == 0U) {
      continue;
    }
    if (intent.kind == INTENT_EAT &&
        ecosystem->resource_winner[intent.destination] == (int)source) {
      ecosystem->final_destination[source] = intent.destination;
    } else if (intent.kind == INTENT_MOVE &&
               ecosystem->resource_winner[intent.destination] == (int)source) {
      ecosystem->final_destination[source] = intent.destination;
    } else {
      /* Perder un arbitraje nunca elimina al actor: conserva su celda de
       * origen. */
      ecosystem->final_destination[source] = (int)source;
    }
  }
}

static Cell advanced_cell(Cell cell, bool ate) {
  if (cell.age < INT_MAX) {
    ++cell.age;
  }
  if (cell.species != SPECIES_PLANT) {
    if (ate) {
      int energy_gain = cell.species == SPECIES_HERBIVORE ? 1 : 2;
      cell.energy = cell.energy <= INT_MAX - energy_gain
                        ? cell.energy + energy_gain
                        : INT_MAX;
      cell.hunger = 0;
    } else {
      --cell.energy;
      if (cell.hunger < INT_MAX) {
        ++cell.hunger;
      }
    }
  }
  return cell;
}

/* Fase C: un único escritor materializa actores antes que nacimientos. */
static void construct_next_grid(Ecosystem *ecosystem) {
  for (size_t source = 0; source < ecosystem->total; ++source) {
    int destination = ecosystem->final_destination[source];
    if (destination >= 0) {
      Intent intent = ecosystem->intents[source];
      bool ate = intent.kind == INTENT_EAT && destination == intent.destination;
      ecosystem->next[destination] =
          advanced_cell(ecosystem->current[source], ate);
    }
  }

  /* Los movimientos ya comprometidos tienen precedencia sobre nacimientos
   * opcionales. */
  for (size_t parent = 0; parent < ecosystem->total; ++parent) {
    int child_destination = ecosystem->intents[parent].child_destination;
    if (ecosystem->alive[parent] != 0U && child_destination >= 0 &&
        ecosystem->next[child_destination].species == SPECIES_EMPTY &&
        ecosystem->resource_winner[child_destination] < 0) {
      ecosystem->resource_winner[child_destination] = (int)parent;
    }
  }
  for (size_t parent = 0; parent < ecosystem->total; ++parent) {
    int child_destination = ecosystem->intents[parent].child_destination;
    if (ecosystem->alive[parent] != 0U && child_destination >= 0 &&
        ecosystem->resource_winner[child_destination] == (int)parent) {
      ecosystem->next[child_destination] =
          initial_cell(ecosystem->current[parent].species);
    }
  }
}

Ecosystem *ecosystem_create(EcosystemConfig config) {
  Ecosystem *ecosystem;
  uint64_t required_bytes;
  size_t rows;
  size_t cols;

  if (!ecosystem_required_bytes(config, &required_bytes) ||
      required_bytes > ECOSYSTEM_MEMORY_BUDGET_BYTES) {
    return NULL;
  }
  rows = (size_t)config.rows;
  cols = (size_t)config.cols;
  if (rows > SIZE_MAX / cols || rows * cols > (size_t)INT_MAX) {
    return NULL;
  }

  ecosystem = calloc(1U, sizeof(*ecosystem));
  if (ecosystem == NULL) {
    return NULL;
  }
  ecosystem->config = config;
  ecosystem->total = rows * cols;
  ecosystem->current = calloc(ecosystem->total, sizeof(*ecosystem->current));
  ecosystem->next = calloc(ecosystem->total, sizeof(*ecosystem->next));
  ecosystem->intents = calloc(ecosystem->total, sizeof(*ecosystem->intents));
  ecosystem->resource_winner =
      malloc(ecosystem->total * sizeof(*ecosystem->resource_winner));
  ecosystem->final_destination =
      malloc(ecosystem->total * sizeof(*ecosystem->final_destination));
  ecosystem->consumed = calloc(ecosystem->total, sizeof(*ecosystem->consumed));
  ecosystem->alive = calloc(ecosystem->total, sizeof(*ecosystem->alive));
  ecosystem->thread_seconds =
      calloc((size_t)config.threads, sizeof(*ecosystem->thread_seconds));
  if (ecosystem->current == NULL || ecosystem->next == NULL ||
      ecosystem->intents == NULL || ecosystem->resource_winner == NULL ||
      ecosystem->final_destination == NULL || ecosystem->consumed == NULL ||
      ecosystem->alive == NULL || ecosystem->thread_seconds == NULL) {
    ecosystem_destroy(ecosystem);
    return NULL;
  }
  return ecosystem;
}

void ecosystem_destroy(Ecosystem *ecosystem) {
  if (ecosystem == NULL) {
    return;
  }
  free(ecosystem->current);
  free(ecosystem->next);
  free(ecosystem->intents);
  free(ecosystem->resource_winner);
  free(ecosystem->final_destination);
  free(ecosystem->consumed);
  free(ecosystem->alive);
  free(ecosystem->thread_seconds);
  free(ecosystem);
}

bool ecosystem_populate(Ecosystem *ecosystem, Population population) {
  int counts[3] = {population.plants, population.herbivores,
                   population.carnivores};
  size_t requested;
  size_t placed = 0U;

  if (ecosystem == NULL || population.plants < 0 || population.herbivores < 0 ||
      population.carnivores < 0) {
    return false;
  }
  requested = (size_t)population.plants + (size_t)population.herbivores +
              (size_t)population.carnivores;
  if (requested > ecosystem->total) {
    return false;
  }
  memset(ecosystem->current, 0, ecosystem->total * sizeof(*ecosystem->current));
  for (int species_offset = 0; species_offset < 3; ++species_offset) {
    Species species = (Species)(species_offset + 1);
    for (int number = 0; number < counts[species_offset]; ++number) {
      size_t position = random_value(ecosystem->config.seed, -1, placed,
                                     (uint64_t)species_offset) %
                        ecosystem->total;
      while (ecosystem->current[position].species != SPECIES_EMPTY) {
        position = (position + 1U) % ecosystem->total;
      }
      ecosystem->current[position] = initial_cell(species);
      ++placed;
    }
  }
  return true;
}

bool ecosystem_tick(Ecosystem *ecosystem, int tick) {
  Cell *swap;

  if (ecosystem == NULL || tick <= 0) {
    return false;
  }

  omp_set_dynamic(0);
  omp_set_num_threads(ecosystem->config.threads);
  /* Fase D: cada iteración escribe Intent[i] y solo lee CurrentGrid inmutable.
   * La barrera implícita entrega todas las intenciones antes de resolverlas.
   * Cada hilo acumula su propio tiempo en thread_seconds para poder reportar
   * el reparto de carga entre hilos al final de la simulación. */
#pragma omp parallel
  {
    int thread_id = omp_get_thread_num();
    double thread_start = omp_get_wtime();

#pragma omp for schedule(static)
    for (int row = 0; row < ecosystem->config.rows; ++row) {
      size_t row_start = (size_t)row * (size_t)ecosystem->config.cols;
      for (int col = 0; col < ecosystem->config.cols; ++col) {
        generate_intent(ecosystem, row_start + (size_t)col, tick);
      }
    }
    ecosystem->thread_seconds[thread_id] += omp_get_wtime() - thread_start;
  }

  /* R/V/C son seriales deliberadamente: expresan un orden total reproducible.
   */
  reset_resolution(ecosystem);
  resolve_predation(ecosystem);
  validate_survivors(ecosystem);
  resolve_movements(ecosystem);
  construct_next_grid(ecosystem);
  swap = ecosystem->current;
  ecosystem->current = ecosystem->next;
  ecosystem->next = swap;
  return true;
}

bool ecosystem_set_cell(Ecosystem *ecosystem, int row, int col, Cell cell) {
  if (ecosystem == NULL || row < 0 || row >= ecosystem->config.rows ||
      col < 0 || col >= ecosystem->config.cols || !valid_cell(cell)) {
    return false;
  }
  ecosystem
      ->current[(size_t)row * (size_t)ecosystem->config.cols + (size_t)col] =
      cell;
  return true;
}

Cell ecosystem_get_cell(const Ecosystem *ecosystem, int row, int col) {
  if (ecosystem == NULL || row < 0 || row >= ecosystem->config.rows ||
      col < 0 || col >= ecosystem->config.cols) {
    return (Cell){SPECIES_EMPTY, 0, 0, 0};
  }
  return ecosystem
      ->current[(size_t)row * (size_t)ecosystem->config.cols + (size_t)col];
}

Population ecosystem_count(const Ecosystem *ecosystem) {
  int plants = 0;
  int herbivores = 0;
  int carnivores = 0;

  if (ecosystem == NULL) {
    return (Population){0, 0, 0};
  }
  omp_set_num_threads(ecosystem->config.threads);
  /* La reducción evita contención: cada hilo acumula localmente y OpenMP
   * combina al final. */
#pragma omp parallel for reduction(+ : plants, herbivores, carnivores)         \
    schedule(static)
  for (int index = 0; index < (int)ecosystem->total; ++index) {
    Species species = ecosystem->current[index].species;
    plants += species == SPECIES_PLANT ? 1 : 0;
    herbivores += species == SPECIES_HERBIVORE ? 1 : 0;
    carnivores += species == SPECIES_CARNIVORE ? 1 : 0;
  }
  return (Population){plants, herbivores, carnivores};
}

bool ecosystem_print(const Ecosystem *ecosystem, int tick, FILE *output) {
  Population population;

  if (ecosystem == NULL || output == NULL || tick < 0) {
    return false;
  }
  population = ecosystem_count(ecosystem);
  if (fprintf(output,
              "Tick %d\nPlantas: %d\nHerbívoros: %d\nCarnívoros: "
              "%d\nDistribución:\n",
              tick, population.plants, population.herbivores,
              population.carnivores) < 0) {
    return false;
  }
  for (int row = 0; row < ecosystem->config.rows; ++row) {
    for (int col = 0; col < ecosystem->config.cols; ++col) {
      Species species = ecosystem_get_cell(ecosystem, row, col).species;
      int symbol = species == SPECIES_PLANT       ? 'P'
                   : species == SPECIES_HERBIVORE ? 'H'
                   : species == SPECIES_CARNIVORE ? 'C'
                                                  : '.';
      if (fputc(symbol, output) == EOF ||
          (col + 1 < ecosystem->config.cols && fputc(' ', output) == EOF)) {
        return false;
      }
    }
    if (fputc('\n', output) == EOF) {
      return false;
    }
  }
  return fputc('\n', output) != EOF;
}

bool ecosystem_print_thread_times(const Ecosystem *ecosystem, FILE *output) {
  if (ecosystem == NULL || output == NULL) {
    return false;
  }
  if (fprintf(output, "Tiempo por hilo (fase D, acumulado):\n") < 0) {
    return false;
  }
  for (int thread = 0; thread < ecosystem->config.threads; ++thread) {
    if (fprintf(output, "  Hilo %d: %.6f segundos\n", thread,
                ecosystem->thread_seconds[thread]) < 0) {
      return false;
    }
  }
  return true;
}
