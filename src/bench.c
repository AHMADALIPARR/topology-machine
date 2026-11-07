/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * Wall-clock samples of library calls. The compiler may not inline
 * across this translation unit without LTO, which the default build
 * does not enable. Figures are measurements of this process, not a
 * hardware claim.
 */
#include "tm.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void row(FILE *out, const char *name, uint64_t ops, double seconds)
{
    double ns = seconds <= 0.0 ? 0.0 : (seconds * 1e9) / (double)ops;
    fprintf(out, "%s %llu %.9f %.3f\n", name,
            (unsigned long long)ops, seconds, ns);
}

int tm_bench_run(FILE *out)
{
    volatile uint64_t sink = 0;
    double t0, dt;
    uint64_t i;
    const uint64_t gf_ops = 65536ull * 32ull;
    const uint64_t dag_ops = 200000ull;
    const uint64_t rev_ops = 200000ull * 3ull;
    const uint64_t analog_ops = 50000ull;
    tm_net *net = NULL;
    tm_dag *dag;
    int node = 0;
    tm_expr *e = NULL;
    char err[64];

    if (!out)
        return TM_ERR_RANGE;
    fprintf(out, "name ops seconds ns_per_op\n");

    t0 = now_s();
    for (i = 0; i < gf_ops; i++) {
        uint8_t a = (uint8_t)(i & 255u);
        uint8_t b = (uint8_t)((i >> 8) & 255u);
        sink += tm_gf_mul(a, b);
    }
    dt = now_s() - t0;
    row(out, "gf_mul", gf_ops, dt);

    dag = tm_dag_new();
    if (!dag || tm_expr_parse("xor(and(i0,i1),not(i0))", &e, err, sizeof err) != TM_OK ||
        tm_expr_to_dag(dag, e, &node) != TM_OK) {
        tm_expr_free(e);
        tm_dag_free(dag);
        return TM_ERR_DOMAIN;
    }
    t0 = now_s();
    for (i = 0; i < dag_ops; i++) {
        int bit = 0;
        tm_dag_eval(dag, node, i & 3u, &bit);
        sink += (uint64_t)bit;
    }
    dt = now_s() - t0;
    row(out, "dag_eval", dag_ops, dt);
    tm_expr_free(e);
    tm_dag_free(dag);

    if (tm_net_gf256_mul(&net) != TM_OK)
        return TM_ERR_DOMAIN;
    t0 = now_s();
    for (i = 0; i < 65536ull; i++) {
        uint64_t y = 0;
        tm_net_eval(net, i, &y);
        sink += y;
    }
    dt = now_s() - t0;
    row(out, "net_eval_65536", 65536ull, dt);
    tm_net_free(net);

    {
        tm_instr prog[3] = {
            {TM_CX, 0, 1, 0},
            {TM_CCX, 0, 1, 2},
            {TM_CSWAP, 0, 1, 2}
        };
        t0 = now_s();
        for (i = 0; i < 200000ull; i++) {
            unsigned s = (unsigned)(i & 7u);
            tm_rev_eval(s, prog, 3, 3, &s);
            sink += s;
        }
        dt = now_s() - t0;
        row(out, "rev_apply", rev_ops, dt);
    }

    {
        double G[4] = {3, -1, -1, 2};
        double I[2] = {1, 0};
        t0 = now_s();
        for (i = 0; i < analog_ops; i++) {
            double V[2], residual = 0;
            tm_analog_solve(G, I, 2, V, &residual);
            sink += (uint64_t)(V[0] * 1000.0);
        }
        dt = now_s() - t0;
        row(out, "analog_solve_2x2", analog_ops, dt);
    }

    fprintf(out, "sink %llu\n", (unsigned long long)sink);
    if (fflush(out) != 0)
        return TM_ERR_IO;
    return TM_OK;
}
