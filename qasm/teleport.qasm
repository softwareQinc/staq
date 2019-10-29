OPENQASM 2.0;
include "qelib1.inc";

gate bell x,y {
  h x;
  cx x,y;
}

qreg q[1];
qreg anc[2];
creg c0[1];
creg c1[1];

bell anc[0],anc[1];
cx q,anc[0];
h q;
measure q -> c0[0];
measure anc[0] -> c1[0];
//if(c0==1) z anc[1];
//if(c1==1) x anc[1];
