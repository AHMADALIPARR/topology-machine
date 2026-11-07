/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "tm.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void help(FILE *fp)
{
    fprintf(fp,
            "Topology Machine %s\n"
            "Discrete Boolean, finite-field, reversible, amplitude, and nodal models.\n"
            "\n"
            "Usage:\n"
            "  tm help\n"
            "  tm version\n"
            "  tm self-check [--out-dir DIR]\n"
            "  tm oracle\n"
            "  tm bench\n"
            "  tm dag eval --expr EXPR --input N\n"
            "  tm dag truth --expr EXPR [--bits N]\n"
            "  tm gf mul A B\n"
            "  tm gf inv A\n"
            "  tm gf pow A EXP\n"
            "  tm gf check [--full]\n"
            "  tm gf circuit --out FILE\n"
            "  tm rev apply --width N --state S --prog PROG\n"
            "  tm rev check\n"
            "  tm qasm bell --out FILE\n"
            "  tm qasm reversible --out FILE\n"
            "  tm qasm sim FILE\n"
            "  tm analog solve --g ROWS --i VALUES\n"
            "  tm smt involution --out FILE\n"
            "  tm smt check\n"
            "\n"
            "Integers are decimal or 0x hex. Expressions:\n"
            "  0  1  i0  not(e)  and(e,e)  or(e,e)  xor(e,e)\n"
            "Reversible PROG uses cx, ccx, and cswap with decimal wires.\n"
            "Analog --g is 'a,b;c,d' and --i is 'i0,i1' in the C locale.\n"
            "Exit 0 ok, 1 failed check, 2 usage, 3 I/O, 4 domain.\n",
            TM_VERSION);
}

static int usage(const char *msg)
{
    fprintf(stderr, "tm: %s\n", msg);
    fprintf(stderr, "tm: try 'tm help'\n");
    return 2;
}

static int map_err(int rc)
{
    if (rc == TM_OK)
        return 0;
    fprintf(stderr, "tm: %s\n", tm_strerror(rc));
    if (rc == TM_ERR_IO)
        return 3;
    if (rc == TM_ERR_PARSE)
        return 2;
    return 4;
}

static const char *flag(int argc, char **argv, const char *name)
{
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], name) == 0 && i + 1 < argc)
            return argv[i + 1];
    }
    return NULL;
}

static int has_flag(int argc, char **argv, const char *name)
{
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], name) == 0)
            return 1;
    }
    return 0;
}

static int cmd_self(int argc, char **argv)
{
    const char *dir = flag(argc, argv, "--out-dir");
    int rc;
    if (!dir)
        dir = "out";
    rc = tm_selfcheck(stdout, dir);
    return rc == TM_OK ? 0 : 1;
}

