#!/bin/bash

set -e

MACOS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${MACOS_DIR}/.."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[MACOS-TEST]${NC} $1"
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

# Check prerequisites
check_prerequisites() {
    print_status "Checking macOS development prerequisites..."
    
    # Check Xcode tools
    if ! xcode-select -p >/dev/null 2>&1; then
        print_error "Xcode command line tools not installed"
        print_info "Run: xcode-select --install"
        exit 1
    fi
    
    # Check CMake
    if ! command -v cmake >/dev/null 2>&1; then
        print_error "CMake not found"
        print_info "Install via: brew install cmake"
        exit 1
    fi
    
    # Check if we're on macOS
    if [[ "$(uname)" != "Darwin" ]]; then
        print_error "This script must be run on macOS"
        exit 1
    fi
    
    print_info "Prerequisites check passed"
}

# Build native tests for macOS
build_native_tests() {
    print_status "Building native tests for macOS..."
    
    cd "${PROJECT_DIR}"
    mkdir -p build-macos
    cd build-macos
    
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
        -DENABLE_TESTING=ON \
        -DENABLE_COVERAGE=ON \
        ..
    
    CPU_CORES=$(sysctl -n hw.ncpu)
    make -j"${CPU_CORES}"
    
    print_info "Native tests built successfully"
}

# Run native unit tests
run_unit_tests() {
    print_status "Running unit tests..."
    
    if [ -f "./unit_tests" ]; then
        if ./unit_tests --gtest_output=xml:macos_unit_results.xml; then
            print_status "Unit tests PASSED"
            return 0
        else
            print_error "Unit tests FAILED"
            return 1
        fi
    else
        print_warning "Unit test binary not found"
        return 1
    fi
}

# Run integration tests
run_integration_tests() {
    print_status "Running integration tests..."
    
    if [ -f "./integration_tests" ]; then
        if ./integration_tests --gtest_output=xml:macos_integration_results.xml; then
            print_status "Integration tests PASSED"
            return 0
        else
            print_error "Integration tests FAILED"
            return 1
        fi
    else
        print_warning "Integration test binary not found"
        return 1
    fi
}

# Run platform-specific tests
run_platform_tests() {
    print_status "Running macOS platform tests..."
    
    if [ -f "./platform_tests" ]; then
        if ./platform_tests --gtest_output=xml:macos_platform_results.xml; then
            print_status "Platform tests PASSED"
            return 0
        else
            print_error "Platform tests FAILED"
            return 1
        fi
    else
        print_warning "Platform test binary not found"
        return 1
    fi
}

# Run hardware tests
run_hardware_tests() {
    print_status "Running hardware tests..."
    
    if [ -f "./hardware_tests" ]; then
        if ./hardware_tests --gtest_output=xml:macos_hardware_results.xml; then
            print_status "Hardware tests PASSED"
            return 0
        else
            print_error "Hardware tests FAILED"
            return 1
        fi
    else
        print_warning "Hardware test binary not found"
        return 1
    fi
}

# Run system tests
run_system_tests() {
    print_status "Running system tests..."
    
    if [ -f "./system_tests" ]; then
        if ./system_tests --gtest_output=xml:macos_system_results.xml; then
            print_status "System tests PASSED"
            return 0
        else
            print_error "System tests FAILED"
            return 1
        fi
    else
        print_warning "System test binary not found"
        return 1
    fi
}

# Generate coverage report
generate_coverage_report() {
    print_status "Generating coverage report..."
    
    if command -v lcov >/dev/null 2>&1; then
        lcov --directory . --capture --output-file coverage.info
        lcov --remove coverage.info '/usr/*' '*/third_party/*' '*/tests/*' --output-file coverage.info
        lcov --list coverage.info
        
        if command -v genhtml >/dev/null 2>&1; then
            genhtml coverage.info --output-directory macos_coverage_report
            print_status "Coverage report generated in macos_coverage_report/"
            
            # Open coverage report in browser
            if command -v open >/dev/null 2>&1; then
                open macos_coverage_report/index.html
            fi
        fi
    else
        print_warning "lcov not found, skipping coverage report"
        print_info "Install via: brew install lcov"
    fi
}

# Run memory leak detection
run_memory_tests() {
    print_status "Running memory leak detection..."
    
    if command -v leaks >/dev/null 2>&1; then
        # Run tests with leak detection
        if [ -f "./unit_tests" ]; then
            print_info "Checking unit tests for memory leaks..."
            if leaks --atExit -- ./unit_tests >/dev/null 2>&1; then
                print_status "No memory leaks detected in unit tests"
            else
                print_warning "Potential memory leaks detected in unit tests"
            fi
        fi
        
        if [ -f "./integration_tests" ]; then
            print_info "Checking integration tests for memory leaks..."
            if leaks --atExit -- ./integration_tests >/dev/null 2>&1; then
                print_status "No memory leaks detected in integration tests"
            else
                print_warning "Potential memory leaks detected in integration tests"
            fi
        fi
    else
        print_info "Leaks tool not available, skipping memory tests"
    fi
}

