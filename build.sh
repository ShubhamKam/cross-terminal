#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Parse command line arguments
BUILD_TYPE="Release"
PLATFORM=""
CLEAN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --android)
            PLATFORM="android"
            shift
            ;;
        --ios)
            PLATFORM="ios"
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --debug       Build in debug mode"
            echo "  --release     Build in release mode (default)"
            echo "  --android     Target Android platform"
            echo "  --ios         Target iOS platform"
            echo "  --clean       Clean build directory first"
            echo "  -h, --help    Show this help message"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

print_status "Starting Cross-Terminal build..."
print_status "Build type: ${BUILD_TYPE}"

# Clean build directory if requested
if [ "$CLEAN" = true ]; then
    print_status "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure CMake based on platform
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"

case "${PLATFORM}" in
    "android")
        print_status "Configuring for Android..."
        if [ -z "${ANDROID_NDK}" ]; then
            print_error "ANDROID_NDK environment variable not set"
            exit 1
        fi
        CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake"
        CMAKE_ARGS="${CMAKE_ARGS} -DANDROID_ABI=arm64-v8a"
        CMAKE_ARGS="${CMAKE_ARGS} -DANDROID_PLATFORM=android-21"
        ;;
    "ios")
        print_status "Configuring for iOS..."
        CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake"
        CMAKE_ARGS="${CMAKE_ARGS} -DIOS_PLATFORM=OS64"
        ;;
    *)
        print_status "Configuring for native platform..."
        ;;
esac

# Run CMake configure
print_status "Running CMake configure..."
cmake ${CMAKE_ARGS} "${PROJECT_DIR}"

# Build the project
print_status "Building project..."
CPU_CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cmake --build . --config "${BUILD_TYPE}" -j "${CPU_CORES}"

if [ $? -eq 0 ]; then
    print_status "Build completed successfully!"
    print_status "Binary location: ${BUILD_DIR}/cross-terminal"
else
    print_error "Build failed!"
    exit 1
fi