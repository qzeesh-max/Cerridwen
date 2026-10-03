#!/bin/bash
set -e

# Build the docker image
echo "Building Docker image..."
docker build -t cerridwen_builder .

# Run the docker image, mounting the current directory
echo "Running tests in Docker container..."
docker run --rm -v "$(pwd):/app" cerridwen_builder bash -c "./tests/build_wasm.sh && ./build/tests/cerridwen_tests"
