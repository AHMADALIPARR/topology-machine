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

/* Copy text into dst. Return -1 instead of truncating: a short buffer must
 * fail the test, and snprintf of two 255-byte pieces into a 512-byte buffer
 * is a -Wformat-truncation error under -Werror on GCC. */
static int append_text(char *dst, size_t cap, size_t *used, const char *text)
{
    size_t n = strlen(text);
    if (*used >= cap || n >= cap - *used)
        return -1;
    memcpy(dst + *used, text, n);
    *used += n;
    dst[*used] = '\0';
    return 0;
}

static int gen(char *dst, size_t cap, int depth)
{
    int kind = (int)(rnd() % (depth <= 0 ? 3u : 7u));
    size_t used = 0;

    if (cap == 0)
        return -1;
    dst[0] = '\0';
    if (kind == 0)
        return append_text(dst, cap, &used, "0");
    if (kind == 1)
        return append_text(dst, cap, &used, "1");
    if (kind == 2) {
        char leaf[16];
        int n = snprintf(leaf, sizeof leaf, "i%u", rnd() % 3u);
        if (n < 0 || (size_t)n >= sizeof leaf)
            return -1;
        return append_text(dst, cap, &used, leaf);
    }
    if (kind == 3) {
        char inner[256];
        if (gen(inner, sizeof inner, depth - 1) != 0)
            return -1;
        if (append_text(dst, cap, &used, "not(") != 0 ||
            append_text(dst, cap, &used, inner) != 0)
            return -1;
        return append_text(dst, cap, &used, ")");
    }
    {
        char left[256], right[256];
        const char *op = kind == 4 ? "and" : kind == 5 ? "or" : "xor";
        if (gen(left, sizeof left, depth - 1) != 0 ||
            gen(right, sizeof right, depth - 1) != 0)
            return -1;
        if (append_text(dst, cap, &used, op) != 0 ||
            append_text(dst, cap, &used, "(") != 0 ||
            append_text(dst, cap, &used, left) != 0 ||
            append_text(dst, cap, &used, ",") != 0 ||
            append_text(dst, cap, &used, right) != 0)
            return -1;
        return append_text(dst, cap, &used, ")");
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
        CHECK(gen(buf, sizeof buf, 4) == 0);
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
