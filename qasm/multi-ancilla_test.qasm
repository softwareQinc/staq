OPENQASM 2.0;
include "qelib1.inc";

gate foo a {
  ancilla b[1];
  ancilla c[1];
  cx a,b[0];
  cx a,c[0];
}

qreg x[1];
foo x[0];
