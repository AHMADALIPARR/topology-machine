# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors

CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -Iinclude
LDFLAGS ?=
LDLIBS ?= -lm

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
LIBEXECDIR ?= $(PREFIX)/libexec
DATADIR ?= $(PREFIX)/share/topology-machine
MANDIR ?= $(PREFIX)/share/man
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig
DESTDIR ?=

LIB_SRCS = src/util.c src/dag.c src/expr.c src/gf256.c src/circuit.c \
	src/reversible.c src/quantum.c src/analog.c src/smt.c src/oracle.c \
	src/selfcheck.c src/bench.c
LIB_OBJS = $(patsubst src/%.c,build/%.o,$(LIB_SRCS))

TEST_NAMES = dag expr gf circuit rev quantum analog smt oracle selfcheck
TEST_BINS = $(patsubst %,build/test_%,$(TEST_NAMES))

KG = third_party/klong/kg
INSTALL = install

.PHONY: all test test-san test-klong test-full verify bench assets ci install uninstall clean klong help restore-bytes

# Decode nstat.kg and klong.png before build, test, or install. `sh` does not
# need the script's executable bit, which a mode-less upload drops.
restore-bytes:
	sh scripts/restore-upstream-bytes.sh

all: restore-bytes build/tm $(KG)

help:
	@echo "make all test test-san test-klong bench install uninstall clean"

build:
	mkdir -p build build/san

build/%.o: src/%.c include/tm.h | build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

build/libtm.a: $(LIB_OBJS)
	ar rcs $@ $(LIB_OBJS)

build/tm: src/cli.c build/libtm.a
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ src/cli.c build/libtm.a $(LDFLAGS) $(LDLIBS)

build/test_%: tests/test_%.c build/libtm.a include/tm.h tests/check.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -o $@ tests/test_$*.c build/libtm.a $(LDFLAGS) $(LDLIBS)

$(KG):
	$(MAKE) -C third_party/klong kg

klong: $(KG)

test: all $(TEST_BINS)
	@set -e; for t in $(TEST_BINS); do echo "== $$t"; ./$$t; done
	$(MAKE) test-klong
	TM=./build/tm bash tests/cli.sh
	TM=./build/tm KG=$(KG) bash tests/integration.sh

test-full: test

test-klong: $(KG) build/tm
	@mkdir -p build
	$(KG) -n -q klong/oracle.kg > build/klong-oracle.txt
	./build/tm oracle > build/native-oracle.txt
	diff -u build/native-oracle.txt build/klong-oracle.txt
	$(KG) -n -q examples/arrays.kg > build/arrays.txt
	printf '13\n[1 1 2 3]\n' > build/arrays-expect.txt
	diff -u build/arrays-expect.txt build/arrays.txt
	$(MAKE) -C third_party/klong test

# `cc` may be clang without compiler-rt. gcc links libasan/libubsan on this image.
SAN_CC ?= gcc
SAN_CFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer \
	-fsanitize=address,undefined -fno-sanitize-recover=undefined
SAN_LDFLAGS = -fsanitize=address,undefined

build/san/%.o: src/%.c include/tm.h
	mkdir -p build/san
	$(SAN_CC) $(SAN_CFLAGS) $(CPPFLAGS) -c -o $@ $<

build/san/libtm.a: $(patsubst src/%.c,build/san/%.o,$(LIB_SRCS))
	ar rcs $@ $^

build/san/tm: src/cli.c build/san/libtm.a
	$(SAN_CC) $(SAN_CFLAGS) $(CPPFLAGS) -o $@ src/cli.c build/san/libtm.a $(SAN_LDFLAGS) $(LDLIBS)

build/san/test_%: tests/test_%.c build/san/libtm.a
	$(SAN_CC) $(SAN_CFLAGS) $(CPPFLAGS) -Itests -o $@ tests/test_$*.c build/san/libtm.a $(SAN_LDFLAGS) $(LDLIBS)

test-san: $(patsubst %,build/san/test_%,$(TEST_NAMES)) build/san/tm
	@set -e; for t in $(patsubst %,build/san/test_%,$(TEST_NAMES)); do echo "== $$t"; ./$$t; done
	mkdir -p build/san/out
	./build/san/tm self-check --out-dir build/san/out

