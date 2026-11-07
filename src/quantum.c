/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * Two-qubit Bell state and a subset OpenQASM 3 simulator.
 * Amplitudes are an explicit real/imag pair. This is not hardware,
 * not a BQP separation, and not a general OpenQASM 3 implementation.
 * Measurement lines are accepted only after the unitary prefix and do
 * not sample a collapse; the reported state is the pre-measurement vector.
 */
#include "tm.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char BELL[] =
    "OPENQASM 3.0;\n"
    "include \"stdgates.inc\";\n"
    "qubit[2] q;\n"
    "bit[2] c;\n"
    "h q[0];\n"
    "cx q[0], q[1];\n"
    "c[0] = measure q[0];\n"
    "c[1] = measure q[1];\n";

static const char REVQ[] =
    "OPENQASM 3.0;\n"
    "include \"stdgates.inc\";\n"
    "qubit[3] q;\n"
    "cx q[0], q[1];\n"
    "ccx q[0], q[1], q[2];\n"
    "cswap q[0], q[1], q[2];\n";

const char *tm_qasm_bell_text(void)
{
    return BELL;
}

const char *tm_qasm_reversible_text(void)
{
    return REVQ;
}

int tm_qasm_write_bell(FILE *fp)
{
    if (!fp)
        return TM_ERR_IO;
    if (fputs(BELL, fp) < 0)
        return TM_ERR_IO;
    return TM_OK;
}

int tm_qasm_write_reversible(FILE *fp)
{
    if (!fp)
        return TM_ERR_IO;
    if (fputs(REVQ, fp) < 0)
        return TM_ERR_IO;
    return TM_OK;
}

int tm_q_bell(tm_amp psi[4])
{
    tm_amp s[4];
    double k;
    int j;
    if (!psi)
        return TM_ERR_RANGE;
    memset(s, 0, sizeof s);
    s[0].re = 1.0;
    k = 1.0 / sqrt(2.0);
    for (j = 0; j < 4; j += 2) {
        double ar = s[j].re, ai = s[j].im;
        double br = s[j + 1].re, bi = s[j + 1].im;
        s[j].re = k * (ar + br);
        s[j].im = k * (ai + bi);
        s[j + 1].re = k * (ar - br);
        s[j + 1].im = k * (ai - bi);
    }
    {
        tm_amp t = s[1];
        s[1] = s[3];
        s[3] = t;
    }
    memcpy(psi, s, sizeof s);
    return TM_OK;
}

static void apply_h(tm_amp *psi, int nq, int q)
{
    int N = 1 << nq;
    double k = 1.0 / sqrt(2.0);
    int i;
    for (i = 0; i < N; i++) {
        int j;
        double ar, ai, br, bi;
        if ((i >> q) & 1)
            continue;
        j = i | (1 << q);
        ar = psi[i].re;
        ai = psi[i].im;
        br = psi[j].re;
        bi = psi[j].im;
        psi[i].re = k * (ar + br);
        psi[i].im = k * (ai + bi);
        psi[j].re = k * (ar - br);
        psi[j].im = k * (ai - bi);
    }
}

static void apply_x(tm_amp *psi, int nq, int q)
{
    int N = 1 << nq;
    int i;
    for (i = 0; i < N; i++) {
        int j;
        tm_amp t;
        if ((i >> q) & 1)
            continue;
        j = i | (1 << q);
        t = psi[i];
        psi[i] = psi[j];
        psi[j] = t;
    }
}

static void apply_cx(tm_amp *psi, int nq, int c, int tbit)
{
    int N = 1 << nq;
    int i;
    for (i = 0; i < N; i++) {
        int j;
        tm_amp t;
        if (((i >> c) & 1) == 0)
            continue;
        if ((i >> tbit) & 1)
            continue;
        j = i | (1 << tbit);
        t = psi[i];
        psi[i] = psi[j];
        psi[j] = t;
    }
}

static void apply_ccx(tm_amp *psi, int nq, int c1, int c2, int tbit)
{
    int N = 1 << nq;
    int i;
    for (i = 0; i < N; i++) {
        int j;
        tm_amp t;
        if (((i >> c1) & 1) == 0 || ((i >> c2) & 1) == 0)
            continue;
        if ((i >> tbit) & 1)
            continue;
        j = i | (1 << tbit);
        t = psi[i];
        psi[i] = psi[j];
        psi[j] = t;
    }
}

static int with_bits_swapped(int i, int a, int b)
{
    int va = (i >> a) & 1;
    int vb = (i >> b) & 1;
    if (va == vb)
        return i;
    return i ^ ((1 << a) | (1 << b));
}

static void apply_cswap(tm_amp *psi, int nq, int c, int a, int b)
{
    int N = 1 << nq;
    int i;
    for (i = 0; i < N; i++) {
        int j;
        tm_amp t;
        if (((i >> c) & 1) == 0)
            continue;
        j = with_bits_swapped(i, a, b);
        if (j <= i)
            continue;
        t = psi[i];
        psi[i] = psi[j];
        psi[j] = t;
    }
}

