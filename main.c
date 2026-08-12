#include "ecosystem.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  EcosystemConfig ecosystem;
  Population population;
  int ticks;
  const char *output_path;
} CliConfig;

static void print_usage(const char *program) {
  fprintf(stderr,
          "Uso: %s [--rows N] [--cols N] [--ticks N] [--plants N]\n"
          "          [--herbivores N] [--carnivores N] [--threads N]\n"
          "          [--seed N] [--output ARCHIVO]\n",
          program);
}

static bool parse_int(const char *text, int minimum, int *result) {
  char *end = NULL;
  long value;

  errno = 0;
  value = strtol(text, &end, 10);
  if (errno == ERANGE || end == text || *end != '\0' || value < minimum ||
      value > INT_MAX) {
    return false;
  }
  *result = (int)value;
  return true;
}

static bool parse_seed(const char *text, uint64_t *result) {
  char *end = NULL;
  uintmax_t value;

  if (text[0] == '-' || text[0] == '\0') {
    return false;
  }
  errno = 0;
  value = strtoumax(text, &end, 10);
  if (errno == ERANGE || *end != '\0' || value > UINT64_MAX) {
    return false;
  }
  *result = (uint64_t)value;
  return true;
}

static bool assign_option(CliConfig *config, const char *option,
                          const char *value) {
  if (strcmp(option, "--rows") == 0) {
    return parse_int(value, 1, &config->ecosystem.rows);
  }
  if (strcmp(option, "--cols") == 0) {
    return parse_int(value, 1, &config->ecosystem.cols);
  }
  if (strcmp(option, "--ticks") == 0) {
    return parse_int(value, 0, &config->ticks);
  }
  if (strcmp(option, "--plants") == 0) {
    return parse_int(value, 0, &config->population.plants);
  }
  if (strcmp(option, "--herbivores") == 0) {
    return parse_int(value, 0, &config->population.herbivores);
  }
  if (strcmp(option, "--carnivores") == 0) {
    return parse_int(value, 0, &config->population.carnivores);
  }
  if (strcmp(option, "--threads") == 0) {
    return parse_int(value, 1, &config->ecosystem.threads);
  }
  if (strcmp(option, "--seed") == 0) {
    return parse_seed(value, &config->ecosystem.seed);
  }
  if (strcmp(option, "--output") == 0 && value[0] != '\0') {
    config->output_path = value;
    return true;
  }
  return false;
}

static bool parse_cli(int argc, char **argv, CliConfig *config) {
  /* La interfaz acepta exclusivamente pares --opción valor. */
  for (int index = 1; index < argc; index += 2) {
    if (index + 1 >= argc ||
        !assign_option(config, argv[index], argv[index + 1])) {
      return false;
    }
  }
  return true;
}

static bool population_fits(const CliConfig *config) {
  uint64_t capacity = (uint64_t)(unsigned int)config->ecosystem.rows *
                      (uint64_t)(unsigned int)config->ecosystem.cols;
  uint64_t requested = (uint64_t)(unsigned int)config->population.plants +
                       (uint64_t)(unsigned int)config->population.herbivores +
                       (uint64_t)(unsigned int)config->population.carnivores;
  return capacity <= (uint64_t)INT_MAX && requested <= capacity;
}

static bool memory_fits(const CliConfig *config) {
  uint64_t required_bytes;

  if (!ecosystem_required_bytes(config->ecosystem, &required_bytes)) {
    fprintf(
        stderr,
        "Las dimensiones de la cuadrícula exceden los límites del motor.\n");
    return false;
  }
  if (required_bytes > ECOSYSTEM_MEMORY_BUDGET_BYTES) {
    fprintf(stderr,
            "La cuadrícula requiere %" PRIu64
            " bytes; el presupuesto del motor es %" PRIu64 " bytes.\n",
            required_bytes, ECOSYSTEM_MEMORY_BUDGET_BYTES);
    return false;
  }
  return true;
}

static bool run_simulation(Ecosystem *ecosystem, const CliConfig *config,
                           FILE *output) {
  if (fprintf(output, "Configuración: %dx%d, ticks=%d, seed=%" PRIu64 "\n\n",
              config->ecosystem.rows, config->ecosystem.cols, config->ticks,
              config->ecosystem.seed) < 0 ||
      !ecosystem_print(ecosystem, 0, output)) {
    return false;
  }
  for (int tick = 1; tick <= config->ticks;) {
    if (!ecosystem_tick(ecosystem, tick) ||
        !ecosystem_print(ecosystem, tick, output)) {
      return false;
    }
    /* El último tick termina antes del incremento, incluso cuando vale INT_MAX.
     */
    if (tick == config->ticks) {
      break;
    }
    ++tick;
  }
  return true;
}

int main(int argc, char **argv) {
  CliConfig config = {{20, 40, 1, UINT64_C(20240820)}, {150, 40, 15}, 20, NULL};
  Ecosystem *ecosystem;
  FILE *output = stdout;
  bool success;

  if (!parse_cli(argc, argv, &config) || !population_fits(&config)) {
    fprintf(stderr, "Configuración inválida o argumento mal formado.\n");
    print_usage(argv[0]);
    return 2;
  }
  if (!memory_fits(&config)) {
    return 2;
  }
  ecosystem = ecosystem_create(config.ecosystem);
  if (ecosystem == NULL) {
    fprintf(stderr, "No se pudo reservar memoria para la cuadrícula.\n");
    return 1;
  }
  if (!ecosystem_populate(ecosystem, config.population)) {
    fprintf(stderr, "No se pudo inicializar la población solicitada.\n");
    ecosystem_destroy(ecosystem);
    return 1;
  }
  if (config.output_path != NULL) {
    output = fopen(config.output_path, "w");
    if (output == NULL) {
      perror(config.output_path);
      ecosystem_destroy(ecosystem);
      return 1;
    }
  }

  success = run_simulation(ecosystem, &config, output);
  /* Cerrar o vaciar también forma parte del resultado observable de E/S. */
  if (config.output_path != NULL && fclose(output) != 0) {
    perror(config.output_path);
    success = false;
  } else if (config.output_path == NULL && fflush(output) != 0) {
    perror("stdout");
    success = false;
  }
  ecosystem_destroy(ecosystem);
  if (!success) {
    fprintf(stderr, "Falló la escritura de la simulación.\n");
    return 1;
  }
  return 0;
}
