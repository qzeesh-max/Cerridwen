#!/bin/bash
set -e

# ==============================================================================
# Cerridwen Framework - Unit Test Runner
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Default configuration
BUILD_DIR="$PROJECT_ROOT/build"
TARGET="cerridwen_unit_tests"
FILTER=""
VERBOSE=false
CLEAN=false
ENABLE_COVERAGE=false
RUN_ALL=false

print_usage() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS]

Runs the Cerridwen unit test suite.

Options:
  -a, --all               Run full test suite (unit tests + trampoline + e2e tests)
  -u, --unit-only         Run standalone unit tests only (default: cerridwen_unit_tests)
  -f, --filter PATTERN    Filter tests using GoogleTest pattern (e.g. --filter="*Serializer*")
  -v, --verbose           Show detailed test execution output
  -c, --coverage          Compile with code coverage instrumentation enabled
  --clean                 Remove existing build directory before running
  -h, --help              Show this help message

Examples:
  ./scripts/run_unit_tests.sh
  ./scripts/run_unit_tests.sh --all
  ./scripts/run_unit_tests.sh --filter="*Storage*"
  ./scripts/run_unit_tests.sh --coverage
EOF
}

# Parse command line options
while [[ $# -gt 0 ]]; do
    case "$1" in
        -a|--all)
            RUN_ALL=true
            TARGET="cerridwen_tests"
            shift
            ;;
        -u|--unit-only)
            RUN_ALL=false
            TARGET="cerridwen_unit_tests"
            shift
            ;;
        -f|--filter)
            FILTER="$2"
            shift 2
            ;;
        --filter=*)
            FILTER="${1#*=}"
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -c|--coverage)
            ENABLE_COVERAGE=true
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

echo "======================================================================"
echo "           Cerridwen Framework - Unit Test Runner                     "
echo "======================================================================"
echo " Target : $TARGET"
[ -n "$FILTER" ] && echo " Filter : $FILTER"
echo " Coverage : $ENABLE_COVERAGE"
echo "======================================================================"

# Handle clean build
if [ "$CLEAN" = true ]; then
    echo "[+] Cleaning build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

# Compiler setup (prefer GCC 16 for C++26 reflection support)
if command -v g++-16 &> /dev/null; then
    export CXX="g++-16"
    export CC="gcc-16"
fi

# Configure CMake
CMAKE_ARGS=("-DCMAKE_BUILD_TYPE=Debug")
if [ "$ENABLE_COVERAGE" = true ]; then
    CMAKE_ARGS+=("-DENABLE_COVERAGE=ON")
fi

echo "[+] Configuring CMake..."
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" "${CMAKE_ARGS[@]}"

# Build the test target
echo "[+] Building $TARGET..."
NPROC=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

# If running all tests, we need the WASM plugin built
if [ "$RUN_ALL" = true ]; then
    echo "[+] Building plugin WASM and generator dependencies..."
    ./tests/build_wasm.sh
fi

cmake --build "$BUILD_DIR" --target "$TARGET" -j"$NPROC"

# Run tests
TEST_EXE="$BUILD_DIR/tests/$TARGET"
if [ ! -x "$TEST_EXE" ]; then
    echo "[-] Error: Test binary not found at $TEST_EXE"
    exit 1
fi

GTEST_ARGS=()
if [ -n "$FILTER" ]; then
    GTEST_ARGS+=("--gtest_filter=$FILTER")
fi

if [ "$VERBOSE" = false ]; then
    GTEST_ARGS+=("--gtest_brief=1")
fi

echo "[+] Running tests: $TEST_EXE ${GTEST_ARGS[*]}"
echo "----------------------------------------------------------------------"

"$TEST_EXE" "${GTEST_ARGS[@]}"
TEST_STATUS=$?

echo "----------------------------------------------------------------------"
if [ $TEST_STATUS -eq 0 ]; then
    echo "[✓] All tests completed successfully!"
else
    echo "[✗] Tests failed with exit code $TEST_STATUS"
fi

exit $TEST_STATUS
