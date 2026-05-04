# SPDX-License-Identifier: MIT
# Copyright (c) 2026 MaIII Themd
#
# c-ulib top-level Makefile -- delegates to each module.
#
#   make             build every module's example/self-test
#   make test        build + run them
#   make clean       drop every module's artefacts

MODULES := mqtt ujson

all:
	@for m in $(MODULES); do \
		printf "==> %s\n" $$m; \
		$(MAKE) -C $$m || exit $$?; \
	done

test:
	@for m in $(MODULES); do \
		printf "==> %s test\n" $$m; \
		$(MAKE) -C $$m test || exit $$?; \
	done

clean:
	@for m in $(MODULES); do \
		$(MAKE) -C $$m clean || exit $$?; \
	done

.PHONY: all test clean
