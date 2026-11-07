/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    unsigned s = 0;
    tm_instr cx = {TM_CX, 0, 1, 0};
    tm_instr ccx = {TM_CCX, 0, 1, 2};
    tm_instr fw = {TM_CSWAP, 0, 1, 2};
    const char *text = tm_oracle_text();
    CHECK(strstr(text, "dot 13\n") != NULL);
    CHECK(strstr(text, "scan 1 1 2 3\n") != NULL);
    CHECK(strstr(text, "gf 87 131 193\n") != NULL);
    CHECK(tm_gf_mul(87, 131) == 193);
    CHECK(tm_gf_mul(83, 1) == 83);
    CHECK(tm_gf_mul(83, 0) == 0);
    CHECK(tm_gf_mul(255, 2) == 229);
    CHECK(tm_gf_mul(128, 2) == 27);
    CHECK(tm_rev_apply(5, cx, 3, &s) == TM_OK && s == 7);
    CHECK(tm_rev_apply(3, ccx, 3, &s) == TM_OK && s == 7);
    CHECK(tm_rev_apply(5, fw, 3, &s) == TM_OK && s == 3);
    CHECK(strstr(text, "rev cx 5 7\n") != NULL);
    CHECK(strstr(text, "rev cswap 5 3\n") != NULL);
    CHECK(strstr(text, "bool xor 1 1 0\n") != NULL);
    return check_report("oracle");
}
