# SPDX-License-Identifier: MIT
# Copyright (c) 2026 MaIII Themd

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Wstrict-prototypes -O2
LDLIBS   = -lm

TARGET = check
SRC    = astro.c example/check.c

$(TARGET): $(SRC) astro.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o example/*.o

.PHONY: test clean
