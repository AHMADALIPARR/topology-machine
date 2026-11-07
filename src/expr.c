/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * Expression language:
 *   0 | 1 | i<bit> | not(e) | and(e,e) | or(e,e) | xor(e,e)
 * The tree evaluator does not fold. The DAG builder does.
 */
#include "tm.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct tm_expr {
    tm_op op;
    int input;
    tm_expr *a, *b;
};

typedef struct {
    const char *s;
    size_t i, n;
    char *err;
    size_t errlen;
} ps_t;

static void fail(ps_t *p, const char *msg)
{
    if (p->err && p->errlen)
        snprintf(p->err, p->errlen, "column %zu: %s", p->i + 1, msg);
}

static void skip(ps_t *p)
{
    while (p->i < p->n && isspace((unsigned char)p->s[p->i]))
        p->i++;
}

static int keyword(ps_t *p, const char *kw)
{
    size_t k = strlen(kw);
    unsigned char end;
    if (p->i + k > p->n || strncmp(p->s + p->i, kw, k) != 0)
        return 0;
    if (p->i + k == p->n)
        return 1;
    end = (unsigned char)p->s[p->i + k];
    if (isalnum(end) || end == '_')
        return 0;
    return 1;
}

static tm_expr *make(tm_op op, int input, tm_expr *a, tm_expr *b)
{
    tm_expr *e = calloc(1, sizeof *e);
    if (!e) {
        tm_expr_free(a);
        tm_expr_free(b);
        return NULL;
    }
    e->op = op;
    e->input = input;
    e->a = a;
    e->b = b;
    return e;
}

void tm_expr_free(tm_expr *e)
{
    if (!e)
        return;
    tm_expr_free(e->a);
    tm_expr_free(e->b);
    free(e);
}

static int parse_expr(ps_t *p, tm_expr **out);

static int expect(ps_t *p, char c, const char *msg)
{
    skip(p);
    if (p->i >= p->n || p->s[p->i] != c) {
        fail(p, msg);
        return -1;
    }
    p->i++;
    return 0;
}

static int parse_call1(ps_t *p, tm_expr **out)
{
    tm_expr *a = NULL;
    if (expect(p, '(', "expected '('") != 0)
        return -1;
    if (parse_expr(p, &a) != 0)
        return -1;
    if (expect(p, ')', "expected ')'") != 0) {
        tm_expr_free(a);
        return -1;
    }
    *out = make(TM_OP_NOT, 0, a, NULL);
    if (!*out) {
        fail(p, "out of memory");
        return -1;
    }
    return 0;
}

static int parse_call2(ps_t *p, tm_op op, tm_expr **out)
{
    tm_expr *a = NULL, *b = NULL;
    if (expect(p, '(', "expected '('") != 0)
        return -1;
    if (parse_expr(p, &a) != 0)
        return -1;
    if (expect(p, ',', "expected ','") != 0) {
        tm_expr_free(a);
        return -1;
    }
    if (parse_expr(p, &b) != 0) {
        tm_expr_free(a);
        return -1;
    }
    if (expect(p, ')', "expected ')'") != 0) {
        tm_expr_free(a);
        tm_expr_free(b);
        return -1;
    }
    *out = make(op, 0, a, b);
    if (!*out) {
        fail(p, "out of memory");
        return -1;
    }
    return 0;
}

static int parse_expr(ps_t *p, tm_expr **out)
{
    skip(p);
    if (p->i >= p->n) {
        fail(p, "unexpected end");
        return -1;
    }
    if (p->s[p->i] == '0' && (p->i + 1 == p->n || !isalnum((unsigned char)p->s[p->i + 1]))) {
        p->i++;
        *out = make(TM_OP_ZERO, 0, NULL, NULL);
        if (!*out) {
            fail(p, "out of memory");
            return -1;
        }
        return 0;
    }
    if (p->s[p->i] == '1' && (p->i + 1 == p->n || !isalnum((unsigned char)p->s[p->i + 1]))) {
        p->i++;
        *out = make(TM_OP_ONE, 0, NULL, NULL);
        if (!*out) {
            fail(p, "out of memory");
            return -1;
        }
        return 0;
    }
    if (keyword(p, "not")) {
        p->i += 3;
        return parse_call1(p, out);
    }
    if (keyword(p, "and")) {
        p->i += 3;
        return parse_call2(p, TM_OP_AND, out);
    }
    if (keyword(p, "or")) {
        p->i += 2;
        return parse_call2(p, TM_OP_OR, out);
    }
    if (keyword(p, "xor")) {
        p->i += 3;
        return parse_call2(p, TM_OP_XOR, out);
    }
    if (p->s[p->i] == 'i') {
        char *end = NULL;
        unsigned long v;
        p->i++;
        if (p->i >= p->n || !isdigit((unsigned char)p->s[p->i])) {
            fail(p, "expected input index");
            return -1;
        }
        v = strtoul(p->s + p->i, &end, 10);
        if (end == p->s + p->i) {
            fail(p, "expected input index");
            return -1;
        }
        p->i = (size_t)(end - p->s);
        if (v >= TM_DAG_MAX_INPUTS) {
            fail(p, "input index out of range");
            return -1;
        }
        *out = make(TM_OP_INPUT, (int)v, NULL, NULL);
        if (!*out) {
            fail(p, "out of memory");
            return -1;
        }
        return 0;
    }
    fail(p, "expected expression");
    return -1;
}

