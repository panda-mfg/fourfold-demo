#!/bin/bash
set -euo pipefail

# Build the code
cmake -S . -B build
cmake --build build

# Create Kempe chain files and Coloring files
./build/a.out -k 9 -c 18
