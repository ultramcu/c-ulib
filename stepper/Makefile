# SPDX-License-Identifier: MIT
# Copyright (c) 2026 MaIII Themd

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Wstrict-prototypes -O2

TARGET = check
SRC    = stepper.c example/check.c

$(TARGET): $(SRC) stepper.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o example/*.o

.PHONY: test clean
