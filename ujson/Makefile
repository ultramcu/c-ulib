# SPDX-License-Identifier: MIT
# Copyright (c) 2026 MaIII Themd

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2

TARGET = ujson_demo
SRC    = ujson.c example/main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	@./$(TARGET) > /dev/null && echo "ujson demo OK"

clean:
	rm -f $(TARGET) *.o example/*.o

.PHONY: test clean
