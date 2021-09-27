#!/usr/bin/env bash

# Device IBM-Tokyo
# A 20-qubit IBM device
# Qubits are arranged like so:
#    0 --  1 --  2 --  3 --  4
#    |     |     |     |  X  |
#    5 --  6 --  7 --  8 --  9
#    |  X  |     |  X  |
#   10 -- 11 -- 12 -- 13 -- 14
#    |     |  X        |  X  |
#   15 -- 16 -- 17    18    19

staq_device_generator graph -n 20 --name "20 qubit IBM Tokyo device" -u 0 1 -u 0 5 -u 1 2 -u 1 6 -u 2 3 -u 2 7 -u 3 4 -u 3 8 -u 3 9 -u 4 8 -u 4 9 -u 5 6 -u 5 10 -u 5 11 -u 6 7 -u 6 10 -u 6 11 -u 7 8 -u 7 12 -u 7 13 -u 8 9 -u 8 12 -u 8 13 -u 10 11 -u 10 15 -u 11 12 -u 11 16 -u 11 17 -u 12 13 -u 12 16 -u 13 14 -u 13 18 -u 13 19 -u 14 18 -u 14 19 -u 15 16 -u 16 17
