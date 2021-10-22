OPENQASM 2.0;
include "qelib1.inc";

gate tof3 a,b,c,d {
  dirty ancilla e[1];
  ccx a,b,e;
  ccx c,e,d;
  ccx a,b,e;
  ccx c,e,d;
}


qreg q[4];
qreg r[1];

tof3 q[0],q[1],q[2],q[3];
