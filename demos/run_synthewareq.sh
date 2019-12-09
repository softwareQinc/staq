#!/bin/bash

echo "Original:"
../build/synthewareQ -S -f resources $1
echo "Optimized:"
time ../build/synthewareQ -S -O2 -f resources $1
echo "Mapped:"
time ../build/synthewareQ -S -O2 -m -d tokyo -l bestfit -M steiner -f resources $1