verify: test
	@./build/tm smt involution --out build/reversible.smt2 >/dev/null
	@if command -v bitwuzla >/dev/null 2>&1; then \
		echo "bitwuzla:"; bitwuzla build/reversible.smt2; \
	elif command -v boolector >/dev/null 2>&1; then \
		echo "boolector:"; boolector build/reversible.smt2; \
	elif command -v z3 >/dev/null 2>&1; then \
		echo "z3:"; z3 build/reversible.smt2; \
	else \
		echo "SMT solver unavailable; native 8-state reading stands, no external solver proof"; \
	fi

bench: build/tm
	mkdir -p bench/results
	{ echo "$(shell ./build/tm version)"; \
	  echo "date_utc $(shell date -u +%Y-%m-%dT%H:%M:%SZ)"; \
	  uname -srm; \
	  $(CC) --version | head -n 1; \
	  echo "cflags $(CFLAGS)"; \
	  echo "runs 5"; \
	  awk -F: '/model name/ {print $$0; exit}' /proc/cpuinfo; \
	} > bench/results/environment.txt
	rm -f bench/results/bench-runs.txt
	i=1; while [ $$i -le 5 ]; do \
	  echo "# run $$i" | tee -a bench/results/bench-runs.txt; \
	  ./build/tm bench | tee -a bench/results/bench-runs.txt; \
	  i=$$((i+1)); \
	done
	cp bench/results/bench-runs.txt bench/results/bench.txt

assets: build/tm
	python3 scripts/regen_assets.py

ci: test test-san verify

install: all
	$(INSTALL) -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR) \
		$(DESTDIR)$(LIBEXECDIR) $(DESTDIR)$(DATADIR)/klong \
		$(DESTDIR)$(DATADIR)/klong-lib $(DESTDIR)$(DATADIR)/examples \
		$(DESTDIR)$(MANDIR)/man1 $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL) -m 755 build/tm $(DESTDIR)$(BINDIR)/tm
	$(INSTALL) -m 755 $(KG) $(DESTDIR)$(LIBEXECDIR)/tm-kg.bin
	$(INSTALL) -m 644 build/libtm.a $(DESTDIR)$(LIBDIR)/libtm.a
	$(INSTALL) -m 644 include/tm.h $(DESTDIR)$(INCLUDEDIR)/tm.h
	$(INSTALL) -m 644 man/tm.1 $(DESTDIR)$(MANDIR)/man1/tm.1
	$(INSTALL) -m 644 klong/oracle.kg $(DESTDIR)$(DATADIR)/klong/oracle.kg
	$(INSTALL) -m 644 examples/arrays.kg $(DESTDIR)$(DATADIR)/examples/arrays.kg
	$(INSTALL) -m 644 examples/gf256.tmnl $(DESTDIR)$(DATADIR)/examples/gf256.tmnl
	$(INSTALL) -m 755 examples/demo.sh $(DESTDIR)$(DATADIR)/examples/demo.sh
	$(INSTALL) -m 644 third_party/klong/lib/*.kg $(DESTDIR)$(DATADIR)/klong-lib/
	$(INSTALL) -m 755 packaging/tm-kg.in $(DESTDIR)$(BINDIR)/tm-kg
	sed -e 's|@PREFIX@|$(PREFIX)|g' -e 's|@LIBDIR@|$(LIBDIR)|g' \
		-e 's|@INCLUDEDIR@|$(INCLUDEDIR)|g' packaging/tm.pc.in > $(DESTDIR)$(PKGCONFIGDIR)/tm.pc

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/tm $(DESTDIR)$(BINDIR)/tm-kg
	rm -f $(DESTDIR)$(LIBEXECDIR)/tm-kg.bin
	rm -f $(DESTDIR)$(LIBDIR)/libtm.a
	rm -f $(DESTDIR)$(INCLUDEDIR)/tm.h
	rm -f $(DESTDIR)$(MANDIR)/man1/tm.1
	rm -f $(DESTDIR)$(PKGCONFIGDIR)/tm.pc
	rm -rf $(DESTDIR)$(DATADIR)

clean:
	rm -rf build
	$(MAKE) -C third_party/klong clean
