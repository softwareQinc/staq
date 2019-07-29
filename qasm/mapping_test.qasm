OPENQASM 2.0;
include "qelib1.inc";

qreg x[4];
qreg y[4];

CX x[0],x[1];
CX x[1],x[2];
CX x[2],x[3];
CX x[3],x[0];

U(0,0,pi/2) x[1];

CX x[0],y[0];
CX x[1],y[1];
CX x[2],y[2];
CX x[3],y[3];

CX y[0],y[1];
CX y[1],y[2];
CX y[2],y[3];
CX y[3],y[0];
