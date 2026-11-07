/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <stdio.h>
#include <string.h>

static int demo_expect(unsigned x)
{
    return (int)((((x & 1u) & ((x >> 1) & 1u)) ^ ((x & 1u) ? 0u : 1u)));
}

int main(void)
{
    tm_dag *d = tm_dag_new();
    tm_op op;
    int a, b, i0, i1, shared, again, y, xx, n0;
    unsigned x;
    CHECK(d != NULL);
    CHECK(tm_dag_n(d) == 2);
    CHECK(tm_dag_op(d, 0, &op, &a, &b) == TM_OK && op == TM_OP_ZERO);
    CHECK(tm_dag_op(d, 1, &op, &a, &b) == TM_OK && op == TM_OP_ONE);
    i0 = tm_dag_input(d, 0);
    i1 = tm_dag_input(d, 1);
    shared = tm_dag_and(d, i0, i1);
    again = tm_dag_and(d, i1, i0);
    CHECK(shared == again);
    y = tm_dag_xor(d, shared, tm_dag_not(d, i0));
    for (x = 0; x < 4; x++) {
        int bit = -1;
        CHECK(tm_dag_eval(d, y, x, &bit) == TM_OK);
        CHECK(bit == demo_expect(x));
    }
    xx = tm_dag_xor(d, i0, i0);
    CHECK(xx == 0);
    CHECK(tm_dag_op(d, xx, &op, &a, &b) == TM_OK && op == TM_OP_ZERO);
    CHECK(tm_dag_and(d, i0, i0) == i0);
    CHECK(tm_dag_or(d, i0, i0) == i0);
    CHECK(tm_dag_not(d, tm_dag_not(d, i0)) == i0);
    CHECK(tm_dag_not(d, 0) == 1);
    CHECK(tm_dag_not(d, 1) == 0);
    CHECK(tm_dag_and(d, i0, 0) == 0);
    CHECK(tm_dag_and(d, 1, i1) == i1);
    CHECK(tm_dag_or(d, i0, 0) == i0);
    CHECK(tm_dag_or(d, i0, 1) == 1);
    CHECK(tm_dag_xor(d, i0, 0) == i0);
    n0 = tm_dag_xor(d, i0, 1);
    CHECK(n0 == tm_dag_not(d, i0));
    CHECK(tm_dag_input(d, 64) == TM_ERR_RANGE);
    CHECK(tm_dag_input(d, -1) == TM_ERR_RANGE);
    CHECK(tm_dag_and(d, 99, i0) == TM_ERR_RANGE);
    CHECK(tm_dag_eval(d, 1000, 0, &a) == TM_ERR_RANGE);
    {
        int prev = i0;
        int hit = 0;
        int i;
        for (i = 0; i < 20000; i++) {
            int in = tm_dag_input(d, i % 64);
            int left = tm_dag_and(d, prev, in);
            int right = tm_dag_and(d, prev, tm_dag_not(d, in));
            int nxt;
            if (left < 0 || right < 0) {
                hit = 1;
                break;
            }
            nxt = tm_dag_or(d, left, right);
            if (nxt == TM_ERR_CAPACITY) {
                hit = 1;
                break;
            }
            CHECK(nxt >= 0);
            prev = nxt;
        }
        CHECK(hit == 1);
        CHECK(tm_dag_n(d) == TM_DAG_MAX_NODES);
        {
            int bit = 0;
            CHECK(tm_dag_eval(d, i0, 1, &bit) == TM_OK);
            CHECK(bit == 1);
        }
    }
    tm_dag_free(d);
    tm_dag_free(NULL);
    return check_report("dag");
}
