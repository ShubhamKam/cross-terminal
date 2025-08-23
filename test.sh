#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
TEST_DIR="${PROJECT_DIR}/tests"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[TEST]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Parse command line arguments
TEST_TYPE="all"
BUILD_TYPE="Debug"
COVERAGE=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --unit)
            TEST_TYPE="unit"
            shift
            ;;
        --integration)
            TEST_TYPE="integration"
            shift
            ;;
        --system)
            TEST_TYPE="system"
            shift
            ;;
        --platform)
            TEST_TYPE="platform"
            shift
            ;;
        --hardware)
            TEST_TYPE="hardware"
            shift
            ;;
        --fast)
            TEST_TYPE="fast"
            shift
            ;;
        --coverage)
            COVERAGE=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --unit         Run only unit tests"
            echo "  --integration  Run only integration tests"
            echo "  --system       Run only system tests"
            echo "  --platform     Run only platform tests"
            echo "  --hardware     Run only hardware tests"
            echo "  --fast         Run fast tests only (unit tests)"
            echo "  --coverage     Generate coverage report"
            echo "  --verbose      Verbose output"
            echo "  --release      Build in release mode"
            echo "  -h, --help     Show this help message"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

print_status "Starting Cross-Terminal test suite..."
print_info "Test type: ${TEST_TYPE}"
print_info "Build type: ${BUILD_TYPE}"

# Ensure build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    print_error "Build directory not found. Please run ./build.sh first."
    exit 1
fi

cd "${BUILD_DIR}"

# Build tests
print_status "Building tests..."
if [ "$COVERAGE" = true ]; then
    cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON "${PROJECT_DIR}"
else
    cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" "${PROJECT_DIR}"
fi

CPU_CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cmake --build . --target test_all -j "${CPU_CORES}"

# Function to run tests with proper error handling
run_test_suite() {
    local test_name=$1
    local test_binary=$2
    
    if [ -f "${test_binary}" ]; then
        print_status "Running ${test_name} tests..."
        if [ "$VERBOSE" = true ]; then
            ./"${test_binary}" --gtest_output=xml:${test_name}_results.xml
        else
            ./"${test_binary}" --gtest_output=xml:${test_name}_results.xml --gtest_brief=1
        fi
        
        if [ $? -eq 0 ]; then
            print_status "${test_name} tests PASSED"
        else
            print_error "${test_name} tests FAILED"
            return 1
        fi
    else
        print_warning "${test_name} test binary not found, skipping..."
    fi
}

# Run tests based on type
case "${TEST_TYPE}" in
    "unit")
        run_test_suite "Unit" "unit_tests"
        ;;
    "integration")
        run_test_suite "Integration" "integration_tests"
        ;;
    "system")
        run_test_suite "System" "system_tests"
        ;;
    "platform")
        run_test_suite "Platform" "platform_tests"
        ;;
    "hardware")
        run_test_suite "Hardware" "hardware_tests"
        ;;
    "fast")
        run_test_suite "Unit" "unit_tests"
        ;;
    "all")
        print_status "Running all test suites..."
        
        run_test_suite "Unit" "unit_tests"
        run_test_suite "Integration" "integration_tests"
        run_test_suite "System" "system_tests"
        run_test_suite "Platform" "platform_tests"
        run_test_suite "Hardware" "hardware_tests"
        ;;
    *)
        print_error "Unknown test type: ${TEST_TYPE}"
        exit 1
        ;;
esac

# Generate coverage report if requested
if [ "$COVERAGE" = true ]; then
    print_status "Generating coverage report..."
    
    if command -v lcov >/dev/null 2>&1; then
        lcov --directory . --capture --output-file coverage.info
        lcov --remove coverage.info '/usr/*' '*/third_party/*' '*/tests/*' --output-file coverage.info
        lcov --list coverage.info
        
        if command -v genhtml >/dev/null 2>&1; then
            genhtml coverage.info --output-directory coverage_report
            print_status "Coverage report generated in coverage_report/"
        fi
    else
        print_warning "lcov not found, skipping coverage report generation"
    fi
fi

# Run performance benchmarks if available
if [ -f "benchmarks" ]; then
    print_status "Running performance benchmarks..."
    ./benchmarks --benchmark_format=json > benchmark_results.json
fi

# Test result summary
print_status "Test execution completed!"
print_info "Test results saved to XML files"

if [ "$COVERAGE" = true ] && [ -d "coverage_report" ]; then
    print_info "Coverage report available in build/coverage_report/index.html"
fi

if [ -f "benchmark_results.json" ]; then
    print_info "Benchmark results saved to benchmark_results.json"
fi