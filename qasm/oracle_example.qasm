OPENQASM 2.0;
include "qelib1.inc";

oracle tof4 a,b,c,d,e { "toffoli_4.v" }

qreg q[5];

tof4 q[0],q[0],q[2],q[3],q[4];
