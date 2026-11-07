# Changelog

## 1.0.0

First maintained tree built from the Topology Machine starter.

- Boolean DAG with structural sharing, commutative operands, constant folding, and `XOR(x,x) = 0`. The starter returned the constant ONE for that case.
- GF(2^8) under the AES polynomial, iterative and recursive products, inverses, and a 223-gate AND/XOR netlist checked on all 65536 inputs.
- CX, Toffoli, and Fredkin, plus the starter QASM and QF_BV texts.
- OpenQASM 3 subset simulator for the circuits this program writes, at most 4 qubits, without measurement sampling.
- Dense nodal solve, including the starter two-node network.
- Vendored Klong 20221212. `klong/oracle.kg` is executed and diffed with `tm oracle`.
- CLI, library, man page, install and uninstall, tests, sanitizers, and five recorded bench runs.
- The packet-capture daemon and the toy MAC from the accompanying notes are not included.
