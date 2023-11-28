OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];
qreg a[2];

tdg a[0];
crz(2) a[1], q[0];
