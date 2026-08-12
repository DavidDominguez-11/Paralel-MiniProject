#include "ecosystem.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <omp.h>
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
  double process_start;
  double process_elapsed;

  if (fprintf(output, "Configuración: %dx%d, ticks=%d, threads=%d, seed=%" PRIu64
                      "\n\n",
              config->ecosystem.rows, config->ecosystem.cols, config->ticks,
              config->ecosystem.threads, config->ecosystem.seed) < 0 ||
      !ecosystem_print(ecosystem, 0, output)) {
    return false;
  }
  process_start = omp_get_wtime();
  for (int tick = 1; tick <= config->ticks;) {
    double tick_start = omp_get_wtime();
    bool tick_ok = ecosystem_tick(ecosystem, tick);
    double tick_elapsed = omp_get_wtime() - tick_start;

    if (!tick_ok || !ecosystem_print(ecosystem, tick, output) ||
        fprintf(output, "Tiempo del tick %d: %.6f segundos\n\n", tick,
                tick_elapsed) < 0) {
      return false;
    }
    /* El último tick termina antes del incremento, incluso cuando vale INT_MAX.
     */
    if (tick == config->ticks) {
      break;
    }
    ++tick;
  }
  process_elapsed = omp_get_wtime() - process_start;
  return ecosystem_print_thread_times(ecosystem, output) &&
         fprintf(output, "Tiempo total del proceso: %.6f segundos\n",
                 process_elapsed) >= 0;
}

/* Vuelca el búfer de salida al destino indicado; reporta el error con perror
 * usando `label` cuando la escritura o el cierre fallan. */
static bool dump_buffer(const char *path, const char *label, const char *data,
                        size_t size) {
  FILE *destination = fopen(path, "w");
  if (destination == NULL) {
    perror(label);
    return false;
  }
  bool ok = fwrite(data, 1, size, destination) == size;
  if (fclose(destination) != 0) {
    ok = false;
  }
  if (!ok) {
    perror(label);
  }
  return ok;
}

int main(int argc, char **argv) {
  CliConfig config = {{20, 40, 1, UINT64_C(20240820)}, {150, 40, 15}, 20, NULL};
  Ecosystem *ecosystem;
  char *buffer = NULL;
  size_t buffer_size = 0;
  FILE *output;
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

  output = open_memstream(&buffer, &buffer_size);
  if (output == NULL) {
    fprintf(stderr, "No se pudo preparar el búfer de salida.\n");
    ecosystem_destroy(ecosystem);
    return 1;
  }
  success = run_simulation(ecosystem, &config, output);
  ecosystem_destroy(ecosystem);
  if (fclose(output) != 0) {
    success = false;
  }
  if (!success) {
    fprintf(stderr, "Falló la escritura de la simulación.\n");
    free(buffer);
    return 1;
  }

  /* La salida siempre se muestra en consola y se guarda en run.txt; si se
   * pidió --output, también se escribe en el archivo solicitado. */
  fwrite(buffer, 1, buffer_size, stdout);
  success = dump_buffer("run.txt", "run.txt", buffer, buffer_size);
  if (success && config.output_path != NULL) {
    success = dump_buffer(config.output_path, config.output_path, buffer,
                          buffer_size);
  }
  free(buffer);
  return success ? 0 : 1;
}
