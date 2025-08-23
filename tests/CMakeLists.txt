cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Find testing framework
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # Download and build GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/release-1.12.1.zip
    )
    FetchContent_MakeAvailable(googletest)
    set(GTest_FOUND TRUE)
endif()

# Test configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

# Common test libraries
set(TEST_LIBS 
    gtest 
    gtest_main 
    gmock 
    gmock_main
    pthread
)

# Mock implementations
add_library(test_mocks
    mocks/mock_platform.cpp
    mocks/mock_hardware_controller.cpp
    mocks/mock_shell.cpp
)

target_include_directories(test_mocks PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests/mocks
)

target_link_libraries(test_mocks ${TEST_LIBS})

# Unit Tests
file(GLOB_RECURSE UNIT_TEST_SOURCES "unit/*.cpp")
add_executable(unit_tests ${UNIT_TEST_SOURCES})
target_link_libraries(unit_tests 
    test_mocks
    ${TEST_LIBS}
)

# Integration Tests
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES "integration/*.cpp")
add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})
target_link_libraries(integration_tests 
    test_mocks
    ${TEST_LIBS}
)

# System Tests
file(GLOB_RECURSE SYSTEM_TEST_SOURCES "system/*.cpp")
add_executable(system_tests ${SYSTEM_TEST_SOURCES})
target_link_libraries(system_tests 
    test_mocks
    ${TEST_LIBS}
)

# Platform Tests
if(ANDROID)
    file(GLOB PLATFORM_TEST_SOURCES "platform/android_*.cpp")
elseif(APPLE)
    if(IOS)
        file(GLOB PLATFORM_TEST_SOURCES "platform/ios_*.cpp")
    else()
        file(GLOB PLATFORM_TEST_SOURCES "platform/macos_*.cpp")
    endif()
elseif(WIN32)
    file(GLOB PLATFORM_TEST_SOURCES "platform/windows_*.cpp")
else()
    file(GLOB PLATFORM_TEST_SOURCES "platform/linux_*.cpp")
endif()

if(PLATFORM_TEST_SOURCES)
    add_executable(platform_tests ${PLATFORM_TEST_SOURCES})
    target_link_libraries(platform_tests 
        test_mocks
        ${TEST_LIBS}
    )
    add_test(NAME platform_tests COMMAND platform_tests)
endif()

# Hardware Tests
file(GLOB_RECURSE HARDWARE_TEST_SOURCES "hardware/*.cpp")
add_executable(hardware_tests ${HARDWARE_TEST_SOURCES})
target_link_libraries(hardware_tests 
    test_mocks
    ${TEST_LIBS}
)

# Register tests with CTest
add_test(NAME unit_tests COMMAND unit_tests)
add_test(NAME integration_tests COMMAND integration_tests)
add_test(NAME system_tests COMMAND system_tests)
add_test(NAME hardware_tests COMMAND hardware_tests)

# Test coverage (if supported)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
    if(ENABLE_COVERAGE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    endif()
endif()

# Performance benchmarks
file(GLOB_RECURSE BENCHMARK_SOURCES "benchmarks/*.cpp")
if(BENCHMARK_SOURCES)
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        add_executable(benchmarks ${BENCHMARK_SOURCES})
        target_link_libraries(benchmarks 
            test_mocks
            benchmark::benchmark
            ${TEST_LIBS}
        )
        add_test(NAME benchmarks COMMAND benchmarks)
    endif()
endif()

# Custom test targets
add_custom_target(test_all
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS unit_tests integration_tests system_tests hardware_tests
)

add_custom_target(test_fast
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose -R "unit_tests"
    DEPENDS unit_tests
)

add_custom_target(test_coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    COMMAND lcov --directory . --capture --output-file coverage.info
    COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
    COMMAND lcov --list coverage.info
    DEPENDS unit_tests integration_tests
)