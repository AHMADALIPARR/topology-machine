/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * The QF_BV query asks whether any 3-bit state breaks the involution
 * identity of the three starter formulas. Zero models means that query
 * is unsatisfiable. It does not certify the C sources or OpenQASM.
 * Names a, b, and c in the starter query are unused; they are kept so
 * the text matches the executable starter byte for byte.
 */
#include "tm.h"

#include <stdio.h>
#include <string.h>

static const char SMT[] =
    "(set-logic QF_BV)\n"
    "(declare-fun s () (_ BitVec 3))\n"
    "(define-fun a () (_ BitVec 1) ((_ extract 0 0) s))\n"
    "(define-fun b () (_ BitVec 1) ((_ extract 1 1) s))\n"
    "(define-fun c () (_ BitVec 1) ((_ extract 2 2) s))\n"
    "(define-fun cx ((x (_ BitVec 3))) (_ BitVec 3)\n"
    " (bvxor x (bvshl (bvand x #b001) #b001)))\n"
    "(define-fun ccx ((x (_ BitVec 3))) (_ BitVec 3)\n"
    " (bvxor x (bvshl (bvand (bvand x #b001) (bvlshr (bvand x #b010) #b001)) #b010)))\n"
    "(define-fun fw ((x (_ BitVec 3))) (_ BitVec 3)\n"
    " (ite (= (bvand x #b001) #b001)\n"
    "   (bvor (bvand x #b001) (bvor (bvshl (bvand x #b010) #b001) (bvlshr (bvand x #b100) #b001))) x))\n"
    "(assert (or (distinct (cx (cx s)) s) (distinct (ccx (ccx s)) s) (distinct (fw (fw s)) s)))\n"
    "(check-sat)\n";

const char *tm_smt_involution_text(void)
{
    return SMT;
}

int tm_smt_write_involution(FILE *fp)
{
    if (!fp)
        return TM_ERR_IO;
    if (fputs(SMT, fp) < 0)
        return TM_ERR_IO;
    return TM_OK;
}

unsigned tm_smt_cx(unsigned x)
{
    x &= 7u;
    return (x ^ ((x & 1u) << 1)) & 7u;
}

unsigned tm_smt_ccx(unsigned x)
{
    unsigned both;
    x &= 7u;
    both = (x & 1u) & ((x & 2u) >> 1);
    return (x ^ (both << 2)) & 7u;
}

unsigned tm_smt_fw(unsigned x)
{
    x &= 7u;
    if ((x & 1u) == 1u) {
        unsigned bit0 = x & 1u;
        unsigned bit1_to_2 = (x & 2u) << 1;
        unsigned bit2_to_1 = (x & 4u) >> 1;
        return bit0 | bit1_to_2 | bit2_to_1;
    }
    return x;
}

int tm_smt_involution_models(unsigned *nmodels)
{
    unsigned s, n = 0;
    if (!nmodels)
        return TM_ERR_RANGE;
    for (s = 0; s < 8; s++) {
        int bad = 0;
        if (tm_smt_cx(tm_smt_cx(s)) != s)
            bad = 1;
        if (tm_smt_ccx(tm_smt_ccx(s)) != s)
            bad = 1;
        if (tm_smt_fw(tm_smt_fw(s)) != s)
            bad = 1;
        if (bad)
            n++;
    }
    *nmodels = n;
    return TM_OK;
}
