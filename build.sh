#!/bin/bash
set -e

echo "Building Cerridwen Framework..."

mkdir -p build
cd build

# Use GCC 16 explicitly if available, otherwise default to g++
if command -v g++-16 &> /dev/null; then
    export CXX=g++-16
    export CC=gcc-16
fi

# We delegate the actual build and WASM generation to the specialized script
cd ..
./tests/build_wasm.sh

cd build
echo "Running tests..."
ctest --output-on-failure
