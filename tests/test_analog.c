/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <math.h>
#include <stdio.h>

int main(void)
{
    double G[16 * 16], I[8], V[8], residual;
    int n, trial, r, c;
    G[0] = 3;
    G[1] = -1;
    G[2] = -1;
    G[3] = 2;
    I[0] = 1;
    I[1] = 0;
    CHECK(tm_analog_solve(G, I, 2, V, &residual) == TM_OK);
    CHECK(fabs(V[0] - 0.4) < 1e-12);
    CHECK(fabs(V[1] - 0.2) < 1e-12);
    CHECK(residual < 1e-9);
    G[0] = 2;
    G[1] = -1;
    G[2] = 0;
    G[3] = -1;
    G[4] = 2;
    G[5] = -1;
    G[6] = 0;
    G[7] = -1;
    G[8] = 2;
    I[0] = 1;
    I[1] = 0;
    I[2] = 0;
    CHECK(tm_analog_solve(G, I, 3, V, &residual) == TM_OK);
    CHECK(fabs(V[0] - 0.75) < 1e-9);
    CHECK(fabs(V[1] - 0.5) < 1e-9);
    CHECK(fabs(V[2] - 0.25) < 1e-9);
    CHECK(residual < 1e-9);
    G[0] = 1;
    I[0] = 4;
    CHECK(tm_analog_solve(G, I, 1, V, &residual) == TM_OK);
    CHECK(fabs(V[0] - 4.0) < 1e-12);
    G[0] = 1;
    G[1] = 2;
    G[2] = 3;
    G[3] = 4;
    I[0] = 5;
    I[1] = 6;
    CHECK(tm_analog_solve(G, I, 2, V, &residual) == TM_OK);
    CHECK(fabs(V[0] - (-4.0)) < 1e-9);
    CHECK(fabs(V[1] - 4.5) < 1e-9);
    G[0] = 1;
    G[1] = 1;
    G[2] = 1;
    G[3] = 1;
    CHECK(tm_analog_solve(G, I, 2, V, &residual) == TM_ERR_SINGULAR);
    G[0] = 0;
    CHECK(tm_analog_solve(G, I, 1, V, &residual) == TM_ERR_SINGULAR);
    G[0] = NAN;
    CHECK(tm_analog_solve(G, I, 1, V, &residual) == TM_ERR_DOMAIN);
    CHECK(tm_analog_solve(G, I, 0, V, &residual) == TM_ERR_RANGE);
    for (trial = 0; trial < 20; trial++) {
        n = 1 + (trial % 4);
        for (r = 0; r < n; r++) {
            double off = 0;
            for (c = 0; c < n; c++) {
                double v = 0;
                if (r != c) {
                    v = (double)(((r * 3 + c + trial) % 5) - 2);
                    off += fabs(v);
                }
                G[r * n + c] = v;
            }
            G[r * n + r] = off + 10.0 + (double)n;
            I[r] = (double)((trial + r) % 7) - 3.0;
        }
        CHECK(tm_analog_solve(G, I, n, V, &residual) == TM_OK);
        CHECK(residual < 1e-8);
    }
    return check_report("analog");
}
