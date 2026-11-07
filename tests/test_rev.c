/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <stdio.h>
#include <string.h>

static int is_perm_and_involution(tm_instr g, unsigned width)
{
    unsigned n = 1u << width;
    unsigned seen = 0;
    unsigned s;
    for (s = 0; s < n; s++) {
        unsigned v = 0, back = 99;
        if (tm_rev_apply(s, g, width, &v) != TM_OK)
            return 0;
        if (v >= n || (seen & (1u << v)))
            return 0;
        seen |= 1u << v;
        if (tm_rev_apply(v, g, width, &back) != TM_OK || back != s)
            return 0;
    }
    return seen == (n == 32 ? 0xffffffffu : (1u << n) - 1u);
}

int main(void)
{
    tm_instr g;
    unsigned s, w, a, b, c;
    char err[80];
    tm_instr prog[8];
    size_t n = 0;
    g.op = TM_CX;
    g.a = 0;
    g.b = 1;
    g.c = 0;
    CHECK(is_perm_and_involution(g, 3));
    g.op = TM_CCX;
    g.c = 2;
    CHECK(is_perm_and_involution(g, 3));
    g.op = TM_CSWAP;
    CHECK(is_perm_and_involution(g, 3));
    for (w = 1; w <= 4; w++) {
        for (a = 0; a < w; a++) {
            for (b = 0; b < w; b++) {
                if (a == b)
                    continue;
                g.op = TM_CX;
                g.a = a;
                g.b = b;
                g.c = 0;
                CHECK(is_perm_and_involution(g, w));
            }
        }
    }
    for (a = 0; a < 3; a++) {
        for (b = 0; b < 3; b++) {
            for (c = 0; c < 3; c++) {
                unsigned v = 0;
                if (a == b || a == c || b == c)
                    continue;
                g.op = TM_CCX;
                g.a = a;
                g.b = b;
                g.c = c;
                CHECK(is_perm_and_involution(g, 3));
                g.op = TM_CSWAP;
                CHECK(is_perm_and_involution(g, 3));
                g.op = TM_CX;
                CHECK(tm_rev_apply(1u << a, g, 3, &v) == TM_OK);
            }
        }
    }
    g.op = TM_CX;
    g.a = 0;
    g.b = 0;
    CHECK(tm_rev_apply(0, g, 3, &s) == TM_ERR_DOMAIN);
    g.b = 1;
    CHECK(tm_rev_apply(8, g, 3, &s) == TM_ERR_RANGE);
    CHECK(tm_rev_apply(0, g, 0, &s) == TM_ERR_RANGE);
    CHECK(tm_rev_parse("cx 0 1; ccx 0 1 2, cswap 0 1 2", prog, 8, &n, 3, err, sizeof err) == TM_OK);
    CHECK(n == 3);
    CHECK(tm_rev_eval(5, prog, n, 3, &s) == TM_OK);
    CHECK(tm_rev_parse("cx 0 0", prog, 8, &n, 3, err, sizeof err) == TM_ERR_DOMAIN);
    CHECK(tm_rev_parse("nope 0 1", prog, 8, &n, 3, err, sizeof err) == TM_ERR_PARSE);
    CHECK(tm_rev_parse("cx 0", prog, 8, &n, 3, err, sizeof err) == TM_ERR_PARSE);
    CHECK(tm_rev_parse("cx 1 99", prog, 8, &n, 3, err, sizeof err) == TM_ERR_PARSE);
    CHECK(tm_rev_parse("", prog, 8, &n, 3, err, sizeof err) == TM_OK && n == 0);
    {
        tm_instr seq[2] = {{TM_CX, 0, 1, 0}, {TM_CX, 1, 2, 0}};
        unsigned seen = 0;
        for (s = 0; s < 8; s++) {
            unsigned v = 0;
            CHECK(tm_rev_eval(s, seq, 2, 3, &v) == TM_OK);
            CHECK((seen & (1u << v)) == 0);
            seen |= 1u << v;
        }
        CHECK(seen == 255);
        CHECK(tm_rev_eval(1, seq, 2, 3, &s) == TM_OK);
        CHECK(s != 1);
    }
    return check_report("rev");
}
