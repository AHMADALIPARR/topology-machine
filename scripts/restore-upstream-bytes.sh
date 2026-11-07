#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors
# Decode two upstream Klong files that are not UTF-8.
# nstat.kg is Latin-1 text (0xD7 is a multiplication sign in a comment).
# klong.png is the upstream logo and is not required to build or run kg.
# The sidecars are standard base64 of the original bytes.
set -eu
cd "$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
restore() {
    b64=$1
    dest=$2
    expect=$3
    mkdir -p "$(dirname "$dest")"
    base64 -d "$b64" > "$dest"
    got=$(sha256sum "$dest" | awk '{print $1}')
    if [ "$got" != "$expect" ]; then
        echo "restore-upstream-bytes: checksum mismatch for $dest" >&2
        echo " expected $expect" >&2
        echo " got      $got" >&2
        exit 1
    fi
    echo "restored $dest"
}
restore third_party/klong/lib/nstat.kg.b64 third_party/klong/lib/nstat.kg \
    0b0c154bb81a5e4918c1797c931cab57541fdb91bd346857d408ac896c58cf48
restore third_party/klong/klong.png.b64 third_party/klong/klong.png \
    bb38ecb44ef35d2c13b240ffc6477591f3c3bbef08dec311f92089d134dee1ac
