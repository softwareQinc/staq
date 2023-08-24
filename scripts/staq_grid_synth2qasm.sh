#!/bin/sh

# staq_grid_synth2qasm
#
# Converts an Rz(angle) rotation produced by staq_grid_synth (C++) into an
# OpenQASM 2 circuit

# $1 - angle, in units of PI
# $2 - precision (10^-precision)
# $3 - cache file, optional

if [ $# -lt 2 ]; then
    printf "Usage: %s <angle (x PI)> <precision (10^-p)> [cache file]\n" "$0"
    exit 1
fi

echo OPENQASM 2.0\;
echo include \"qelib1.inc\"\;
echo
echo qreg q[1]\;
echo

if [ $# -gt 2 ]; then
    staq_grid_synth -t "$1" -p "$2" -r "$3" | tr '[:upper:]' '[:lower:]' | tr -d 'i' | tr ' ' '\n' | sed '/w/d' | sed '/^$/d' | awk '{print $1, "q[0];";}'
else
    staq_grid_synth -t "$1" -p "$2" | tr '[:upper:]' '[:lower:]' | tr -d 'i' | tr ' ' '\n' | sed '/w/d' | sed '/^$/d' | awk '{print $1, "q[0];";}'
fi
