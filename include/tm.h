/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * Topology Machine public API.
 * Node indices are nonnegative. Index 0 is a valid node. Errors are negative.
 */
#ifndef TM_H
#define TM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TM_VERSION "1.0.0"
#define TM_GF_POLY 0x11bu   /* x^8 + x^4 + x^3 + x + 1 */
#define TM_GF_REDUCE 0x1bu  /* low 8 bits of the polynomial */
#define TM_DAG_MAX_NODES 4096
#define TM_DAG_MAX_INPUTS 64
#define TM_ANALOG_MAX 16
#define TM_QASM_MAX_QUBITS 4
#define TM_REV_MAX_WIRES 16
#define TM_NET_MAX_GATES 8192
#define TM_NET_NAME 32
#define TM_ANALOG_PIVOT 1e-12

enum {
    TM_OK = 0,
    TM_ERR_RANGE = -1,
    TM_ERR_PARSE = -2,
    TM_ERR_CAPACITY = -3,
    TM_ERR_SINGULAR = -4,
    TM_ERR_IO = -5,
    TM_ERR_DOMAIN = -6,
    TM_ERR_NOMEM = -7
};

const char *tm_strerror(int err);
int tm_parse_u64(const char *s, uint64_t *out);
int tm_mkdir(const char *path);
int tm_write_text(const char *path, const char *text);

typedef enum {
    TM_OP_ZERO = 0,
    TM_OP_ONE = 1,
    TM_OP_INPUT = 2,
    TM_OP_NOT = 3,
    TM_OP_AND = 4,
    TM_OP_OR = 5,
    TM_OP_XOR = 6
} tm_op;

typedef struct tm_dag tm_dag;

tm_dag *tm_dag_new(void);
void tm_dag_free(tm_dag *d);
int tm_dag_n(const tm_dag *d);
int tm_dag_op(const tm_dag *d, int node, tm_op *op, int *a, int *b);
int tm_dag_input(tm_dag *d, int bit);
int tm_dag_not(tm_dag *d, int a);
int tm_dag_and(tm_dag *d, int a, int b);
int tm_dag_or(tm_dag *d, int a, int b);
int tm_dag_xor(tm_dag *d, int a, int b);
int tm_dag_eval(const tm_dag *d, int root, uint64_t input, int *bit);

typedef struct tm_expr tm_expr;

int tm_expr_parse(const char *s, tm_expr **out, char *err, size_t errlen);
void tm_expr_free(tm_expr *e);
int tm_expr_eval(const tm_expr *e, uint64_t input, int *bit);
int tm_expr_inputs(const tm_expr *e);
int tm_expr_to_dag(tm_dag *d, const tm_expr *e, int *node);

uint8_t tm_gf_mul(uint8_t a, uint8_t b);
uint8_t tm_gf_mul_rec(uint8_t a, uint8_t b);
uint8_t tm_gf_pow(uint8_t a, unsigned exp);
int tm_gf_inv(uint8_t a, uint8_t *inv);

typedef struct tm_net tm_net;

void tm_net_free(tm_net *n);
int tm_net_gf256_mul(tm_net **out);
int tm_net_ngates(const tm_net *n);
int tm_net_ninputs(const tm_net *n);
int tm_net_noutputs(const tm_net *n);
int tm_net_write(const tm_net *n, FILE *fp);
int tm_net_write_path(const tm_net *n, const char *path);
int tm_net_read(FILE *fp, tm_net **out, char *err, size_t errlen);
int tm_net_read_path(const char *path, tm_net **out, char *err, size_t errlen);
int tm_net_eval(const tm_net *n, uint64_t inputs, uint64_t *outputs);

typedef enum { TM_CX = 1, TM_CCX = 2, TM_CSWAP = 3 } tm_gate_op;

typedef struct {
    tm_gate_op op;
    unsigned a, b, c;
} tm_instr;

int tm_rev_apply(unsigned state, tm_instr g, unsigned width, unsigned *out);
int tm_rev_eval(unsigned state, const tm_instr *prog, size_t n,
                unsigned width, unsigned *out);
int tm_rev_parse(const char *text, tm_instr *out, size_t cap, size_t *n,
                 unsigned width, char *err, size_t errlen);

typedef struct {
    double re, im;
} tm_amp;

int tm_q_bell(tm_amp psi[4]);
int tm_qasm_write_bell(FILE *fp);
int tm_qasm_write_reversible(FILE *fp);
const char *tm_qasm_bell_text(void);
const char *tm_qasm_reversible_text(void);
int tm_qasm_sim_memory(const char *text, size_t len, tm_amp *psi, int cap,
                       int *nqubits, char *err, size_t errlen);
int tm_qasm_sim_path(const char *path, tm_amp *psi, int cap, int *nqubits,
                     char *err, size_t errlen);

int tm_analog_solve(const double *G, const double *I, int n, double *V,
                    double *residual);

int tm_smt_write_involution(FILE *fp);
const char *tm_smt_involution_text(void);
int tm_smt_involution_models(unsigned *nmodels);
unsigned tm_smt_cx(unsigned x);
unsigned tm_smt_ccx(unsigned x);
unsigned tm_smt_fw(unsigned x);

int tm_oracle_write(FILE *fp);
const char *tm_oracle_text(void);

int tm_selfcheck(FILE *out, const char *outdir);

int tm_bench_run(FILE *out);

#endif
