OPENQASM 2.0;
include "qelib1.inc";

qreg x[4];
qreg y[4];

cx x[0],x[1];
cx x[1],x[2];
cx x[2],x[3];
cx x[3],x[0];

t x[1];

cx x[0],y[0];
cx x[1],y[1];
cx x[2],y[2];
cx x[3],y[3];

cx y[0],y[1];
cx y[1],y[2];
cx y[2],y[3];
cx y[3],y[0];
