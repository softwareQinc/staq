OPENQASM 2.0;
include "qelib1.inc";

qreg q[4];

oracle adder u,v { "adder.v" }
