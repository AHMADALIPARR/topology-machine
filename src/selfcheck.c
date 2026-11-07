/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "tm.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int fail(FILE *out, const char *msg)
{
    fprintf(out, "FAIL: %s\n", msg);
    return TM_ERR_DOMAIN;
}

int tm_selfcheck(FILE *out, const char *outdir)
{
    tm_dag *d;
    tm_expr *e = NULL;
    tm_net *net = NULL;
    tm_net *reread = NULL;
    char err[128];
    char path[512];
    int node = 0, rc;
    unsigned x, models = 1;
    int a;
    tm_amp psi[4];
    double G[4], I[2], V[2], residual = 1;
    const char *expr = "xor(and(i0,i1),not(i0))";

    if (!out)
        return TM_ERR_RANGE;

    d = tm_dag_new();
    if (!d)
        return fail(out, "dag allocation");
    rc = tm_expr_parse(expr, &e, err, sizeof err);
    if (rc != TM_OK) {
        tm_dag_free(d);
        return fail(out, "demo expression");
    }
    rc = tm_expr_to_dag(d, e, &node);
    if (rc != TM_OK) {
        tm_expr_free(e);
        tm_dag_free(d);
        return fail(out, "demo dag");
    }
    for (x = 0; x < 4; x++) {
        int tree = 0, dag = 0;
        int expect = (int)((((x & 1u) & ((x >> 1) & 1u)) ^ ((x & 1u) ? 0u : 1u)));
        if (tm_expr_eval(e, x, &tree) != TM_OK || tm_dag_eval(d, node, x, &dag) != TM_OK ||
            tree != expect || dag != expect) {
            tm_expr_free(e);
            tm_dag_free(d);
            return fail(out, "dag demo mismatch");
        }
    }
    {
        int i0 = tm_dag_input(d, 0);
        int i1 = tm_dag_input(d, 1);
        int s1 = tm_dag_and(d, i0, i1);
        int s2 = tm_dag_and(d, i1, i0);
        int xx = tm_dag_xor(d, i0, i0);
        if (s1 < 0 || s1 != s2 || xx != 0) {
            tm_expr_free(e);
            tm_dag_free(d);
            return fail(out, "canonical sharing");
        }
    }
    fprintf(out, "DAG: canonical sharing verified; nodes=%d; %s all 4 inputs passed\n",
            tm_dag_n(d), expr);
    tm_expr_free(e);
    tm_dag_free(d);

    if (tm_gf_mul(0x57, 0x83) != 0xc1 || tm_gf_mul_rec(0x57, 0x83) != 0xc1)
        return fail(out, "aes vector");
    for (a = 0; a < 256; a++) {
        int b;
        uint8_t ua = (uint8_t)a;
        if (tm_gf_mul(ua, 0) != 0 || tm_gf_mul(ua, 1) != ua)
            return fail(out, "gf identity");
        if (tm_gf_mul(ua, ua) != tm_gf_mul_rec(ua, ua))
            return fail(out, "gf recursive mismatch");
        for (b = 0; b < 256; b++) {
            uint8_t ub = (uint8_t)b;
            if (tm_gf_mul(ua, ub) != tm_gf_mul(ub, ua))
                return fail(out, "gf commutativity");
            if (tm_gf_mul(ua, ub) != tm_gf_mul_rec(ua, ub))
                return fail(out, "gf recursive mismatch");
        }
        if (ua == 0)
            continue;
        {
            uint8_t inv = 0;
            if (tm_gf_inv(ua, &inv) != TM_OK || tm_gf_mul(ua, inv) != 1)
                return fail(out, "gf inverse");
        }
    }
    {
        uint8_t inv = 1;
        if (tm_gf_inv(0, &inv) != TM_ERR_DOMAIN)
            return fail(out, "gf zero inverse");
    }
    fprintf(out, "GF(2^8): 0x57*0x83=0xc1; identity; commutativity 65536; "
                 "iterative=recursive; inverses 255\n");

    {
        tm_instr gates[3] = {
            {TM_CX, 0, 1, 0},
            {TM_CCX, 0, 1, 2},
            {TM_CSWAP, 0, 1, 2}
        };
        unsigned g;
        for (g = 0; g < 3; g++) {
            unsigned seen = 0, s;
            for (s = 0; s < 8; s++) {
                unsigned v = 0, back = 0;
                if (tm_rev_apply(s, gates[g], 3, &v) != TM_OK || v > 7)
                    return fail(out, "reversible apply");
                if (seen & (1u << v))
                    return fail(out, "reversible not a permutation");
                seen |= 1u << v;
                if (tm_rev_apply(v, gates[g], 3, &back) != TM_OK || back != s)
                    return fail(out, "reversible not an involution");
                if (g == 0 && tm_smt_cx(s) != v)
                    return fail(out, "smt cx differs from native");
                if (g == 1 && tm_smt_ccx(s) != v)
                    return fail(out, "smt ccx differs from native");
                if (g == 2 && tm_smt_fw(s) != v)
                    return fail(out, "smt fredkin differs from native");
            }
            if (seen != 255)
                return fail(out, "reversible image");
        }
    }
    fprintf(out, "REVERSIBLE: CX, Toffoli, Fredkin are 3-bit permutations and involutions\n");

    if (tm_q_bell(psi) != TM_OK)
        return fail(out, "bell");
    {
        double norm = 0;
        int j;
        for (j = 0; j < 4; j++)
            norm += psi[j].re * psi[j].re + psi[j].im * psi[j].im;
        if (fabs(norm - 1.0) > 1e-12)
            return fail(out, "bell norm");
        if (fabs(psi[0].re - 1.0 / sqrt(2.0)) > 1e-12 || fabs(psi[0].im) > 1e-12)
            return fail(out, "bell |00>");
        if (fabs(psi[3].re - 1.0 / sqrt(2.0)) > 1e-12 || fabs(psi[3].im) > 1e-12)
            return fail(out, "bell |11>");
        if (fabs(psi[1].re) > 1e-12 || fabs(psi[1].im) > 1e-12 ||
            fabs(psi[2].re) > 1e-12 || fabs(psi[2].im) > 1e-12)
            return fail(out, "bell leakage");
    }
    fprintf(out, "QUANTUM: Bell (|00>+|11>)/sqrt(2); probabilities 0.5/0.5; norm 1\n");

    G[0] = 3;
    G[1] = -1;
    G[2] = -1;
    G[3] = 2;
    I[0] = 1;
    I[1] = 0;
    if (tm_analog_solve(G, I, 2, V, &residual) != TM_OK)
        return fail(out, "analog solve");
    if (fabs(V[0] - 0.4) > 1e-12 || fabs(V[1] - 0.2) > 1e-12 || residual > 1e-12)
        return fail(out, "analog reference");
    fprintf(out, "ANALOG: resistor-network nodal solution V=(%.3f,%.3f) residual=%.3e\n",
            V[0], V[1], residual);

    if (tm_smt_involution_models(&models) != TM_OK || models != 0)
        return fail(out, "smt models");
    fprintf(out, "SMT: involution counterexample query has 0 models over 8 states\n");

    if (tm_net_gf256_mul(&net) != TM_OK)
        return fail(out, "circuit build");
    for (a = 0; a < 256; a++) {
        int b;
        for (b = 0; b < 256; b++) {
            uint64_t in = (uint64_t)(a & 255) | ((uint64_t)(b & 255) << 8);
            uint64_t y = 0;
            if (tm_net_eval(net, in, &y) != TM_OK || (y & 255u) != tm_gf_mul((uint8_t)a, (uint8_t)b)) {
                tm_net_free(net);
                return fail(out, "circuit disagrees with gf_mul");
            }
        }
    }
    fprintf(out, "CIRCUIT: gf256 multiplier gates=%d inputs=%d outputs=%d; exhaustive 65536 match\n",
            tm_net_ngates(net), tm_net_ninputs(net), tm_net_noutputs(net));

    if (outdir && outdir[0]) {
        if (tm_mkdir(outdir) != TM_OK) {
            tm_net_free(net);
            return fail(out, "cannot create output directory");
        }
        snprintf(path, sizeof path, "%s/bell.qasm", outdir);
        if (tm_write_text(path, tm_qasm_bell_text()) != TM_OK) {
            tm_net_free(net);
            return fail(out, "write bell.qasm");
        }
        snprintf(path, sizeof path, "%s/reversible.qasm", outdir);
        if (tm_write_text(path, tm_qasm_reversible_text()) != TM_OK) {
            tm_net_free(net);
            return fail(out, "write reversible.qasm");
        }
        snprintf(path, sizeof path, "%s/reversible.smt2", outdir);
        if (tm_write_text(path, tm_smt_involution_text()) != TM_OK) {
            tm_net_free(net);
            return fail(out, "write reversible.smt2");
        }
        snprintf(path, sizeof path, "%s/gf256.tmnl", outdir);
        if (tm_net_write_path(net, path) != TM_OK) {
            tm_net_free(net);
            return fail(out, "write netlist");
        }
        if (tm_net_read_path(path, &reread, err, sizeof err) != TM_OK) {
            tm_net_free(net);
            return fail(out, "reread netlist");
        }
        {
            uint64_t y1 = 0, y2 = 0;
            if (tm_net_eval(net, 0x8357ull, &y1) != TM_OK ||
                tm_net_eval(reread, 0x8357ull, &y2) != TM_OK || y1 != y2) {
                tm_net_free(reread);
                tm_net_free(net);
                return fail(out, "netlist roundtrip");
            }
        }
        tm_net_free(reread);
        snprintf(path, sizeof path, "%s/bell.qasm", outdir);
        {
            tm_amp sim[4];
            int nq = 0;
            if (tm_qasm_sim_path(path, sim, 4, &nq, err, sizeof err) != TM_OK || nq != 2) {
                tm_net_free(net);
                return fail(out, "qasm sim of emitted bell");
            }
            if (fabs(sim[0].re - psi[0].re) > 1e-12 || fabs(sim[3].re - psi[3].re) > 1e-12) {
                tm_net_free(net);
                return fail(out, "emitted bell disagrees with native amplitudes");
            }
        }
        fprintf(out, "GENERATED: %s/bell.qasm %s/reversible.qasm %s/reversible.smt2 %s/gf256.tmnl\n",
                outdir, outdir, outdir, outdir);
    }
    tm_net_free(net);
    fprintf(out, "PASS: all native checks\n");
    return TM_OK;
}
