#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TM=${TM:-"$ROOT/build/tm"}
OUT=${OUT:-"$ROOT/out"}
"$TM" self-check --out-dir "$OUT"
"$TM" dag truth --expr 'xor(and(i0,i1),not(i0))' --bits 2
"$TM" gf mul 0x57 0x83
"$TM" rev apply --width 3 --state 5 --prog 'cx 0 1'
"$TM" analog solve --g '3,-1;-1,2' --i '1,0'
"$TM" qasm sim "$OUT/bell.qasm"
"$TM" smt check
