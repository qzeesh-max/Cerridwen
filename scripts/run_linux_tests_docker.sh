#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( dirname "$DIR" )"

echo "Building Docker image for Cerridwen..."
docker build -t cerridwen-test-env -f "$PROJECT_ROOT/scripts/Dockerfile" "$PROJECT_ROOT"

echo "Running tests in Docker..."
docker run --rm cerridwen-test-env
