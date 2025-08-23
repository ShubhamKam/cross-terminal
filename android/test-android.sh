#!/bin/bash

set -e

ANDROID_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${ANDROID_DIR}/.."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[ANDROID-TEST]${NC} $1"
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
    print_status "Checking Android development prerequisites..."
    
    if [ -z "${ANDROID_NDK}" ]; then
        print_error "ANDROID_NDK environment variable not set"
        exit 1
    fi
    
    if [ -z "${ANDROID_SDK_ROOT}" ]; then
        print_error "ANDROID_SDK_ROOT environment variable not set"
        exit 1
    fi
    
    if ! command -v adb >/dev/null 2>&1; then
        print_error "adb not found in PATH"
        exit 1
    fi
    
    # Check if device is connected
    if ! adb devices | grep -q "device$"; then
        print_warning "No Android device connected. Some tests may be skipped."
    fi
    
    print_info "Prerequisites check passed"
}

# Build native tests for Android
build_native_tests() {
    print_status "Building native tests for Android..."
    
    cd "${PROJECT_DIR}"
    mkdir -p build-android
    cd build-android
    
    cmake \
        -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK}/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI=arm64-v8a \
        -DANDROID_PLATFORM=android-21 \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_TESTING=ON \
        ..
    
    make -j$(nproc)
    
    print_info "Native tests built successfully"
}

# Push test binaries to device
deploy_tests() {
    print_status "Deploying test binaries to Android device..."
    
    if ! adb devices | grep -q "device$"; then
        print_warning "No device connected, skipping deployment"
        return 0
    fi
    
    # Create test directory on device
    adb shell "mkdir -p /data/local/tmp"
    
    # Push test binaries
    if [ -f "unit_tests" ]; then
        adb push unit_tests /data/local/tmp/cross-terminal-unit-tests
        adb shell "chmod 755 /data/local/tmp/cross-terminal-unit-tests"
    fi
    
    if [ -f "integration_tests" ]; then
        adb push integration_tests /data/local/tmp/cross-terminal-integration-tests
        adb shell "chmod 755 /data/local/tmp/cross-terminal-integration-tests"
    fi
    
    if [ -f "platform_tests" ]; then
        adb push platform_tests /data/local/tmp/cross-terminal-platform-tests
        adb shell "chmod 755 /data/local/tmp/cross-terminal-platform-tests"
    fi
    
    if [ -f "hardware_tests" ]; then
        adb push hardware_tests /data/local/tmp/cross-terminal-hardware-tests
        adb shell "chmod 755 /data/local/tmp/cross-terminal-hardware-tests"
    fi
    
    print_info "Test binaries deployed"
}

# Run native tests on device
run_native_tests() {
    print_status "Running native tests on Android device..."
    
    if ! adb devices | grep -q "device$"; then
        print_warning "No device connected, skipping native tests"
        return 0
    fi
    
    local failed_tests=0
    
    # Run unit tests
    if adb shell "test -f /data/local/tmp/cross-terminal-unit-tests"; then
        print_info "Running unit tests..."
        if adb shell "/data/local/tmp/cross-terminal-unit-tests --gtest_output=xml:/data/local/tmp/unit_results.xml"; then
            print_status "Unit tests PASSED"
            adb pull /data/local/tmp/unit_results.xml android_unit_results.xml 2>/dev/null || true
        else
            print_error "Unit tests FAILED"
            failed_tests=$((failed_tests + 1))
        fi
    fi
    
    # Run integration tests
    if adb shell "test -f /data/local/tmp/cross-terminal-integration-tests"; then
        print_info "Running integration tests..."
        if adb shell "/data/local/tmp/cross-terminal-integration-tests --gtest_output=xml:/data/local/tmp/integration_results.xml"; then
            print_status "Integration tests PASSED"
            adb pull /data/local/tmp/integration_results.xml android_integration_results.xml 2>/dev/null || true
        else
            print_error "Integration tests FAILED"
            failed_tests=$((failed_tests + 1))
        fi
    fi
    
    # Run platform tests
    if adb shell "test -f /data/local/tmp/cross-terminal-platform-tests"; then
        print_info "Running platform tests..."
        if adb shell "/data/local/tmp/cross-terminal-platform-tests --gtest_output=xml:/data/local/tmp/platform_results.xml"; then
            print_status "Platform tests PASSED"
            adb pull /data/local/tmp/platform_results.xml android_platform_results.xml 2>/dev/null || true
        else
            print_error "Platform tests FAILED"
            failed_tests=$((failed_tests + 1))
        fi
    fi
    
    # Run hardware tests
    if adb shell "test -f /data/local/tmp/cross-terminal-hardware-tests"; then
        print_info "Running hardware tests..."
        if adb shell "/data/local/tmp/cross-terminal-hardware-tests --gtest_output=xml:/data/local/tmp/hardware_results.xml"; then
            print_status "Hardware tests PASSED"
            adb pull /data/local/tmp/hardware_results.xml android_hardware_results.xml 2>/dev/null || true
        else
            print_error "Hardware tests FAILED"
            failed_tests=$((failed_tests + 1))
        fi
    fi
    
    return $failed_tests
}

# Build Android app
build_android_app() {
    print_status "Building Android application..."
    
    cd "${ANDROID_DIR}"
    
    if [ -f "gradlew" ]; then
        ./gradlew clean assembleDebug
    else
        gradle clean assembleDebug
    fi
    
    print_info "Android app built successfully"
}

# Run Android instrumentation tests
run_instrumentation_tests() {
    print_status "Running Android instrumentation tests..."
    
    if ! adb devices | grep -q "device$"; then
        print_warning "No device connected, skipping instrumentation tests"
        return 0
    fi
    
    cd "${ANDROID_DIR}"
    
    if [ -f "gradlew" ]; then
        ./gradlew connectedAndroidTest
    else
        gradle connectedAndroidTest
    fi
    
    print_info "Instrumentation tests completed"
}

# Main execution
main() {
    local test_type="all"
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --native)
                test_type="native"
                shift
                ;;
            --app)
                test_type="app"
                shift
                ;;
            --instrumentation)
                test_type="instrumentation"
                shift
                ;;
            -h|--help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --native          Run native C++ tests only"
                echo "  --app            Build Android app only"
                echo "  --instrumentation Run instrumentation tests only"
                echo "  -h, --help       Show this help message"
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    print_status "Starting Android testing pipeline..."
    
    check_prerequisites
    
    case "${test_type}" in
        "native")
            build_native_tests
            deploy_tests
            run_native_tests
            ;;
        "app")
            build_android_app
            ;;
        "instrumentation")
            build_android_app
            run_instrumentation_tests
            ;;
        "all")
            build_native_tests
            deploy_tests
            build_android_app
            
            local failed_tests=0
            if ! run_native_tests; then
                failed_tests=$((failed_tests + $?))
            fi
            
            if ! run_instrumentation_tests; then
                failed_tests=$((failed_tests + 1))
            fi
            
            if [ $failed_tests -gt 0 ]; then
                print_error "Some tests failed"
                exit 1
            fi
            ;;
    esac
    
    print_status "Android testing completed successfully!"
}

main "$@"