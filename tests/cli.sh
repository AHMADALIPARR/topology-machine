#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors
set -euo pipefail
TM=${TM:-./build/tm}
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$ROOT"

fail() { echo "cli test failed: $*" >&2; exit 1; }

set +e
"$TM" >/tmp/tm-noargs.err 2>&1
code=$?
set -e
test "$code" -eq 2
grep -q "Usage:" /tmp/tm-noargs.err

"$TM" help | grep -q "Topology Machine"
"$TM" version | grep -qx "tm 1.0.0"
test "$("$TM" gf mul 0x57 0x83)" = "0xc1"
test "$("$TM" gf mul 87 131)" = "0xc1"
test "$("$TM" dag eval --expr 'xor(and(i0,i1),not(i0))' --input 0)" = "1"
test "$("$TM" dag eval --expr 'xor(i0,i0)' --input 1)" = "0"
test "$("$TM" rev apply --width 3 --state 5 --prog 'cx 0 1')" = "7"

set +e
"$TM" gf inv 0 >/tmp/tm-inv.err 2>&1
code=$?
set -e
test "$code" -eq 4

set +e
"$TM" dag eval --expr 'and(i0' --input 0 >/tmp/tm-parse.err 2>&1
code=$?
set -e
test "$code" -eq 2
grep -q "column" /tmp/tm-parse.err

set +e
"$TM" analog solve --g '1,1;1,1' --i '1,1' >/tmp/tm-sing.err 2>&1
code=$?
set -e
test "$code" -eq 4

set +e
"$TM" analog solve --g '1,2,3;4,5' --i '1,2' >/tmp/tm-rag.err 2>&1
code=$?
set -e
test "$code" -eq 2

set +e
"$TM" rev apply --width 3 --state 1 --prog 'cx 0 0' >/tmp/tm-wire.err 2>&1
code=$?
set -e
test "$code" -eq 4

set +e
"$TM" qasm sim /no/such/file.qasm >/tmp/tm-miss.err 2>&1
code=$?
set -e
test "$code" -eq 3

set +e
"$TM" nosuch >/tmp/tm-cmd.err 2>&1
code=$?
set -e
test "$code" -eq 2

"$TM" smt check | head -n 1 | grep -qx unsat
"$TM" dag truth --expr 'xor(and(i0,i1),not(i0))' --bits 2 | grep -qx '00 1'
echo "cli tests passed"
