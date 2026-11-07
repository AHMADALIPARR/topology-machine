/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * Dense real linear solve, G V = I, by Gauss-Jordan with partial pivoting.
 * A passive resistor network is one use. The solver does not check
 * passivity, and a pivot below TM_ANALOG_PIVOT is reported as singular.
 * Settling time and precision limits are not modelled.
 */
#include "tm.h"

#include <math.h>
#include <string.h>

int tm_analog_solve(const double *G, const double *I, int n, double *V,
                    double *residual)
{
    double a[TM_ANALOG_MAX][TM_ANALOG_MAX + 1];
    int col, r, c;
    double res;

    if (!G || !I || !V || !residual || n < 1 || n > TM_ANALOG_MAX)
        return TM_ERR_RANGE;
    for (r = 0; r < n; r++) {
        for (c = 0; c < n; c++) {
            double g = G[(size_t)r * (size_t)n + (size_t)c];
            if (!isfinite(g))
                return TM_ERR_DOMAIN;
            a[r][c] = g;
        }
        if (!isfinite(I[r]))
            return TM_ERR_DOMAIN;
        a[r][n] = I[r];
    }
    for (col = 0; col < n; col++) {
        int piv = col;
        double scale;
        for (r = col + 1; r < n; r++) {
            if (fabs(a[r][col]) > fabs(a[piv][col]))
                piv = r;
        }
        if (fabs(a[piv][col]) < TM_ANALOG_PIVOT)
            return TM_ERR_SINGULAR;
        if (piv != col) {
            for (c = col; c <= n; c++) {
                double tmp = a[col][c];
                a[col][c] = a[piv][c];
                a[piv][c] = tmp;
            }
        }
        scale = a[col][col];
        for (c = col; c <= n; c++)
            a[col][c] /= scale;
        for (r = 0; r < n; r++) {
            double f;
            if (r == col)
                continue;
            f = a[r][col];
            for (c = col; c <= n; c++)
                a[r][c] -= f * a[col][c];
        }
    }
    for (r = 0; r < n; r++)
        V[r] = a[r][n];
    res = 0.0;
    for (r = 0; r < n; r++) {
        double acc = 0.0;
        double diff;
        for (c = 0; c < n; c++)
            acc += G[(size_t)r * (size_t)n + (size_t)c] * V[c];
        diff = fabs(acc - I[r]);
        if (diff > res)
            res = diff;
    }
    *residual = res;
    return TM_OK;
}
