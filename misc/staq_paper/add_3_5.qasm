// Try with "staq -S -O2 -m -d aspen-4 -l bestfit -M steiner add_3_5.qasm"

OPENQASM 2.0;
include "qelib1.inc";

oracle adder a0,a1,a2,a3,b0,b1,b2,b3,c0,c1,c2,c3 { "adder_4.v" }

qreg a[4];
qreg b[4];
qreg c[4];
creg result[4];

// a = 3
x a[0];
x a[1];

// b = 5
x b[0];
x b[2];

adder a[0],a[1],a[2],a[3],b[0],b[1],b[2],b[3],c[0],c[1],c[2],c[3];

// measure
measure c -> result;
