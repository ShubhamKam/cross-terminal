cmake_minimum_required(VERSION 3.18.1)

# Set project name
set(PROJECT_NAME "cross-terminal")
project(${PROJECT_NAME})

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find required packages
find_library(log-lib log)
find_library(android-lib android)

# Include directories
include_directories(
    ../../../../../src
    ../../../../../src/core
    ../../../../../src/platform
    ../../../../../src/hardware
    ../../../../../src/memory
    ../../../../../src/utils
)

# Source files
set(CROSS_TERMINAL_SOURCES
    # JNI bridge
    native-lib.cpp
    
    # Core terminal engine
    ../../../../../src/core/terminal_engine.cpp
    ../../../../../src/core/command_processor.cpp
    ../../../../../src/core/terminal_renderer.cpp
    
    # Android platform implementation
    ../../../../../src/platform/android/android_platform.cpp
    
    # Android hardware implementation
    ../../../../../src/hardware/android/android_hardware.cpp
    
    # Memory management
    ../../../../../src/memory/memory_manager.cpp
    
    # Utilities
    ../../../../../src/utils/string_utils.cpp
    ../../../../../src/utils/file_utils.cpp
)

# Create shared library
add_library(
    ${PROJECT_NAME}
    SHARED
    ${CROSS_TERMINAL_SOURCES}
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -O3
    -ffast-math
    -funroll-loops
    -DANDROID_PLATFORM
    -DCROSS_TERMINAL_VERSION_MAJOR=1
    -DCROSS_TERMINAL_VERSION_MINOR=0
    -DCROSS_TERMINAL_VERSION_PATCH=0
)

# Link libraries
target_link_libraries(
    ${PROJECT_NAME}
    ${log-lib}
    ${android-lib}
)

# Android-specific configurations
if(ANDROID)
    # Set Android API level
    set(ANDROID_PLATFORM android-24)
    
    # Enable NEON optimizations for ARM
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        target_compile_options(${PROJECT_NAME} PRIVATE -mfpu=neon)
    endif()
    
    # Strip symbols in release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
    endif()
endif()

# Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
    target_compile_options(${PROJECT_NAME} PRIVATE -g)
endif()

# Install the shared library
install(TARGETS ${PROJECT_NAME} DESTINATION lib)