# Build and run application tests
build_and_test_app() {
    print_status "Building and testing macOS application..."
    
    # Build the main application
    if make cross-terminal; then
        print_status "Application built successfully"
        
        # Basic smoke test
        if [ -f "./cross-terminal" ]; then
            print_info "Running application smoke test..."
            # Run app for a few seconds to ensure it starts correctly
            timeout 5s ./cross-terminal --test-mode || true
            print_status "Application smoke test completed"
        fi
    else
        print_error "Application build failed"
        return 1
    fi
}

# Run performance benchmarks
run_benchmarks() {
    print_status "Running performance benchmarks..."
    
    if [ -f "./benchmarks" ]; then
        ./benchmarks --benchmark_format=json > macos_benchmark_results.json
        print_status "Benchmarks completed"
        print_info "Results saved to macos_benchmark_results.json"
    else
        print_warning "Benchmark binary not found"
    fi
}

# Validate macOS-specific features
validate_macos_features() {
    print_status "Validating macOS-specific features..."
    
    # Check system information access
    print_info "Testing system information access..."
    if ./platform_tests --gtest_filter="*SystemInfo*" >/dev/null 2>&1; then
        print_status "System information access working"
    else
        print_warning "System information access may be limited"
    fi
    
    # Check hardware control features
    print_info "Testing hardware control features..."
    if ./hardware_tests --gtest_filter="*Control*" >/dev/null 2>&1; then
        print_status "Hardware control features working"
    else
        print_warning "Hardware control features may be limited"
    fi
    
    # Check network features
    print_info "Testing network features..."
    if ./platform_tests --gtest_filter="*Network*" >/dev/null 2>&1; then
        print_status "Network features working"
    else
        print_warning "Network features may be limited"
    fi
}

# Main execution
main() {
    local test_type="all"
    local coverage=false
    local memory_check=false
    local benchmarks=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --unit)
                test_type="unit"
                shift
                ;;
            --integration)
                test_type="integration"
                shift
                ;;
            --platform)
                test_type="platform"
                shift
                ;;
            --hardware)
                test_type="hardware"
                shift
                ;;
            --system)
                test_type="system"
                shift
                ;;
            --app)
                test_type="app"
                shift
                ;;
            --coverage)
                coverage=true
                shift
                ;;
            --memory)
                memory_check=true
                shift
                ;;
            --benchmarks)
                benchmarks=true
                shift
                ;;
            -h|--help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --unit        Run unit tests only"
                echo "  --integration Run integration tests only"
                echo "  --platform    Run platform tests only"
                echo "  --hardware    Run hardware tests only"
                echo "  --system      Run system tests only"
                echo "  --app         Build and test application only"
                echo "  --coverage    Generate coverage report"
                echo "  --memory      Run memory leak detection"
                echo "  --benchmarks  Run performance benchmarks"
                echo "  -h, --help    Show this help message"
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    print_status "Starting macOS testing pipeline..."
    
    check_prerequisites
    build_native_tests
    
    # Change to build directory
    cd "${PROJECT_DIR}/build-macos"
    
    local failed_tests=0
    
    case "${test_type}" in
        "unit")
            run_unit_tests || failed_tests=$((failed_tests + 1))
            ;;
        "integration")
            run_integration_tests || failed_tests=$((failed_tests + 1))
            ;;
        "platform")
            run_platform_tests || failed_tests=$((failed_tests + 1))
            ;;
        "hardware")
            run_hardware_tests || failed_tests=$((failed_tests + 1))
            ;;
        "system")
            run_system_tests || failed_tests=$((failed_tests + 1))
            ;;
        "app")
            build_and_test_app || failed_tests=$((failed_tests + 1))
            ;;
        "all")
            run_unit_tests || failed_tests=$((failed_tests + 1))
            run_integration_tests || failed_tests=$((failed_tests + 1))
            run_platform_tests || failed_tests=$((failed_tests + 1))
            run_hardware_tests || failed_tests=$((failed_tests + 1))
            run_system_tests || failed_tests=$((failed_tests + 1))
            build_and_test_app || failed_tests=$((failed_tests + 1))
            validate_macos_features
            ;;
    esac
    
    # Optional features
    if [ "$coverage" = true ]; then
        generate_coverage_report
    fi
    
    if [ "$memory_check" = true ]; then
        run_memory_tests
    fi
    
    if [ "$benchmarks" = true ]; then
        run_benchmarks
    fi
    
    # Final report
    if [ $failed_tests -eq 0 ]; then
        print_status "All macOS tests completed successfully!"
    else
        print_error "$failed_tests test suite(s) failed"
        exit 1
    fi
}

main "$@"