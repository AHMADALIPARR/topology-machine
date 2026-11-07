#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$ROOT"
TM=${TM:-./build/tm}
KG=${KG:-./third_party/klong/kg}

mkdir -p build/it-out
"$TM" self-check --out-dir build/it-out >/tmp/tm-self.out
diff -u tests/golden/bell.qasm build/it-out/bell.qasm
diff -u tests/golden/reversible.qasm build/it-out/reversible.qasm
diff -u tests/golden/reversible.smt2 build/it-out/reversible.smt2
diff -u examples/gf256.tmnl build/it-out/gf256.tmnl
grep -q "PASS: all native checks" /tmp/tm-self.out
"$TM" qasm sim build/it-out/bell.qasm | grep -q "prob 00 0.500000000000"
"$TM" qasm sim build/it-out/bell.qasm | grep -q "prob 11 0.500000000000"
"$TM" analog solve --g '3,-1;-1,2' --i '1,0' | grep -q "0.400000000000"

PREFIX=/usr/local
STAGE=$(mktemp -d)
make install DESTDIR="$STAGE" PREFIX="$PREFIX" >/tmp/tm-install.out
test -x "$STAGE$PREFIX/bin/tm"
test -x "$STAGE$PREFIX/bin/tm-kg"
test -f "$STAGE$PREFIX/include/tm.h"
test -f "$STAGE$PREFIX/lib/libtm.a"
test -f "$STAGE$PREFIX/share/man/man1/tm.1"
"$STAGE$PREFIX/bin/tm" version | grep -qx "tm 1.0.0"
"$STAGE$PREFIX/bin/tm-kg" -n -q "$ROOT/klong/oracle.kg" > "$STAGE/oracle.txt"
"$STAGE$PREFIX/bin/tm" oracle > "$STAGE/native.txt"
diff -u "$STAGE/native.txt" "$STAGE/oracle.txt"
make uninstall DESTDIR="$STAGE" PREFIX="$PREFIX"
test ! -e "$STAGE$PREFIX/bin/tm"
test ! -e "$STAGE$PREFIX/share/topology-machine"
rm -rf "$STAGE"

TM="$TM" OUT="$ROOT/build/demo-out" bash examples/demo.sh >/tmp/tm-demo.out
grep -q "PASS: all native checks" /tmp/tm-demo.out
echo "integration tests passed"