int tm_expr_parse(const char *s, tm_expr **out, char *err, size_t errlen)
{
    ps_t p;
    tm_expr *e = NULL;
    if (!s || !out)
        return TM_ERR_PARSE;
    if (err && errlen)
        err[0] = '\0';
    p.s = s;
    p.i = 0;
    p.n = strlen(s);
    p.err = err;
    p.errlen = errlen;
    if (parse_expr(&p, &e) != 0)
        return TM_ERR_PARSE;
    skip(&p);
    if (p.i != p.n) {
        tm_expr_free(e);
        fail(&p, "trailing junk");
        return TM_ERR_PARSE;
    }
    *out = e;
    return TM_OK;
}

int tm_expr_eval(const tm_expr *e, uint64_t input, int *bit)
{
    int x = 0, y = 0, rc;
    if (!e || !bit)
        return TM_ERR_RANGE;
    switch (e->op) {
    case TM_OP_ZERO:
        *bit = 0;
        return TM_OK;
    case TM_OP_ONE:
        *bit = 1;
        return TM_OK;
    case TM_OP_INPUT:
        if (e->input < 0 || e->input >= TM_DAG_MAX_INPUTS)
            return TM_ERR_RANGE;
        *bit = (int)((input >> e->input) & 1u);
        return TM_OK;
    case TM_OP_NOT:
        rc = tm_expr_eval(e->a, input, &x);
        if (rc != TM_OK)
            return rc;
        *bit = !x;
        return TM_OK;
    case TM_OP_AND:
    case TM_OP_OR:
    case TM_OP_XOR:
        rc = tm_expr_eval(e->a, input, &x);
        if (rc != TM_OK)
            return rc;
        rc = tm_expr_eval(e->b, input, &y);
        if (rc != TM_OK)
            return rc;
        if (e->op == TM_OP_AND)
            *bit = x & y;
        else if (e->op == TM_OP_OR)
            *bit = x | y;
        else
            *bit = x ^ y;
        return TM_OK;
    default:
        return TM_ERR_DOMAIN;
    }
}

static int max_input(const tm_expr *e)
{
    int m = -1, a, b;
    if (!e)
        return -1;
    if (e->op == TM_OP_INPUT)
        m = e->input;
    a = max_input(e->a);
    b = max_input(e->b);
    if (a > m)
        m = a;
    if (b > m)
        m = b;
    return m;
}

int tm_expr_inputs(const tm_expr *e)
{
    int m = max_input(e);
    return m < 0 ? 0 : m + 1;
}

int tm_expr_to_dag(tm_dag *d, const tm_expr *e, int *node)
{
    int a, b, n, rc;
    if (!d || !e || !node)
        return TM_ERR_RANGE;
    switch (e->op) {
    case TM_OP_ZERO:
        *node = 0;
        return TM_OK;
    case TM_OP_ONE:
        *node = 1;
        return TM_OK;
    case TM_OP_INPUT:
        n = tm_dag_input(d, e->input);
        if (n < 0)
            return n;
        *node = n;
        return TM_OK;
    case TM_OP_NOT:
        rc = tm_expr_to_dag(d, e->a, &a);
        if (rc != TM_OK)
            return rc;
        n = tm_dag_not(d, a);
        if (n < 0)
            return n;
        *node = n;
        return TM_OK;
    case TM_OP_AND:
    case TM_OP_OR:
    case TM_OP_XOR:
        rc = tm_expr_to_dag(d, e->a, &a);
        if (rc != TM_OK)
            return rc;
        rc = tm_expr_to_dag(d, e->b, &b);
        if (rc != TM_OK)
            return rc;
        if (e->op == TM_OP_AND)
            n = tm_dag_and(d, a, b);
        else if (e->op == TM_OP_OR)
            n = tm_dag_or(d, a, b);
        else
            n = tm_dag_xor(d, a, b);
        if (n < 0)
            return n;
        *node = n;
        return TM_OK;
    default:
        return TM_ERR_DOMAIN;
    }
}
