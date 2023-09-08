OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];

rz(0.3) q[0];
rx(3/10) q[0];
ry(3*100/10/100) q;
