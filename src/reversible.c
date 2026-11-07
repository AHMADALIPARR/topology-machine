/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * CX, Toffoli (CCX), and Fredkin (CSWAP) on a finite bit width.
 * A single gate of each kind is an involution. A sequence is a
 * permutation of the 2^width states and is reversible, but it is not
 * automatically an involution.
 */
#include "tm.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned mask_of(unsigned width)
{
    if (width >= 32)
        return 0xffffffffu;
    return (1u << width) - 1u;
}

int tm_rev_apply(unsigned state, tm_instr g, unsigned width, unsigned *out)
{
    unsigned mask, s, a, b;
    if (!out || width < 1 || width > TM_REV_MAX_WIRES)
        return TM_ERR_RANGE;
    mask = mask_of(width);
    if (state > mask)
        return TM_ERR_RANGE;
    if (g.a >= width || g.b >= width)
        return TM_ERR_RANGE;
    if (g.op != TM_CX && g.c >= width)
        return TM_ERR_RANGE;
    if (g.op == TM_CX && g.a == g.b)
        return TM_ERR_DOMAIN;
    if ((g.op == TM_CCX || g.op == TM_CSWAP) &&
        (g.a == g.b || g.a == g.c || g.b == g.c))
        return TM_ERR_DOMAIN;
    s = state;
    a = (s >> g.a) & 1u;
    b = (s >> g.b) & 1u;
    switch (g.op) {
    case TM_CX:
        if (a)
            s ^= 1u << g.b;
        break;
    case TM_CCX:
        if (a && b)
            s ^= 1u << g.c;
        break;
    case TM_CSWAP:
        if (a && (((s >> g.b) ^ (s >> g.c)) & 1u))
            s ^= (1u << g.b) | (1u << g.c);
        break;
    default:
        return TM_ERR_DOMAIN;
    }
    *out = s & mask;
    return TM_OK;
}

int tm_rev_eval(unsigned state, const tm_instr *prog, size_t n,
                unsigned width, unsigned *out)
{
    size_t i;
    unsigned s = state;
    if (!out || (n && !prog))
        return TM_ERR_RANGE;
    for (i = 0; i < n; i++) {
        int rc = tm_rev_apply(s, prog[i], width, &s);
        if (rc != TM_OK)
            return rc;
    }
    *out = s;
    return TM_OK;
}

static const char *skip(const char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    return s;
}

static int take_u(const char **ps, unsigned *out)
{
    const char *s = skip(*ps);
    char *end = NULL;
    unsigned long v;
    if (!isdigit((unsigned char)*s))
        return -1;
    errno = 0;
    v = strtoul(s, &end, 10);
    if (end == s || errno == ERANGE || v >= TM_REV_MAX_WIRES)
        return -1;
    *out = (unsigned)v;
    *ps = end;
    return 0;
}

int tm_rev_parse(const char *text, tm_instr *out, size_t cap, size_t *n,
                 unsigned width, char *err, size_t errlen)
{
    const char *s;
    size_t count = 0;
    if (!text || !n || width < 1 || width > TM_REV_MAX_WIRES)
        return TM_ERR_RANGE;
    s = text;
    while (*s) {
        tm_instr g;
        char op[16];
        size_t k = 0;
        unsigned dummy;
        int rc;
        s = skip(s);
        if (*s == '\0')
            break;
        if (*s == ';' || *s == ',') {
            s++;
            continue;
        }
        while (*s && !isspace((unsigned char)*s) && *s != ';' && *s != ',' && k + 1 < sizeof op)
            op[k++] = *s++;
        op[k] = '\0';
        g.c = 0;
        if (strcmp(op, "cx") == 0) {
            g.op = TM_CX;
            if (take_u(&s, &g.a) || take_u(&s, &g.b)) {
                if (err && errlen)
                    snprintf(err, errlen, "cx needs two wires");
                return TM_ERR_PARSE;
            }
        } else if (strcmp(op, "ccx") == 0 || strcmp(op, "cswap") == 0) {
            g.op = strcmp(op, "ccx") == 0 ? TM_CCX : TM_CSWAP;
            if (take_u(&s, &g.a) || take_u(&s, &g.b) || take_u(&s, &g.c)) {
                if (err && errlen)
                    snprintf(err, errlen, "%s needs three wires", op);
                return TM_ERR_PARSE;
            }
        } else {
            if (err && errlen)
                snprintf(err, errlen, "unknown gate");
            return TM_ERR_PARSE;
        }
        if (count >= cap || !out) {
            if (err && errlen)
                snprintf(err, errlen, "too many gates");
            return TM_ERR_CAPACITY;
        }
        rc = tm_rev_apply(0, g, width, &dummy);
        if (rc != TM_OK) {
            if (err && errlen)
                snprintf(err, errlen, "gate wires rejected");
            return rc;
        }
        out[count++] = g;
    }
    *n = count;
    return TM_OK;
}
