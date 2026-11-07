OPENQASM 3.0;
include "stdgates.inc";
qubit[3] q;
cx q[0], q[1];
ccx q[0], q[1], q[2];
cswap q[0], q[1], q[2];
