OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];

gate cphase(theta) a,b {
  rz(theta/2) a;
  rz(theta/2) b;
  cx a,b;
  rz(-theta/2) b;
  cx a,b;
}

cphase(pi/4) q[0],q[1];
