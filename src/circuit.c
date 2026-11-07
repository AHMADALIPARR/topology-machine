/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * Finite GF(2^8) multiplier netlist. One circuit for 8-bit inputs.
 * This is not a P/poly family and not an FPGA bitstream.
 *
 * Text format (tm-netlist 1):
 *   # comments and blank lines are ignored
 *   tm-netlist 1
 *   input <name>
 *   const <0|1> <name>
 *   and <a> <b> <name>
 *   xor <a> <b> <name>
 *   not <a> <name>
 *   output <name> <gate>
 * Names refer only to earlier gates. Evaluation is declaration order.
 */
#include "tm.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    NK_CONST = 1,
    NK_INPUT = 2,
    NK_AND = 3,
    NK_XOR = 4,
    NK_NOT = 5
};

typedef struct {
    int kind;
    int a, b;
    int cbit;
    char name[TM_NET_NAME];
} gate_t;

struct tm_net {
    gate_t *g;
    int n, cap;
    int n_in, n_out;
    int out_at[64];
    char out_name[64][TM_NET_NAME];
    char note[128];
};

void tm_net_free(tm_net *n)
{
    if (!n)
        return;
    free(n->g);
    free(n);
}

int tm_net_ngates(const tm_net *n)
{
    return n ? n->n : 0;
}

int tm_net_ninputs(const tm_net *n)
{
    return n ? n->n_in : 0;
}

int tm_net_noutputs(const tm_net *n)
{
    return n ? n->n_out : 0;
}

static int grow(tm_net *n)
{
    int cap;
    gate_t *g;
    if (n->n < n->cap)
        return TM_OK;
    if (n->cap >= TM_NET_MAX_GATES)
        return TM_ERR_CAPACITY;
    cap = n->cap ? n->cap * 2 : 64;
    if (cap > TM_NET_MAX_GATES)
        cap = TM_NET_MAX_GATES;
    g = realloc(n->g, (size_t)cap * sizeof *g);
    if (!g)
        return TM_ERR_NOMEM;
    n->g = g;
    n->cap = cap;
    return TM_OK;
}

static int name_ok(const char *s)
{
    size_t i;
    if (!s || !s[0] || strlen(s) >= TM_NET_NAME)
        return 0;
    if (!isalpha((unsigned char)s[0]) && s[0] != '_')
        return 0;
    for (i = 1; s[i]; i++) {
        if (!isalnum((unsigned char)s[i]) && s[i] != '_')
            return 0;
    }
    return 1;
}