static void seterr(char *err, size_t n, const char *msg)
{
    if (err && n)
        snprintf(err, n, "%s", msg);
}

static int parse_index(const char **ps, char reg, int *idx, char *err, size_t errlen)
{
    const char *s = *ps;
    char *end = NULL;
    long v;
    while (*s && isspace((unsigned char)*s))
        s++;
    if (*s != reg || s[1] != '[') {
        seterr(err, errlen, "expected register index");
        return -1;
    }
    s += 2;
    v = strtol(s, &end, 10);
    if (end == s || *end != ']' || v < 0 || v >= TM_QASM_MAX_QUBITS) {
        seterr(err, errlen, "bad register index");
        return -1;
    }
    *idx = (int)v;
    *ps = end + 1;
    return 0;
}

static int q_ok(int q, int nq, char *err, size_t errlen)
{
    if (nq < 1) {
        seterr(err, errlen, "qubit register missing");
        return -1;
    }
    if (q < 0 || q >= nq) {
        seterr(err, errlen, "qubit index out of range");
        return -1;
    }
    return 0;
}

int tm_qasm_sim_memory(const char *text, size_t len, tm_amp *psi, int cap,
                       int *nqubits, char *err, size_t errlen)
{
    size_t i = 0;
    int nq = -1;
    int saw_version = 0;
    int measured = 0;
    if (!text || !psi || !nqubits || cap < 1)
        return TM_ERR_RANGE;
    while (i < len) {
        size_t start = i;
        size_t end;
        char buf[256];
        char *p;
        size_t n;
        while (i < len && text[i] != '\n')
            i++;
        end = i;
        if (i < len && text[i] == '\n')
            i++;
        n = end - start;
        if (n >= sizeof buf) {
            seterr(err, errlen, "qasm line too long");
            return TM_ERR_PARSE;
        }
        memcpy(buf, text + start, n);
        buf[n] = '\0';
        if (n && buf[n - 1] == '\r')
            buf[n - 1] = '\0';
        p = buf;
        while (*p && isspace((unsigned char)*p))
            p++;
        {
            char *cmt = strstr(p, "//");
            if (cmt)
                *cmt = '\0';
        }
        {
            size_t L = strlen(p);
            while (L && isspace((unsigned char)p[L - 1]))
                p[--L] = '\0';
            if (L && p[L - 1] == ';')
                p[--L] = '\0';
            while (L && isspace((unsigned char)p[L - 1]))
                p[--L] = '\0';
        }
        if (*p == '\0')
            continue;
        if (strncmp(p, "OPENQASM", 8) == 0) {
            if (strcmp(p, "OPENQASM 3.0") != 0) {
                seterr(err, errlen, "only OPENQASM 3.0 is accepted");
                return TM_ERR_PARSE;
            }
            saw_version = 1;
            continue;
        }
        if (!saw_version) {
            seterr(err, errlen, "missing OPENQASM 3.0");
            return TM_ERR_PARSE;
        }
        if (strncmp(p, "include ", 8) == 0)
            continue;
        if (strncmp(p, "qubit[", 6) == 0) {
            char *end = NULL;
            long v = strtol(p + 6, &end, 10);
            if (!end || end[0] != ']' || v < 1 || v > TM_QASM_MAX_QUBITS) {
                seterr(err, errlen, "unsupported qubit register");
                return TM_ERR_PARSE;
            }
            if (nq >= 0) {
                seterr(err, errlen, "only one qubit register is supported");
                return TM_ERR_PARSE;
            }
            nq = (int)v;
            if ((1 << nq) > cap) {
                seterr(err, errlen, "state vector capacity exceeded");
                return TM_ERR_CAPACITY;
            }
            memset(psi, 0, (size_t)(1 << nq) * sizeof *psi);
            psi[0].re = 1.0;
            continue;
        }
        if (strncmp(p, "bit[", 4) == 0)
            continue;
        if (p[0] == 'h' && isspace((unsigned char)p[1])) {
            if (measured) {
                seterr(err, errlen, "gate after measurement is outside this simulator");
                return TM_ERR_PARSE;
            }
            const char *s = p + 1;
            int q;
            if (parse_index(&s, 'q', &q, err, errlen) || q_ok(q, nq, err, errlen))
                return TM_ERR_PARSE;
            while (*s && isspace((unsigned char)*s))
                s++;
            if (*s) {
                seterr(err, errlen, "trailing junk after h");
                return TM_ERR_PARSE;
            }
            apply_h(psi, nq, q);
            continue;
        }
        if (p[0] == 'x' && isspace((unsigned char)p[1])) {
            if (measured) {
                seterr(err, errlen, "gate after measurement is outside this simulator");
                return TM_ERR_PARSE;
            }
            const char *s = p + 1;
            int q;
            if (parse_index(&s, 'q', &q, err, errlen) || q_ok(q, nq, err, errlen))
                return TM_ERR_PARSE;
            apply_x(psi, nq, q);
            continue;
        }
        if (strncmp(p, "cx ", 3) == 0) {
            if (measured) {
                seterr(err, errlen, "gate after measurement is outside this simulator");
                return TM_ERR_PARSE;
            }
            const char *s = p + 2;
            int a, b;
            if (parse_index(&s, 'q', &a, err, errlen) || q_ok(a, nq, err, errlen))
                return TM_ERR_PARSE;
            while (*s && (isspace((unsigned char)*s) || *s == ','))
                s++;
            if (parse_index(&s, 'q', &b, err, errlen) || q_ok(b, nq, err, errlen))
                return TM_ERR_PARSE;
            if (a == b) {
                seterr(err, errlen, "cx wires must differ");
                return TM_ERR_DOMAIN;
            }
            apply_cx(psi, nq, a, b);
            continue;
        }
        if (strncmp(p, "ccx ", 4) == 0 || strncmp(p, "cswap ", 6) == 0) {
            int is_ccx = p[1] == 'c';
            if (measured) {
                seterr(err, errlen, "gate after measurement is outside this simulator");
                return TM_ERR_PARSE;
            }
            const char *s = p + (is_ccx ? 3 : 5);
            int a, b, c;
            if (parse_index(&s, 'q', &a, err, errlen) || q_ok(a, nq, err, errlen))
                return TM_ERR_PARSE;
            while (*s && (isspace((unsigned char)*s) || *s == ','))
                s++;
            if (parse_index(&s, 'q', &b, err, errlen) || q_ok(b, nq, err, errlen))
                return TM_ERR_PARSE;
            while (*s && (isspace((unsigned char)*s) || *s == ','))
                s++;
            if (parse_index(&s, 'q', &c, err, errlen) || q_ok(c, nq, err, errlen))
                return TM_ERR_PARSE;
            if (a == b || a == c || b == c) {
                seterr(err, errlen, "gate wires must be distinct");
                return TM_ERR_DOMAIN;
            }
            if (is_ccx)
                apply_ccx(psi, nq, a, b, c);
            else
                apply_cswap(psi, nq, a, b, c);
            continue;
        }
        if (p[0] == 'c' && strchr(p, '=')) {
            /* c[k] = measure q[j] */
            const char *s = p;
            int dst, src;
            if (parse_index(&s, 'c', &dst, err, errlen))
                return TM_ERR_PARSE;
            while (*s && isspace((unsigned char)*s))
                s++;
            if (*s != '=') {
                seterr(err, errlen, "expected measure assignment");
                return TM_ERR_PARSE;
            }
            s++;
            while (*s && isspace((unsigned char)*s))
                s++;
            if (strncmp(s, "measure", 7) != 0) {
                seterr(err, errlen, "only measure assignments are accepted");
                return TM_ERR_PARSE;
            }
            s += 7;
            if (parse_index(&s, 'q', &src, err, errlen) || q_ok(src, nq, err, errlen))
                return TM_ERR_PARSE;
            (void)dst;
            measured = 1;
            continue;
        }
        seterr(err, errlen, "unsupported qasm statement");
        return TM_ERR_PARSE;
    }
    if (!saw_version) {
        seterr(err, errlen, "missing OPENQASM 3.0");
        return TM_ERR_PARSE;
    }
    if (nq < 0) {
        seterr(err, errlen, "qubit register missing");
        return TM_ERR_PARSE;
    }
    *nqubits = nq;
    return TM_OK;
}

int tm_qasm_sim_path(const char *path, tm_amp *psi, int cap, int *nqubits,
                     char *err, size_t errlen)
{
    FILE *fp;
    char *buf;
    long sz;
    size_t nread;
    int rc;
    if (!path)
        return TM_ERR_IO;
    fp = fopen(path, "rb");
    if (!fp) {
        seterr(err, errlen, "cannot open qasm");
        return TM_ERR_IO;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return TM_ERR_IO;
    }
    sz = ftell(fp);
    if (sz < 0 || sz > 1024 * 1024) {
        fclose(fp);
        seterr(err, errlen, "qasm file is empty or too large");
        return TM_ERR_IO;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return TM_ERR_IO;
    }
    buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(fp);
        return TM_ERR_NOMEM;
    }
    nread = fread(buf, 1, (size_t)sz, fp);
    buf[nread] = '\0';
    fclose(fp);
    if (nread != (size_t)sz) {
        free(buf);
        return TM_ERR_IO;
    }
    rc = tm_qasm_sim_memory(buf, nread, psi, cap, nqubits, err, errlen);
    free(buf);
    return rc;
}
