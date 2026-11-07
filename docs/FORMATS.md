# Formats

Topology Machine text is ASCII. Decimal points are the C-locale point. Integers on the CLI are decimal or `0x` / `0X` hex. A leading sign, a trailing character, and `0b` binary are rejected.

## Boolean expressions

```text
expr = "0" | "1" | "i" DIGITS
     | "not" "(" expr ")"
     | "and" "(" expr "," expr ")"
     | "or"  "(" expr "," expr ")"
     | "xor" "(" expr "," expr ")"
```

`i0` is bit 0, the least significant bit of `--input`. Indices `0` through `63` are accepted. The truth-table label prints bit `n-1` on the left and bit 0 on the right. A DAG evaluation and a separate tree evaluation must agree; the tree does not fold, and the DAG does.

## Netlist 1

```text
# comment
tm-netlist 1
input <name>
const <0|1> <name>
and <a> <b> <name>
xor <a> <b> <name>
not <a> <name>
output <name> <gate>
```

Names match `[A-Za-z_][A-Za-z0-9_]*` and are at most 31 characters. A reference must name an earlier gate. Duplicate names are errors. Evaluation walks the file in order. Input bit 0 of `tm_net_eval` is the first `input` line. Output bit 0 is the first `output` line.

For `examples/gf256.tmnl`, inputs are `a0`..`a7` then `b0`..`b7`, and outputs are `y0`..`y7`. Pack `a` in the low byte and `b` in the next byte. The low byte of the result is the field product.

The multiplier is built by schoolbook convolution of the two degree-7 polynomials, then by folding degrees 14 down through 8 with `x^8 = x^4 + x^3 + x + 1`. Identical AND gates are shared. Each output is a left-associated XOR of its terms.

## Reversible programs

Gates are separated by whitespace, commas, or semicolons.

```text
cx <control> <target>
ccx <control1> <control2> <target>
cswap <control> <wire1> <wire2>
```

Wires are decimal, distinct, and strictly less than the width. Width is 1..16. State bits outside the width are rejected.

## OpenQASM subset

The files written by `tm qasm bell` and `tm qasm reversible` match `tests/golden/` byte for byte, including the final newline. The simulator also accepts the statements listed in the README. Qubit 0 is the least significant state index. Probability lines look like `prob 00 0.500000000000`, with the highest qubit on the left of the label.

## SMT-LIB query

`tests/golden/reversible.smt2` is a QF_BV query over one 3-bit variable `s`. It asks whether `cx`, `ccx`, or `fw` (Fredkin) fails `f(f(s)) = s`. The names `a`, `b`, and `c` are unused extracts kept so the file matches the starter. `tm smt check` evaluates those three formulas on `s = 0..7`.

## Oracle

`tm oracle` and `klong/oracle.kg` print:

```text
dot 13
scan 1 1 2 3
gf 87 131 193
gf 83 1 83
gf 83 0 0
gf 255 2 229
gf 128 2 27
gf 1 1 1
gf 2 2 4
rev cx 5 7
rev ccx 3 7
rev cswap 5 3
bool and 0 1 0
bool or 0 1 1
bool xor 1 1 0
bool not 0 1
```

`gf A B P` means the field product of decimal bytes `A` and `B` is `P`. `rev` lines are the 3-bit demo gates: CX 0→1 on state 5, Toffoli 0,1→2 on state 3, Fredkin 0 swapping 1 and 2 on state 5.

## Bench

```text
# run 1
name ops seconds ns_per_op
gf_mul 2097152 0.008850907 4.220
```

`seconds` is `CLOCK_MONOTONIC` around the loop. `ns_per_op` is `seconds * 1e9 / ops`. A zero elapsed time would print `0` rather than divide by zero; the recorded runs did not hit that case.
