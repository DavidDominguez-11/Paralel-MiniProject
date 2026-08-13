CC ?= gcc
CPPFLAGS ?= -I.
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
OPENMP_FLAGS ?= -fopenmp
LDLIBS ?=
CLANG_FORMAT ?= clang-format

TARGET := ecosystem
TEST_TARGET := test_ecosystem
COMMON_OBJECTS := ecosystem.o
C_SOURCES := main.c ecosystem.c ecosystem.h tests/test_ecosystem.c

all: $(TARGET)

$(TARGET): main.o $(COMMON_OBJECTS)
	$(CC) $(OPENMP_FLAGS) $^ $(LDLIBS) -o $@

$(TEST_TARGET): tests/test_ecosystem.o $(COMMON_OBJECTS)
	$(CC) $(OPENMP_FLAGS) $^ $(LDLIBS) -o $@

main.o ecosystem.o tests/test_ecosystem.o: ecosystem.h

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OPENMP_FLAGS) -c $< -o $@

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET)
	sh ./tests/test_determinism.sh ./$(TARGET)

# clang-format es opcional y nunca forma parte de la construcción ordinaria.
format:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { \
		echo "Error: no se encontró $(CLANG_FORMAT)." >&2; exit 1; \
	}
	$(CLANG_FORMAT) -i $(C_SOURCES)

check-format:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { \
		echo "Error: no se encontró $(CLANG_FORMAT)." >&2; exit 1; \
	}
	$(CLANG_FORMAT) --dry-run --Werror $(C_SOURCES)

clean:
	$(RM) $(TARGET) $(TEST_TARGET) main.o ecosystem.o tests/test_ecosystem.o

.PHONY: all check-format clean format test