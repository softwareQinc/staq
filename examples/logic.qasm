OPENQASM 2.0;
include "qelib1.inc";

qreg q[4];

oracle adder a1,a2,b1,b2,c { "tmp.v" }
