#!/usr/bin/env bash

# Device Rigetti-Agave
# The Rigetti 8 qubit Agave qpu
# Qubits are arranged like so:
#   1 -- 0 -- 7
#   |         |
#   2         6
#   |         |
#   3 -- 4 -- 5

staq_device_generator graph -n 8 --name "Rigetti Agave" -f 0 0.957 -f 1 0.951 -f 2 0.982 -f 3 0.97 -f 4 0.969 -f 5 0.962 -f 6 0.969 -f 7 0.932 -U 0 1 0.92 -U 0 7 0.91 -U 1 2 0.91 -U 2 3 0.82 -U 3 4 0.87 -U 4 5 0.67 -U 5 6 0.93 -U 6 7 0.93

# If fidelities aren't needed, one can simply do
# staq_device_generator --circle 8