static int find_name(const tm_net *n, const char *name)
{
    int i;
    for (i = 0; i < n->n; i++) {
        if (strcmp(n->g[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int add_gate(tm_net *n, int kind, int a, int b, int cbit, const char *name)
{
    int rc;
    if (!name_ok(name) || find_name(n, name) >= 0)
        return TM_ERR_PARSE;
    rc = grow(n);
    if (rc != TM_OK)
        return rc;
    n->g[n->n].kind = kind;
    n->g[n->n].a = a;
    n->g[n->n].b = b;
    n->g[n->n].cbit = cbit;
    memcpy(n->g[n->n].name, name, strlen(name) + 1);
    if (kind == NK_INPUT)
        n->n_in++;
    return n->n++;
}

static int find_and(const tm_net *n, int a, int b)
{
    int i;
    if (a > b) {
        int t = a;
        a = b;
        b = t;
    }
    for (i = 0; i < n->n; i++) {
        if (n->g[i].kind == NK_AND && n->g[i].a == a && n->g[i].b == b)
            return i;
    }
    return -1;
}

int tm_net_gf256_mul(tm_net **out)
{
    tm_net *n;
    uint64_t acc[16];
    int in_a[8], in_b[8];
    int ia, ib, i, k;
    char name[TM_NET_NAME];

    if (!out)
        return TM_ERR_RANGE;
    n = calloc(1, sizeof *n);
    if (!n)
        return TM_ERR_NOMEM;
    snprintf(n->note, sizeof n->note, "gf256-mul aes-poly 0x11b");
    for (ia = 0; ia < 8; ia++) {
        snprintf(name, sizeof name, "a%d", ia);
        in_a[ia] = add_gate(n, NK_INPUT, 0, 0, 0, name);
        if (in_a[ia] < 0) {
            int rc = in_a[ia];
            tm_net_free(n);
            return rc;
        }
    }
    for (ib = 0; ib < 8; ib++) {
        snprintf(name, sizeof name, "b%d", ib);
        in_b[ib] = add_gate(n, NK_INPUT, 0, 0, 0, name);
        if (in_b[ib] < 0) {
            int rc = in_b[ib];
            tm_net_free(n);
            return rc;
        }
    }
    memset(acc, 0, sizeof acc);
    for (ib = 0; ib < 8; ib++) {
        for (ia = 0; ia < 8; ia++)
            acc[ia + ib] ^= 1ull << (ia + 8 * ib);
    }
    /* Fold high degrees downward so a newly written lower bit is reduced later. */
    for (i = 14; i >= 8; i--) {
        uint64_t hi = acc[i];
        acc[i] = 0;
        acc[i - 8 + 0] ^= hi;
        acc[i - 8 + 1] ^= hi;
        acc[i - 8 + 3] ^= hi;
        acc[i - 8 + 4] ^= hi;
    }
    for (k = 0; k < 8; k++) {
        int acc_g = -1;
        int seq = 0;
        int term;
        for (term = 0; term < 64; term++) {
            int g, aa, bb;
            if (((acc[k] >> term) & 1ull) == 0)
                continue;
            ia = term & 7;
            ib = term >> 3;
            aa = in_a[ia];
            bb = in_b[ib];
            g = find_and(n, aa, bb);
            if (g < 0) {
                if (aa > bb) {
                    int t = aa;
                    aa = bb;
                    bb = t;
                }
                snprintf(name, sizeof name, "and_a%d_b%d", ia, ib);
                g = add_gate(n, NK_AND, aa, bb, 0, name);
                if (g < 0) {
                    tm_net_free(n);
                    return g;
                }
            }
            if (acc_g < 0) {
                acc_g = g;
            } else {
                int x;
                snprintf(name, sizeof name, "xor_y%d_%d", k, seq++);
                x = add_gate(n, NK_XOR, acc_g, g, 0, name);
                if (x < 0) {
                    tm_net_free(n);
                    return x;
                }
                acc_g = x;
            }
        }
        if (acc_g < 0) {
            int c = find_name(n, "c0");
            if (c < 0) {
                c = add_gate(n, NK_CONST, 0, 0, 0, "c0");
                if (c < 0) {
                    tm_net_free(n);
                    return c;
                }
            }
            acc_g = c;
        }
        snprintf(n->out_name[n->n_out], TM_NET_NAME, "y%d", k);
        n->out_at[n->n_out++] = acc_g;
    }
    *out = n;
    return TM_OK;
}

int tm_net_write(const tm_net *n, FILE *fp)
{
    int i;
    if (!n || !fp)
        return TM_ERR_IO;
    if (fprintf(fp, "# topology-machine netlist 1\n") < 0)
        return TM_ERR_IO;
    if (n->note[0] && fprintf(fp, "# model %s\n", n->note) < 0)
        return TM_ERR_IO;
    if (fprintf(fp, "tm-netlist 1\n") < 0)
        return TM_ERR_IO;
    for (i = 0; i < n->n; i++) {
        const gate_t *g = &n->g[i];
        int rc = 0;
        switch (g->kind) {
        case NK_INPUT:
            rc = fprintf(fp, "input %s\n", g->name);
            break;
        case NK_CONST:
            rc = fprintf(fp, "const %d %s\n", g->cbit, g->name);
            break;
        case NK_AND:
            rc = fprintf(fp, "and %s %s %s\n", n->g[g->a].name, n->g[g->b].name, g->name);
            break;
        case NK_XOR:
            rc = fprintf(fp, "xor %s %s %s\n", n->g[g->a].name, n->g[g->b].name, g->name);
            break;
        case NK_NOT:
            rc = fprintf(fp, "not %s %s\n", n->g[g->a].name, g->name);
            break;
        default:
            return TM_ERR_DOMAIN;
        }
        if (rc < 0)
            return TM_ERR_IO;
    }
    for (i = 0; i < n->n_out; i++) {
        if (fprintf(fp, "output %s %s\n", n->out_name[i], n->g[n->out_at[i]].name) < 0)
            return TM_ERR_IO;
    }
    return TM_OK;
}

int tm_net_write_path(const tm_net *n, const char *path)
{
    FILE *fp;
    int rc;
    if (!path)
        return TM_ERR_IO;
    fp = fopen(path, "w");
    if (!fp)
        return TM_ERR_IO;
    rc = tm_net_write(n, fp);
    if (fclose(fp) != 0)
        return TM_ERR_IO;
    return rc;
}

static void seterr(char *err, size_t n, const char *msg)
{
    if (err && n)
        snprintf(err, n, "%s", msg);
}

static int add_output_name(tm_net *n, const char *name, int gate)
{
    int i;
    if (!name_ok(name))
        return TM_ERR_PARSE;
    for (i = 0; i < n->n_out; i++) {
        if (strcmp(n->out_name[i], name) == 0)
            return TM_ERR_PARSE;
    }
    if (n->n_out >= 64)
        return TM_ERR_CAPACITY;
    memcpy(n->out_name[n->n_out], name, strlen(name) + 1);
    n->out_at[n->n_out++] = gate;
    return TM_OK;
}

int tm_net_read(FILE *fp, tm_net **out, char *err, size_t errlen)
{
    tm_net *n;
    char line[512];
    int lineno = 0;
    int saw_magic = 0;
    if (!fp || !out)
        return TM_ERR_PARSE;
    n = calloc(1, sizeof *n);
    if (!n)
        return TM_ERR_NOMEM;
    while (fgets(line, sizeof line, fp)) {
        char *p = line;
        char *tok[6] = {0};
        int nt = 0;
        size_t len;
        lineno++;
        if (strchr(line, '\n') == NULL && !feof(fp)) {
            seterr(err, errlen, "line too long");
            tm_net_free(n);
            return TM_ERR_PARSE;
        }
        len = strlen(line);
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        while (*p && isspace((unsigned char)*p))
            p++;
        if (*p == '\0' || *p == '#')
            continue;
        while (*p && nt < 6) {
            tok[nt++] = p;
            while (*p && !isspace((unsigned char)*p))
                p++;
            if (*p) {
                *p++ = '\0';
                while (*p && isspace((unsigned char)*p))
                    p++;
            }
        }
        if (*p) {
            seterr(err, errlen, "too many tokens");
            tm_net_free(n);
            return TM_ERR_PARSE;
        }
        /* A non-empty line always yields one token. Guard anyway so tok[0]
         * is not read when nt is 0 (GCC -Wmaybe-uninitialized). */
        if (nt < 1) {
            seterr(err, errlen, "unknown netlist statement");
            tm_net_free(n);
            return TM_ERR_PARSE;
        }
        if (!saw_magic) {
            if (nt == 2 && strcmp(tok[0], "tm-netlist") == 0 && strcmp(tok[1], "1") == 0) {
                saw_magic = 1;
                continue;
            }
            seterr(err, errlen, "expected 'tm-netlist 1'");
            tm_net_free(n);
            return TM_ERR_PARSE;
        }
        if (nt >= 1 && strcmp(tok[0], "input") == 0 && nt == 2) {
            int g = add_gate(n, NK_INPUT, 0, 0, 0, tok[1]);
            if (g < 0) {
                seterr(err, errlen, "bad input");
                tm_net_free(n);
                return g;
            }
        } else if (strcmp(tok[0], "const") == 0 && nt == 3) {
            int bit;
            int g;
            if ((strcmp(tok[1], "0") != 0 && strcmp(tok[1], "1") != 0)) {
                seterr(err, errlen, "const must be 0 or 1");
                tm_net_free(n);
                return TM_ERR_PARSE;
            }
            bit = tok[1][0] - '0';
            g = add_gate(n, NK_CONST, 0, 0, bit, tok[2]);
            if (g < 0) {
                seterr(err, errlen, "bad const");
                tm_net_free(n);
                return g;
            }
        } else if ((strcmp(tok[0], "and") == 0 || strcmp(tok[0], "xor") == 0) && nt == 4) {
            int a = find_name(n, tok[1]);
            int b = find_name(n, tok[2]);
            int g;
            if (a < 0 || b < 0) {
                seterr(err, errlen, "unknown gate reference");
                tm_net_free(n);
                return TM_ERR_PARSE;
            }
            g = add_gate(n, strcmp(tok[0], "and") == 0 ? NK_AND : NK_XOR, a, b, 0, tok[3]);
            if (g < 0) {
                seterr(err, errlen, "bad gate");
                tm_net_free(n);
                return g == TM_ERR_PARSE ? TM_ERR_PARSE : g;
            }
        } else if (strcmp(tok[0], "not") == 0 && nt == 3) {
            int a = find_name(n, tok[1]);
            int g;
            if (a < 0) {
                seterr(err, errlen, "unknown gate reference");
                tm_net_free(n);
                return TM_ERR_PARSE;
            }
            g = add_gate(n, NK_NOT, a, 0, 0, tok[2]);
            if (g < 0) {
                seterr(err, errlen, "bad not");
                tm_net_free(n);
                return g;
            }
        } else if (strcmp(tok[0], "output") == 0 && nt == 3) {
            int a = find_name(n, tok[2]);
            int rc;
            if (a < 0) {
                seterr(err, errlen, "unknown output reference");
                tm_net_free(n);
                return TM_ERR_PARSE;
            }
            rc = add_output_name(n, tok[1], a);
            if (rc != TM_OK) {
                seterr(err, errlen, "bad output");
                tm_net_free(n);
                return rc;
            }
        } else {
            seterr(err, errlen, "unknown netlist statement");
            tm_net_free(n);
            return TM_ERR_PARSE;
        }
        (void)lineno;
    }
    if (!saw_magic) {
        seterr(err, errlen, "empty netlist");
        tm_net_free(n);
        return TM_ERR_PARSE;
    }
    if (n->n_out == 0) {
        seterr(err, errlen, "netlist has no outputs");
        tm_net_free(n);
        return TM_ERR_PARSE;
    }
    *out = n;
    return TM_OK;
}

int tm_net_read_path(const char *path, tm_net **out, char *err, size_t errlen)
{
    FILE *fp;
    int rc;
    if (!path)
        return TM_ERR_IO;
    fp = fopen(path, "r");
    if (!fp) {
        seterr(err, errlen, "cannot open netlist");
        return TM_ERR_IO;
    }
    rc = tm_net_read(fp, out, err, errlen);
    fclose(fp);
    return rc;
}

int tm_net_eval(const tm_net *n, uint64_t inputs, uint64_t *outputs)
{
    uint8_t val[TM_NET_MAX_GATES];
    int i, in_i;
    uint64_t packed;

    if (!n || !outputs)
        return TM_ERR_RANGE;
    if (n->n > TM_NET_MAX_GATES || n->n_in > 64 || n->n_out > 64)
        return TM_ERR_CAPACITY;
    in_i = 0;
    for (i = 0; i < n->n; i++) {
        switch (n->g[i].kind) {
        case NK_INPUT:
            val[i] = (uint8_t)((inputs >> in_i) & 1u);
            in_i++;
            break;
        case NK_CONST:
            val[i] = (uint8_t)(n->g[i].cbit & 1);
            break;
        case NK_AND:
            val[i] = (uint8_t)(val[n->g[i].a] & val[n->g[i].b]);
            break;
        case NK_XOR:
            val[i] = (uint8_t)(val[n->g[i].a] ^ val[n->g[i].b]);
            break;
        case NK_NOT:
            val[i] = (uint8_t)!val[n->g[i].a];
            break;
        default:
            return TM_ERR_DOMAIN;
        }
    }
    packed = 0;
    for (i = 0; i < n->n_out; i++) {
        if (val[n->out_at[i]])
            packed |= 1ull << i;
    }
    *outputs = packed;
    return TM_OK;
}
