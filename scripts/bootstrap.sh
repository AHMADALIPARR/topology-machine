#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors
set -eu
cd "$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
need() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "bootstrap: missing $1" >&2
        exit 1
    }
}
need cc
need make
need diff
if ! cc -std=c11 -x c -c /dev/null -o /dev/null 2>/dev/null; then
    echo "bootstrap: cc does not accept -std=c11" >&2
    exit 1
fi
sh scripts/restore-upstream-bytes.sh
make all
echo "bootstrap: built build/tm and third_party/klong/kg"
echo "bootstrap: next: make test"
