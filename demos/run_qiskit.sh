#!/bin/bash

echo "Original:"
python qiskit_base.py $1
echo "Optimized:"
time python qiskit_optimize.py $1
echo "Mapped:"
time python qiskit_map.py $1
