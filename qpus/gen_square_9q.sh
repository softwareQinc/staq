#!/usr/bin/env bash

# Device square_9q
# A 9 qubit square lattice
# Qubits are arranged like so:
#   0 -- 1 -- 2
#   |    |    |
#   3 -- 4 -- 5
#   |    |    |
#   6 -- 7 -- 8

staq_device_generator --rectangle 3
