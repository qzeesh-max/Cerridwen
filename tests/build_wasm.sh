#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." >/dev/null 2>&1 && pwd)"
cd "$DIR"

cd build
if command -v g++-16 &> /dev/null; then
    export CXX=g++-16
    export CC=gcc-16
fi
cmake ..
# First build the generator tool
cmake --build . --target generate_plugin_tool
cd ..

echo "Running the generator to create plugin_generated.cpp and plugin_trampoline.hpp..."
./build/tests/generate_plugin_tool

cd build
# Now build everything else (tests) which depend on the generated files
cmake --build . -j$(sysctl -n hw.ncpu || echo 4)
cd ..



echo "Compiling WASM plugin..."
/Users/zeeshanqazi/.gemini/antigravity/scratch/wasm-terminal/emsdk/upstream/emscripten/em++ \
    tests/plugin.cpp tests/plugin_generated.cpp \
    -o build/tests/plugin.wasm \
    -s STANDALONE_WASM=1 \
    -mno-bulk-memory \
    -Wl,--allow-undefined \
    --no-entry \
    -O0 \
    -Iinclude

echo "WASM plugin built successfully at build/tests/plugin.wasm"
