#!/usr/bin/env bash

# Device IBM-Singapore
# A 20-qubit IBM device
# Qubits are arranged like so:
#    0 --  1 --  2 --  4 --  3
#          |           |
#   12 -- 11 -- 10 --  5 --  6
#    |           |           |
#   13 -- 14 --  9 --  8 --  7
#          |           |
#   15 -- 16 -- 17 -- 18 -- 19

staq_device_generator graph -n 20 --name "20 qubit IBM Singapore device" -u 0 1 -u 1 2 -u 1 11 -u 2 4 -u 3 4 -u 4 5 -u 5 6 -u 5 10 -u 6 7 -u 7 8 -u 8 9 -u 8 18 -u 9 10 -u 9 14 -u 10 11 -u 11 12 -u 12 13 -u 13 14 -u 14 16 -u 15 16 -u 16 17 -u 17 18 -u 18 19