static int cmd_dag(int argc, char **argv)
{
    const char *expr_s;
    tm_expr *e = NULL;
    char err[160];
    int rc;
    if (argc < 1)
        return usage("dag needs a subcommand");
    expr_s = flag(argc, argv, "--expr");
    if (!expr_s)
        return usage("dag requires --expr");
    rc = tm_expr_parse(expr_s, &e, err, sizeof err);
    if (rc != TM_OK) {
        fprintf(stderr, "tm: %s\n", err[0] ? err : tm_strerror(rc));
        return 2;
    }
    if (strcmp(argv[0], "eval") == 0) {
        const char *in_s = flag(argc, argv, "--input");
        uint64_t input = 0;
        tm_dag *d;
        int node = 0, bit = 0;
        if (!in_s) {
            tm_expr_free(e);
            return usage("dag eval requires --input");
        }
        if (tm_parse_u64(in_s, &input) != TM_OK) {
            tm_expr_free(e);
            return usage("bad --input");
        }
        d = tm_dag_new();
        if (!d) {
            tm_expr_free(e);
            return map_err(TM_ERR_NOMEM);
        }
        rc = tm_expr_to_dag(d, e, &node);
        if (rc == TM_OK)
            rc = tm_dag_eval(d, node, input, &bit);
        tm_dag_free(d);
        tm_expr_free(e);
        if (rc != TM_OK)
            return map_err(rc);
        printf("%d\n", bit);
        return 0;
    }
    if (strcmp(argv[0], "truth") == 0) {
        const char *bits_s = flag(argc, argv, "--bits");
        int bits = tm_expr_inputs(e);
        uint64_t n, i;
        tm_dag *d;
        int node = 0;
        if (bits_s) {
            uint64_t v = 0;
            if (tm_parse_u64(bits_s, &v) != TM_OK || v > 16) {
                tm_expr_free(e);
                return usage("--bits must be 0..16");
            }
            bits = (int)v;
        }
        if (tm_expr_inputs(e) > bits) {
            tm_expr_free(e);
            return usage("expression uses an input outside --bits");
        }
        d = tm_dag_new();
        if (!d) {
            tm_expr_free(e);
            return map_err(TM_ERR_NOMEM);
        }
        rc = tm_expr_to_dag(d, e, &node);
        if (rc != TM_OK) {
            tm_dag_free(d);
            tm_expr_free(e);
            return map_err(rc);
        }
        n = bits == 0 ? 1ull : (1ull << bits);
        for (i = 0; i < n; i++) {
            int bit = 0, b;
            int tree = 0;
            rc = tm_dag_eval(d, node, i, &bit);
            if (rc != TM_OK || tm_expr_eval(e, i, &tree) != TM_OK || tree != bit) {
                tm_dag_free(d);
                tm_expr_free(e);
                fprintf(stderr, "tm: dag and tree evaluators diverged\n");
                return 1;
            }
            for (b = bits - 1; b >= 0; b--)
                putchar(((i >> b) & 1ull) ? '1' : '0');
            if (bits == 0)
                putchar('-');
            printf(" %d\n", bit);
        }
        tm_dag_free(d);
        tm_expr_free(e);
        return 0;
    }
    tm_expr_free(e);
    return usage("unknown dag subcommand");
}

static int parse_byte(const char *s, uint8_t *out)
{
    uint64_t v = 0;
    if (tm_parse_u64(s, &v) != TM_OK || v > 255)
        return -1;
    *out = (uint8_t)v;
    return 0;
}

