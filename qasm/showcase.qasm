OPENQASM 2.0;
include "qelib1.inc";

oracle tof4 a,b,c,d,e { "toffoli_4.v" }

qreg a[6];

tof4 a[0],a[1],a[2],a[3],a[4];
tof4 a[0],a[1],a[2],a[3],a[5];
