#!/bin/bash
set -e

# Source emsdk
source /opt/emsdk/emsdk_env.sh

# Note: GCC 16 is not yet in the official repos, so on Ubuntu you might have to build it from source
# For the sake of this environment we assume standard G++ has enough C++26 support or you mount your own
export CXX=g++-14
export CC=gcc-14

# Execute the passed arguments (e.g. bash tests/build_wasm.sh && build/tests/cerridwen_tests)
exec "$@"
