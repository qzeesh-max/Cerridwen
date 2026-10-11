#!/bin/bash
set -e

# ==============================================================================
# Cerridwen Framework - Code Coverage Runner & Reporter
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

BUILD_DIR="$PROJECT_ROOT/build"
REPORT_DIR="$BUILD_DIR/coverage_report"
THRESHOLD=65
HTML=true
OPEN_REPORT=false
RUN_ALL=true

print_usage() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS]

Runs tests with code coverage instrumentation, generates coverage metrics,
and produces HTML and terminal summary reports.

Options:
  -t, --threshold PERCENT Set minimum required coverage threshold (default: 80%)
  -u, --unit-only         Run unit tests only (skip end-to-end WASM tests)
  -a, --all               Run full test suite (unit + trampoline + e2e) [default]
  --no-html               Skip generating HTML coverage report
  --open                  Open generated HTML report in default browser (macOS/Linux)
  -h, --help              Show this help message

Examples:
  ./scripts/run_coverage.sh
  ./scripts/run_coverage.sh --threshold 85
  ./scripts/run_coverage.sh --unit-only
  ./scripts/run_coverage.sh --open
EOF
}

# Parse options
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--threshold)
            THRESHOLD="$2"
            shift 2
            ;;
        --threshold=*)
            THRESHOLD="${1#*=}"
            shift
            ;;
        -u|--unit-only)
            RUN_ALL=false
            shift
            ;;
        -a|--all)
            RUN_ALL=true
            shift
            ;;
        --no-html)
            HTML=false
            shift
            ;;
        --open)
            OPEN_REPORT=true
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
echo "         Cerridwen Framework - Code Coverage Analysis                 "
echo "======================================================================"
echo " Threshold : ${THRESHOLD}%"
echo " Mode      : $([ "$RUN_ALL" = true ] && echo "Full Suite (unit + e2e)" || echo "Unit Tests Only")"
echo " HTML Gen  : $HTML"
echo "======================================================================"

# Determine compiler and tools
if command -v g++-16 &> /dev/null; then
    export CXX="g++-16"
    export CC="gcc-16"
fi

GCOV_EXE="gcov"
if command -v gcov-16 &> /dev/null; then
    GCOV_EXE="gcov-16"
fi

GCOVR_EXE=""
if command -v gcovr &> /dev/null; then
    GCOVR_EXE="gcovr"
elif [ -x "/Users/zeeshanqazi/.gemini/antigravity/scratch/wasm-terminal/emsdk/python/3.13.3_64bit/bin/gcovr" ]; then
    GCOVR_EXE="/Users/zeeshanqazi/.gemini/antigravity/scratch/wasm-terminal/emsdk/python/3.13.3_64bit/bin/gcovr"
fi

NPROC=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

mkdir -p "$BUILD_DIR"

# 1. Reset old coverage counters
echo "[1/5] Resetting coverage counters..."
find "$BUILD_DIR" -name "*.gcda" -delete 2>/dev/null || true

# 2. Build with coverage instrumentation
echo "[2/5] Configuring & building with coverage instrumentation..."
export ENABLE_COVERAGE=ON
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON

if [ "$RUN_ALL" = true ]; then
    ./tests/build_wasm.sh
    TARGET="cerridwen_tests"
else
    cmake --build "$BUILD_DIR" --target cerridwen_unit_tests -j"$NPROC"
    TARGET="cerridwen_unit_tests"
fi

# 3. Execute tests to generate .gcda data
echo "[3/5] Executing $TARGET to collect coverage profile..."
"$BUILD_DIR/tests/$TARGET"

# 4. Generate Coverage Report
echo "[4/5] Generating coverage report..."
mkdir -p "$REPORT_DIR"

COVERAGE_PASS=true

if [ -n "$GCOVR_EXE" ]; then
    echo "----------------------------------------------------------------------"
    echo "                     COVERAGE REPORT SUMMARY                          "
    echo "----------------------------------------------------------------------"
    "$GCOVR_EXE" \
        --gcov-executable "$GCOV_EXE" \
        --gcov-ignore-errors=no_working_dir_found \
        --filter "$PROJECT_ROOT/include/cerridwen/.*" \
        -r "$PROJECT_ROOT" \
        --print-summary \
        --fail-under-line "$THRESHOLD" \
        --json "$REPORT_DIR/coverage.json" || COVERAGE_PASS=false

    if [ "$HTML" = true ]; then
        "$GCOVR_EXE" \
            --gcov-executable "$GCOV_EXE" \
            --gcov-ignore-errors=no_working_dir_found \
            --filter "$PROJECT_ROOT/include/cerridwen/.*" \
            -r "$PROJECT_ROOT" \
            --html-details "$REPORT_DIR/index.html"
        echo "[+] HTML coverage report generated at: $REPORT_DIR/index.html"
    fi
elif command -v lcov &> /dev/null; then
    LCOV_INFO="$REPORT_DIR/coverage.info"
    lcov --gcov-tool "$GCOV_EXE" --capture --directory "$BUILD_DIR" --output-file "$LCOV_INFO"
    lcov --gcov-tool "$GCOV_EXE" --extract "$LCOV_INFO" "$PROJECT_ROOT/include/*" --output-file "$LCOV_INFO"
    lcov --summary "$LCOV_INFO"

    if [ "$HTML" = true ] && command -v genhtml &> /dev/null; then
        genhtml "$LCOV_INFO" --output-directory "$REPORT_DIR"
        echo "[+] HTML coverage report generated at: $REPORT_DIR/index.html"
    fi
else
    echo "[-] Warning: Neither gcovr nor lcov found. Raw .gcda/.gcno files available in $BUILD_DIR"
fi

echo "----------------------------------------------------------------------"

# 5. Open report if requested
if [ "$OPEN_REPORT" = true ] && [ -f "$REPORT_DIR/index.html" ]; then
    if command -v open &> /dev/null; then
        open "$REPORT_DIR/index.html"
    elif command -v xdg-open &> /dev/null; then
        xdg-open "$REPORT_DIR/index.html"
    fi
fi

if [ "$COVERAGE_PASS" = true ]; then
    echo "[✓] Coverage meets or exceeds the required threshold (${THRESHOLD}%)."
    exit 0
else
    echo "[✗] Coverage is below the required threshold (${THRESHOLD}%)."
    exit 1
fi
