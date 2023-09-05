OPENQASM 2.0;
include "qelib1.inc";

qreg q1[4];
qreg q2[4];

cx q1[0],q1[1];
cx q1[1],q1[2];
cx q1[2],q1[3];
cx q1[3],q1[0];

t q1[1];

cx q1[0],q2[0];
cx q1[1],q2[1];
cx q1[2],q2[2];
cx q1[3],q2[3];

cx q2[0],q2[1];
cx q2[1],q2[2];
cx q2[2],q2[3];
cx q2[3],q2[0];
