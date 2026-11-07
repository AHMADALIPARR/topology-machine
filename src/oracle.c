/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * Lines compared with klong/oracle.kg under the real Klong interpreter.
 * Decimal on purpose: Klong's integer printer and this text must match.
 */
#include "tm.h"

#include <stdio.h>
#include <string.h>

static const char ORACLE[] =
    "dot 13\n"
    "scan 1 1 2 3\n"
    "gf 87 131 193\n"
    "gf 83 1 83\n"
    "gf 83 0 0\n"
    "gf 255 2 229\n"
    "gf 128 2 27\n"
    "gf 1 1 1\n"
    "gf 2 2 4\n"
    "rev cx 5 7\n"
    "rev ccx 3 7\n"
    "rev cswap 5 3\n"
    "bool and 0 1 0\n"
    "bool or 0 1 1\n"
    "bool xor 1 1 0\n"
    "bool not 0 1\n";

const char *tm_oracle_text(void)
{
    return ORACLE;
}

int tm_oracle_write(FILE *fp)
{
    if (!fp)
        return TM_ERR_IO;
    if (fputs(ORACLE, fp) < 0)
        return TM_ERR_IO;
    return TM_OK;
}
