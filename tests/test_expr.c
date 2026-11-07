/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t rng_state = 0xC0FFEEu;

static uint32_t rnd(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void gen(char *dst, size_t cap, int depth)
{
    int kind = (int)(rnd() % (depth <= 0 ? 3u : 7u));
    if (kind == 0)
        snprintf(dst, cap, "0");
    else if (kind == 1)
        snprintf(dst, cap, "1");
    else if (kind == 2)
        snprintf(dst, cap, "i%u", rnd() % 3u);
    else if (kind == 3) {
        char inner[256];
        gen(inner, sizeof inner, depth - 1);
        snprintf(dst, cap, "not(%s)", inner);
    } else {
        char l[256], r[256];
        const char *op = kind == 4 ? "and" : kind == 5 ? "or" : "xor";
        gen(l, sizeof l, depth - 1);
        gen(r, sizeof r, depth - 1);
        snprintf(dst, cap, "%s(%s,%s)", op, l, r);
    }
}

static void expect_parse_fail(const char *s)
{
    tm_expr *e = NULL;
    char err[128];
    int rc = tm_expr_parse(s, &e, err, sizeof err);
    CHECK(rc == TM_ERR_PARSE);
    CHECK(e == NULL);
    CHECK(strstr(err, "column") != NULL);
}

int main(void)
{
    static const char *bad[] = {
        "", " ", "and(i0)", "xor(i0,i1,i2)", "i", "i64", "i999",
        "AND(i0,i1)", "not i0", "xor(i0,i1) junk", "(", "not(i0",
        "or(i0,)", "i-1", "2", "xor()", NULL
    };
    int i;
    char err[128];
    tm_expr *e = NULL;
    tm_dag *d;
    int node = 0;
    unsigned x;
    CHECK(tm_expr_parse("xor(and(i0,i1),not(i0))", &e, err, sizeof err) == TM_OK);
    CHECK(tm_expr_inputs(e) == 2);
    d = tm_dag_new();
    CHECK(tm_expr_to_dag(d, e, &node) == TM_OK);
    for (x = 0; x < 4; x++) {
        int tree = -1, dag = -1;
        int expect = (int)((((x & 1u) & ((x >> 1) & 1u)) ^ ((x & 1u) ? 0u : 1u)));
        CHECK(tm_expr_eval(e, x, &tree) == TM_OK);
        CHECK(tm_dag_eval(d, node, x, &dag) == TM_OK);
        CHECK(tree == expect && dag == expect);
    }
    tm_expr_free(e);
    tm_dag_free(d);
    for (i = 0; bad[i]; i++)
        expect_parse_fail(bad[i]);
    for (i = 0; i < 200; i++) {
        char buf[512];
        uint32_t junk = rnd();
        int n = (int)(junk % 40u);
        int j;
        tm_expr *got = NULL;
        for (j = 0; j < n; j++)
            buf[j] = (char)(32 + (rnd() % 95u));
        buf[n] = '\0';
        if (tm_expr_parse(buf, &got, err, sizeof err) == TM_OK)
            tm_expr_free(got);
        else
            CHECK(got == NULL);
    }
    for (i = 0; i < 100; i++) {
        char buf[512];
        tm_expr *tree = NULL;
        tm_dag *dag = tm_dag_new();
        int root = 0;
        gen(buf, sizeof buf, 4);
        CHECK(tm_expr_parse(buf, &tree, err, sizeof err) == TM_OK);
        CHECK(tm_expr_to_dag(dag, tree, &root) == TM_OK);
        for (x = 0; x < 8; x++) {
            int a = -1, b = -1;
            CHECK(tm_expr_eval(tree, x, &a) == TM_OK);
            CHECK(tm_dag_eval(dag, root, x, &b) == TM_OK);
            CHECK(a == b);
        }
        tm_expr_free(tree);
        tm_dag_free(dag);
    }
    return check_report("expr");
}