static int cmd_gf(int argc, char **argv)
{
    if (argc < 1)
        return usage("gf needs a subcommand");
    if (strcmp(argv[0], "mul") == 0) {
        uint8_t a, b;
        if (argc < 3)
            return usage("gf mul A B");
        if (parse_byte(argv[1], &a) || parse_byte(argv[2], &b))
            return usage("gf operands must be bytes");
        printf("0x%02x\n", tm_gf_mul(a, b));
        return 0;
    }
    if (strcmp(argv[0], "inv") == 0) {
        uint8_t a, inv = 0;
        int rc;
        if (argc < 2)
            return usage("gf inv A");
        if (parse_byte(argv[1], &a))
            return usage("gf operand must be a byte");
        rc = tm_gf_inv(a, &inv);
        if (rc != TM_OK)
            return map_err(rc);
        printf("0x%02x\n", inv);
        return 0;
    }
    if (strcmp(argv[0], "pow") == 0) {
        uint8_t a;
        uint64_t e = 0;
        if (argc < 3)
            return usage("gf pow A EXP");
        if (parse_byte(argv[1], &a) || tm_parse_u64(argv[2], &e) != TM_OK || e > 1000000ull)
            return usage("bad gf pow arguments");
        printf("0x%02x\n", tm_gf_pow(a, (unsigned)e));
        return 0;
    }
    if (strcmp(argv[0], "check") == 0) {
        int full = has_flag(argc, argv, "--full");
        int a, b, c;
        for (a = 0; a < 256; a++) {
            if (tm_gf_mul((uint8_t)a, 0) != 0 || tm_gf_mul((uint8_t)a, 1) != (uint8_t)a)
                return 1;
            for (b = 0; b < 256; b++) {
                uint8_t ua = (uint8_t)a, ub = (uint8_t)b;
                if (tm_gf_mul(ua, ub) != tm_gf_mul(ub, ua))
                    return 1;
                if (tm_gf_mul(ua, ub) != tm_gf_mul_rec(ua, ub))
                    return 1;
            }
        }
        printf("gf check: commutativity and iterative=recursive on 65536 pairs\n");
        if (!full) {
            printf("gf check: associativity and distributivity not run (pass --full)\n");
            return 0;
        }
        for (a = 0; a < 256; a++) {
            for (b = 0; b < 256; b++) {
                for (c = 0; c < 256; c++) {
                    uint8_t ua = (uint8_t)a, ub = (uint8_t)b, uc = (uint8_t)c;
                    uint8_t left = tm_gf_mul(tm_gf_mul(ua, ub), uc);
                    uint8_t right = tm_gf_mul(ua, tm_gf_mul(ub, uc));
                    uint8_t d1 = tm_gf_mul((uint8_t)(ua ^ ub), uc);
                    uint8_t d2 = (uint8_t)(tm_gf_mul(ua, uc) ^ tm_gf_mul(ub, uc));
                    if (left != right || d1 != d2) {
                        fprintf(stderr, "tm: field law failed at %d %d %d\n", a, b, c);
                        return 1;
                    }
                }
            }
        }
        printf("gf check: associativity and distributivity on 16777216 triples\n");
        return 0;
    }
    if (strcmp(argv[0], "circuit") == 0) {
        const char *path = flag(argc, argv, "--out");
        tm_net *net = NULL;
        int rc, a, b;
        if (!path)
            return usage("gf circuit requires --out");
        rc = tm_net_gf256_mul(&net);
        if (rc != TM_OK)
            return map_err(rc);
        for (a = 0; a < 256; a++) {
            for (b = 0; b < 256; b++) {
                uint64_t in = (uint64_t)a | ((uint64_t)b << 8);
                uint64_t y = 0;
                if (tm_net_eval(net, in, &y) != TM_OK || (uint8_t)y != tm_gf_mul((uint8_t)a, (uint8_t)b)) {
                    tm_net_free(net);
                    fprintf(stderr, "tm: circuit mismatch\n");
                    return 1;
                }
            }
        }
        rc = tm_net_write_path(net, path);
        printf("gates %d\n", tm_net_ngates(net));
        tm_net_free(net);
        return map_err(rc);
    }
    return usage("unknown gf subcommand");
}

static int cmd_rev(int argc, char **argv)
{
    if (argc < 1)
        return usage("rev needs a subcommand");
    if (strcmp(argv[0], "check") == 0) {
        tm_instr gates[3] = {{TM_CX, 0, 1, 0}, {TM_CCX, 0, 1, 2}, {TM_CSWAP, 0, 1, 2}};
        unsigned g, s;
        for (g = 0; g < 3; g++) {
            unsigned seen = 0;
            for (s = 0; s < 8; s++) {
                unsigned v = 0, back = 0;
                if (tm_rev_apply(s, gates[g], 3, &v) != TM_OK)
                    return 1;
                if (seen & (1u << v))
                    return 1;
                seen |= 1u << v;
                if (tm_rev_apply(v, gates[g], 3, &back) != TM_OK || back != s)
                    return 1;
            }
        }
        printf("REVERSIBLE: CX, Toffoli, Fredkin verified as 3-bit permutations and involutions\n");
        return 0;
    }
    if (strcmp(argv[0], "apply") == 0) {
        const char *ws = flag(argc, argv, "--width");
        const char *ss = flag(argc, argv, "--state");
        const char *ps = flag(argc, argv, "--prog");
        uint64_t width = 3, state = 0;
        tm_instr prog[64];
        size_t n = 0;
        unsigned out = 0;
        char err[128];
        int rc;
        if (!ws || !ss || !ps)
            return usage("rev apply requires --width, --state, and --prog");
        if (tm_parse_u64(ws, &width) != TM_OK || width < 1 || width > TM_REV_MAX_WIRES)
            return usage("bad --width");
        if (tm_parse_u64(ss, &state) != TM_OK)
            return usage("bad --state");
        rc = tm_rev_parse(ps, prog, 64, &n, (unsigned)width, err, sizeof err);
        if (rc != TM_OK) {
            fprintf(stderr, "tm: %s\n", err[0] ? err : tm_strerror(rc));
            return rc == TM_ERR_PARSE ? 2 : map_err(rc);
        }
        rc = tm_rev_eval((unsigned)state, prog, n, (unsigned)width, &out);
        if (rc != TM_OK)
            return map_err(rc);
        printf("%u\n", out);
        return 0;
    }
    return usage("unknown rev subcommand");
}

