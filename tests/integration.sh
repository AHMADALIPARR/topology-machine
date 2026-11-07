#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$ROOT"
TM=${TM:-./build/tm}
KG=${KG:-./third_party/klong/kg}

for s in scripts/bootstrap.sh scripts/restore-upstream-bytes.sh scripts/install.sh \
    scripts/uninstall.sh scripts/ci.sh scripts/fetch-klong.sh scripts/regen_assets.py \
    examples/demo.sh tests/cli.sh tests/integration.sh packaging/tm-kg.in; do
    test -x "$s"
done
./scripts/restore-upstream-bytes.sh

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
DESTDIR="$STAGE" PREFIX="$PREFIX" ./scripts/install.sh >/tmp/tm-install.out
test -x "$STAGE$PREFIX/bin/tm"
test -x "$STAGE$PREFIX/bin/tm-kg"
test -x "$STAGE$PREFIX/share/topology-machine/examples/demo.sh"
test -f "$STAGE$PREFIX/include/tm.h"
test -f "$STAGE$PREFIX/lib/libtm.a"
test -f "$STAGE$PREFIX/share/man/man1/tm.1"
cmp -s third_party/klong/lib/nstat.kg "$STAGE$PREFIX/share/topology-machine/klong-lib/nstat.kg"
"$STAGE$PREFIX/bin/tm" version | grep -qx "tm 1.0.0"
"$STAGE$PREFIX/bin/tm-kg" -n -q "$ROOT/klong/oracle.kg" > "$STAGE/oracle.txt"
"$STAGE$PREFIX/bin/tm" oracle > "$STAGE/native.txt"
diff -u "$STAGE/native.txt" "$STAGE/oracle.txt"
DESTDIR="$STAGE" PREFIX="$PREFIX" ./scripts/uninstall.sh >/tmp/tm-uninstall.out
test ! -e "$STAGE$PREFIX/bin/tm"
test ! -e "$STAGE$PREFIX/share/topology-machine"
rm -rf "$STAGE"

TM="$TM" OUT="$ROOT/build/demo-out" ./examples/demo.sh >/tmp/tm-demo.out
grep -q "PASS: all native checks" /tmp/tm-demo.out
echo "integration tests passed"
