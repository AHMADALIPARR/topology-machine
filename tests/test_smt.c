/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    FILE *fp = fopen("tests/golden/reversible.smt2", "rb");
    char buf[2048];
    size_t n;
    unsigned models = 9;
    unsigned s;
    tm_instr cx = {TM_CX, 0, 1, 0};
    tm_instr ccx = {TM_CCX, 0, 1, 2};
    tm_instr fw = {TM_CSWAP, 0, 1, 2};
    CHECK(fp != NULL);
    n = fread(buf, 1, sizeof buf - 1, fp);
    buf[n] = '\0';
    fclose(fp);
    CHECK(strcmp(buf, tm_smt_involution_text()) == 0);
    CHECK(tm_smt_involution_models(&models) == TM_OK);
    CHECK(models == 0);
    for (s = 0; s < 8; s++) {
        unsigned v = 0;
        CHECK(tm_rev_apply(s, cx, 3, &v) == TM_OK);
        CHECK(v == tm_smt_cx(s));
        CHECK(tm_rev_apply(s, ccx, 3, &v) == TM_OK);
        CHECK(v == tm_smt_ccx(s));
        CHECK(tm_rev_apply(s, fw, 3, &v) == TM_OK);
        CHECK(v == tm_smt_fw(s));
        CHECK(tm_smt_cx(tm_smt_cx(s)) == s);
        CHECK(tm_smt_ccx(tm_smt_ccx(s)) == s);
        CHECK(tm_smt_fw(tm_smt_fw(s)) == s);
    }
    return check_report("smt");
}