static int cmd_qasm(int argc, char **argv)
{
    if (argc < 1)
        return usage("qasm needs a subcommand");
    if (strcmp(argv[0], "bell") == 0 || strcmp(argv[0], "reversible") == 0) {
        const char *path = flag(argc, argv, "--out");
        const char *text;
        int rc;
        if (!path)
            return usage("qasm write requires --out");
        text = argv[0][0] == 'b' ? tm_qasm_bell_text() : tm_qasm_reversible_text();
        rc = tm_write_text(path, text);
        if (rc != TM_OK)
            return map_err(rc);
        printf("wrote %s\n", path);
        return 0;
    }
    if (strcmp(argv[0], "sim") == 0) {
        tm_amp psi[1 << TM_QASM_MAX_QUBITS];
        int nq = 0, i, N;
        char err[160];
        int rc;
        double norm = 0;
        if (argc < 2)
            return usage("qasm sim FILE");
        rc = tm_qasm_sim_path(argv[1], psi, 1 << TM_QASM_MAX_QUBITS, &nq, err, sizeof err);
        if (rc != TM_OK) {
            fprintf(stderr, "tm: %s\n", err[0] ? err : tm_strerror(rc));
            return map_err(rc);
        }
        N = 1 << nq;
        printf("nqubits %d\n", nq);
        for (i = 0; i < N; i++) {
            double p = psi[i].re * psi[i].re + psi[i].im * psi[i].im;
            int b;
            norm += p;
            printf("prob ");
            for (b = nq - 1; b >= 0; b--)
                putchar(((i >> b) & 1) ? '1' : '0');
            printf(" %.12f\n", p);
        }
        printf("norm %.12f\n", norm);
        printf("note pre-measurement amplitudes; this simulator does not sample collapse\n");
        return 0;
    }
    return usage("unknown qasm subcommand");
}

static int parse_csv_row(const char *s, double *out, int cap, int *n)
{
    const char *p = s;
    int k = 0;
    if (!s || !*s)
        return -1;
    while (*p) {
        char *end = NULL;
        double v;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0' || *p == ';')
            return -1;
        errno = 0;
        v = strtod(p, &end);
        if (end == p || errno != 0 || !isfinite(v))
            return -1;
        if (k >= cap)
            return -1;
        out[k++] = v;
        p = end;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == '\0' || *p == ';')
            break;
        return -1;
    }
    *n = k;
    return k > 0 ? 0 : -1;
}

