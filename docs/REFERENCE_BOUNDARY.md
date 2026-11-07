# Reference boundary

The Topology Machine starter is the C program, Makefile, array script, and generated QASM and SMT files in the uploaded `topology_machine` archive. That program is the foundation of this tree: the same Bell and reversible QASM bytes, the same SMT-LIB query, the same AES product `0x57 * 0x83 = 0xc1`, the same three-bit gates, and the same nodal solution `V = (0.4, 0.2)`.

One starter rule was wrong and is corrected. `XOR(x,x)` returned node index 1. Index 1 is the constant ONE. The identity is 0, which is node index 0. `tests/test_dag` fails if that regresses.

A separate note file came with the archive. It is reference material, not a specification this repository implements. It contains three kinds of text:

1. GF(2^8) sketches: the iterative product, a recursive product of depth 8, and a symbolic AND/XOR multiplier. Those ideas are implemented. `tm_gf_mul` and `tm_gf_mul_rec` are checked against each other on all 65536 pairs. The netlist is generated in C. There is no Wolfram call and no `popen` of an external computer-algebra system.
2. A duplicated AF_PACKET capture loop: raw sockets, promiscuous mode, a shared ring, and an interface-down action. That code is not in this repository. The program does not open a packet socket, does not read a network interface, and does not change interface flags.
3. An `O_APPEND` log described as WORM storage, and a “MAC” built from GF multiplies and a toy curve mix. Those constructions are not implemented and are not secure. `O_APPEND` does not stop a later writer from changing earlier bytes, and the mixing function was not a message-authentication code. Shipping either of them as a security feature would be a false claim.

Those exclusions conflict with any reading of the notes that asks for a live capture daemon. The conflict is resolved by leaving that daemon out. The mathematical models do not need it.

Klong in the starter was an illustrative script, not an executed toolchain. This tree vendors Klong 20221212, runs the scripts, and diffs them with `tm oracle`.
