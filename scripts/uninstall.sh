#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors
set -eu
cd "$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
make uninstall PREFIX="${PREFIX:-/usr/local}" DESTDIR="${DESTDIR:-}"