static int cmd_analog(int argc, char **argv)
{
    const char *gs, *is;
    double rows[TM_ANALOG_MAX][TM_ANALOG_MAX];
    double I[TM_ANALOG_MAX], V[TM_ANALOG_MAX], G[TM_ANALOG_MAX * TM_ANALOG_MAX];
    char buf[512];
    char *save = NULL;
    char *tok;
    int nrows = 0, width = -1, ni = 0, r, c;
    double residual = 0;
    int rc;
    if (argc < 1 || strcmp(argv[0], "solve") != 0)
        return usage("analog solve --g ROWS --i VALUES");
    gs = flag(argc, argv, "--g");
    is = flag(argc, argv, "--i");
    if (!gs || !is)
        return usage("analog solve requires --g and --i");
    if (strlen(gs) >= sizeof buf)
        return usage("matrix text too long");
    memcpy(buf, gs, strlen(gs) + 1);
    tok = strtok_r(buf, ";", &save);
    while (tok) {
        int n = 0;
        if (nrows >= TM_ANALOG_MAX)
            return usage("too many rows");
        if (parse_csv_row(tok, rows[nrows], TM_ANALOG_MAX, &n) != 0)
            return usage("malformed conductance row");
        if (width < 0)
            width = n;
        if (n != width)
            return usage("ragged conductance matrix");
        nrows++;
        tok = strtok_r(NULL, ";", &save);
    }
    if (nrows != width || width < 1)
        return usage("conductance matrix must be square");
    if (parse_csv_row(is, I, TM_ANALOG_MAX, &ni) != 0 || ni != width)
        return usage("current vector length must match the matrix");
    for (r = 0; r < width; r++)
        for (c = 0; c < width; c++)
            G[r * width + c] = rows[r][c];
    rc = tm_analog_solve(G, I, width, V, &residual);
    if (rc != TM_OK)
        return map_err(rc);
    printf("V");
    for (r = 0; r < width; r++)
        printf(" %.12f", V[r]);
    printf("\nresidual %.6e\n", residual);
    return 0;
}

static int cmd_smt(int argc, char **argv)
{
    if (argc < 1)
        return usage("smt needs a subcommand");
    if (strcmp(argv[0], "involution") == 0) {
        const char *path = flag(argc, argv, "--out");
        int rc;
        if (!path)
            return usage("smt involution requires --out");
        rc = tm_write_text(path, tm_smt_involution_text());
        if (rc != TM_OK)
            return map_err(rc);
        printf("wrote %s\n", path);
        return 0;
    }
    if (strcmp(argv[0], "check") == 0) {
        unsigned models = 99;
        if (tm_smt_involution_models(&models) != TM_OK)
            return 1;
        if (models == 0)
            printf("unsat\n");
        else
            printf("sat %u\n", models);
        printf("note native reading of this fixed 8-state query, not an external solver proof\n");
        return models == 0 ? 0 : 1;
    }
    return usage("unknown smt subcommand");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        help(stderr);
        return 2;
    }
    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        help(stdout);
        return 0;
    }
    if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("tm %s\n", TM_VERSION);
        return 0;
    }
    if (strcmp(argv[1], "self-check") == 0)
        return cmd_self(argc - 2, argv + 2);
    if (strcmp(argv[1], "oracle") == 0)
        return tm_oracle_write(stdout) == TM_OK ? 0 : 3;
    if (strcmp(argv[1], "bench") == 0)
        return tm_bench_run(stdout) == TM_OK ? 0 : 1;
    if (strcmp(argv[1], "dag") == 0)
        return cmd_dag(argc - 2, argv + 2);
    if (strcmp(argv[1], "gf") == 0)
        return cmd_gf(argc - 2, argv + 2);
    if (strcmp(argv[1], "rev") == 0)
        return cmd_rev(argc - 2, argv + 2);
    if (strcmp(argv[1], "qasm") == 0)
        return cmd_qasm(argc - 2, argv + 2);
    if (strcmp(argv[1], "analog") == 0)
        return cmd_analog(argc - 2, argv + 2);
    if (strcmp(argv[1], "smt") == 0)
        return cmd_smt(argc - 2, argv + 2);
    fprintf(stderr, "tm: unknown command '%s'\n", argv[1]);
    return 2;
}
