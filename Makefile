CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11

ecosystem: main.c
	$(CC) $(CFLAGS) -fopenmp main.c -o ecosystem

clean:
	$(RM) ecosystem ecosystem.exe

.PHONY: clean
