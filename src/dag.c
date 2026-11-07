/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * Boolean DAG with deterministic structural sharing.
 * Sharing is an exact (op, a, b) scan in creation order, not an e-graph.
 * Local rules: commutative operand order, idempotent AND/OR, XOR(x,x)=0,
 * constant folding through 0 and 1, and NOT(NOT(x))=x.
 * XOR(x,x) is 0. A prior starter returned node 1 (the constant ONE).
 */
#include "tm.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    tm_op op;
    int a, b;
} tm_node;

struct tm_dag {
    tm_node v[TM_DAG_MAX_NODES];
    int n;
};

tm_dag *tm_dag_new(void)
{
    tm_dag *d = calloc(1, sizeof *d);
    if (!d)
        return NULL;
    d->v[0].op = TM_OP_ZERO;
    d->v[1].op = TM_OP_ONE;
    d->n = 2;
    return d;
}

void tm_dag_free(tm_dag *d)
{
    free(d);
}

int tm_dag_n(const tm_dag *d)
{
    return d ? d->n : 0;
}

int tm_dag_op(const tm_dag *d, int node, tm_op *op, int *a, int *b)
{
    if (!d || !op || !a || !b || node < 0 || node >= d->n)
        return TM_ERR_RANGE;
    *op = d->v[node].op;
    *a = d->v[node].a;
    *b = d->v[node].b;
    return TM_OK;
}

static int valid(const tm_dag *d, int i)
{
    return d && i >= 0 && i < d->n;
}

static int share(tm_dag *d, tm_op op, int a, int b)
{
    int i;
    for (i = 0; i < d->n; i++) {
        if (d->v[i].op == op && d->v[i].a == a && d->v[i].b == b)
            return i;
    }
    if (d->n >= TM_DAG_MAX_NODES)
        return TM_ERR_CAPACITY;
    d->v[d->n].op = op;
    d->v[d->n].a = a;
    d->v[d->n].b = b;
    return d->n++;
}

int tm_dag_input(tm_dag *d, int bit)
{
    if (!d || bit < 0 || bit >= TM_DAG_MAX_INPUTS)
        return TM_ERR_RANGE;
    return share(d, TM_OP_INPUT, bit, 0);
}

int tm_dag_not(tm_dag *d, int a)
{
    if (!valid(d, a))
        return TM_ERR_RANGE;
    if (d->v[a].op == TM_OP_NOT)
        return d->v[a].a;
    if (d->v[a].op == TM_OP_ZERO)
        return 1;
    if (d->v[a].op == TM_OP_ONE)
        return 0;
    return share(d, TM_OP_NOT, a, 0);
}

static int binary(tm_dag *d, tm_op op, int a, int b)
{
    int t;
    if (!valid(d, a) || !valid(d, b))
        return TM_ERR_RANGE;
    if (a > b) {
        t = a;
        a = b;
        b = t;
    }
    if (a == b) {
        if (op == TM_OP_XOR)
            return 0;
        return a;
    }
    /* Nodes 0 and 1 are reserved constants, so the lower index tells us
     * whether a constant is involved. */
    if (a == 0) {
        if (op == TM_OP_AND)
            return 0;
        return b;
    }
    if (a == 1) {
        if (op == TM_OP_AND)
            return b;
        if (op == TM_OP_OR)
            return 1;
        return tm_dag_not(d, b);
    }
    return share(d, op, a, b);
}

int tm_dag_and(tm_dag *d, int a, int b)
{
    return binary(d, TM_OP_AND, a, b);
}

int tm_dag_or(tm_dag *d, int a, int b)
{
    return binary(d, TM_OP_OR, a, b);
}

int tm_dag_xor(tm_dag *d, int a, int b)
{
    return binary(d, TM_OP_XOR, a, b);
}

int tm_dag_eval(const tm_dag *d, int root, uint64_t input, int *bit)
{
    uint8_t val[TM_DAG_MAX_NODES];
    int i;

    if (!d || !bit || root < 0 || root >= d->n)
        return TM_ERR_RANGE;
    for (i = 0; i < d->n; i++) {
        int a = d->v[i].a;
        int b = d->v[i].b;
        switch (d->v[i].op) {
        case TM_OP_INPUT:
            val[i] = (uint8_t)((input >> a) & 1u);
            break;
        case TM_OP_ZERO:
            val[i] = 0;
            break;
        case TM_OP_ONE:
            val[i] = 1;
            break;
        case TM_OP_NOT:
            val[i] = (uint8_t)!val[a];
            break;
        case TM_OP_AND:
            val[i] = (uint8_t)(val[a] & val[b]);
            break;
        case TM_OP_OR:
            val[i] = (uint8_t)(val[a] | val[b]);
            break;
        case TM_OP_XOR:
            val[i] = (uint8_t)(val[a] ^ val[b]);
            break;
        default:
            return TM_ERR_DOMAIN;
        }
    }
    *bit = val[root];
    return TM_OK;
}
