#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors
# Re-download the pinned Klong tarball and check its SHA-256.
# Does not replace third_party/klong unless TM_KLONG_REPLACE=1.
set -eu
cd "$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
url=https://t3x.org/klong/klong20221212.tgz
expect=5e1a4877228a3c643a99dbfd2d73e60bc3fa856e2615ac2fe78c80370e7f96b4
tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT
curl -fsSL -o "$tmp" "$url"
got=$(sha256sum "$tmp" | awk '{print $1}')
if [ "$got" != "$expect" ]; then
    echo "fetch-klong: checksum mismatch" >&2
    echo " expected $expect" >&2
    echo " got      $got" >&2
    exit 1
fi
echo "fetch-klong: $expect"
if [ "${TM_KLONG_REPLACE:-0}" = 1 ]; then
    echo "fetch-klong: refusing automatic replacement; extract and review the diff by hand" >&2
    exit 2
fi
