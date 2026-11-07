/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    long sz;
    char *buf;
    if (!fp)
        return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    sz = ftell(fp);
    rewind(fp);
    buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    buf[sz] = '\0';
    fclose(fp);
    return buf;
}

static double prob(const tm_amp *a)
{
    return a->re * a->re + a->im * a->im;
}

int main(void)
{
    tm_amp psi[16];
    int nq = 0;
    char err[128];
    char *gold;
    const char *xgate =
        "OPENQASM 3.0;\n"
        "qubit[1] q;\n"
        "x q[0];\n";
    const char *mid =
        "OPENQASM 3.0;\n"
        "qubit[1] q;\n"
        "bit[1] c;\n"
        "c[0] = measure q[0];\n"
        "x q[0];\n";
    const char *bad = "OPENQASM 2.0;\nqubit[1] q;\n";
    double k = 1.0 / sqrt(2.0);
    CHECK(tm_q_bell(psi) == TM_OK);
    CHECK(fabs(psi[0].re - k) < 1e-12 && fabs(psi[0].im) < 1e-12);
    CHECK(fabs(psi[3].re - k) < 1e-12 && fabs(psi[3].im) < 1e-12);
    CHECK(prob(&psi[1]) < 1e-18 && prob(&psi[2]) < 1e-18);
    gold = read_file("tests/golden/bell.qasm");
    CHECK(gold != NULL);
    CHECK(strcmp(gold, tm_qasm_bell_text()) == 0);
    free(gold);
    gold = read_file("tests/golden/reversible.qasm");
    CHECK(gold != NULL);
    CHECK(strcmp(gold, tm_qasm_reversible_text()) == 0);
    free(gold);
    CHECK(tm_qasm_sim_memory(tm_qasm_bell_text(), strlen(tm_qasm_bell_text()),
                             psi, 16, &nq, err, sizeof err) == TM_OK);
    CHECK(nq == 2);
    CHECK(fabs(prob(&psi[0]) - 0.5) < 1e-12);
    CHECK(fabs(prob(&psi[3]) - 0.5) < 1e-12);
    CHECK(tm_qasm_sim_memory(xgate, strlen(xgate), psi, 16, &nq, err, sizeof err) == TM_OK);
    CHECK(nq == 1);
    CHECK(fabs(prob(&psi[1]) - 1.0) < 1e-12);
    CHECK(tm_qasm_sim_memory(mid, strlen(mid), psi, 16, &nq, err, sizeof err) == TM_ERR_PARSE);
    CHECK(tm_qasm_sim_memory(bad, strlen(bad), psi, 16, &nq, err, sizeof err) == TM_ERR_PARSE);
    CHECK(tm_qasm_sim_memory("nope", 4, psi, 16, &nq, err, sizeof err) == TM_ERR_PARSE);
    CHECK(tm_qasm_sim_path("tests/golden/reversible.qasm", psi, 16, &nq, err, sizeof err) == TM_OK);
    CHECK(nq == 3);
    CHECK(fabs(prob(&psi[0]) - 1.0) < 1e-12);
    CHECK(tm_qasm_sim_path("tests/golden/missing.qasm", psi, 16, &nq, err, sizeof err) == TM_ERR_IO);
    return check_report("quantum");
}
