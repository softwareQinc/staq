#!/usr/bin/env bash

# Device Rigetti-Aspen-4
# A 16-qubit Rigetti lattice
# Qubits are arranged like so:
#     4 -- 3         12 -- 11
#    /      \       /        \
#   5        2 -- 13          10
#   |        |     |          |
#   6        1 -- 14          9
#    \      /       \        /
#     7 -- 0         15 -- 8

staq_device_generator graph -n 16 --name "16-qubit Rigetti lattice" -u 0 1 -u 0 7 -u 1 2 -u 1 14 -u 2 3 -u 2 13 -u 3 4 -u 4 5 -u 5 6 -u 6 7 -u 8 9 -u 8 15 -u 9 10 -u 10 11 -u 11 12 -u 12 13 -u 13 14 -u 14 15
