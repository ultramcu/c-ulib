# SPDX-License-Identifier: MIT
# Copyright (c) 2026 MaIII Themd

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Wstrict-prototypes -O2

# byte_stream.h is a sibling module in the same umbrella repo.
CPPFLAGS += -I../byte_stream

TARGET = build_packets
SRC    = mqtt.c examples/build_packets.c

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o examples/*.o

.PHONY: test clean
