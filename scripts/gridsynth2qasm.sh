#!/bin/sh

# gridsynth2qasm
#
# Converts an Rz(angle) rotation produced by gridsynth (Haskell) into an
# OpenQASM 2 circuit

# $1 - angle, in units of PI
# $2 - precision (10^-precision)

if [ $# -lt 2 ]; then
    printf "Usage: %s <angle (x PI)> <precision (10^-p)>\n" "$0"
    exit 1
fi

echo OPENQASM 2.0\;
echo include \"qelib1.inc\"\;
echo
echo qreg q[1]\;
echo

gridsynth "$1*pi" "-d$2" | tr '[:upper:]' '[:lower:]' | tr -d 'i' | sed 's/[a-z]/&\n/g' | sed '/w/d' | sed '/^$/d' | awk '{print $1, "q[0];";}'
