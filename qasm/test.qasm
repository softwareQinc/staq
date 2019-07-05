OPENQASM 2.0;
include "qelib1.inc";

qreg a[2];

t a[0];
x a[0];
tdg a[0]; 
x a[0];

h a[1];
t a[1];
h a[1];
h a[1];
t a[1];
h a[1];

U(pi,pi,pi) a;
