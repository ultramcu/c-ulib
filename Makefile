# SPDX-License-Identifier: MIT
# Copyright (c) 2026 MaIII Themd
#
# c-mqtt-lite -- Makefile for the example self-test.
#
#   make           build ./build_packets
#   make test      build and run it (also exits non-zero on a mismatch)
#   make clean

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Wstrict-prototypes -O2

TARGET = build_packets
SRC    = mqtt.c examples/build_packets.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o examples/*.o

.PHONY: test clean
