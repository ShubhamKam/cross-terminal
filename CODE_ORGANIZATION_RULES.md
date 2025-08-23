# Cross-Terminal Code Organization and Optimization Rules

## File Organization Rules

### RULE 1: Hierarchical Module Structure
```
src/
├── core/                    # Core terminal functionality
│   ├── interfaces/         # Abstract interfaces
│   ├── implementations/    # Concrete implementations
│   └── utils/             # Utility functions
├── platform/              # Platform abstraction layer
│   ├── common/            # Shared platform code
│   ├── android/           # Android-specific
│   ├── macos/             # macOS-specific
│   └── interfaces/        # Platform interfaces
├── hardware/              # Hardware control layer
│   ├── controllers/       # Hardware controllers
│   ├── sensors/           # Sensor implementations
│   └── interfaces/        # Hardware interfaces
├── memory/                # Memory management
│   ├── pools/             # Memory pools
│   ├── allocators/        # Custom allocators
│   └── monitors/          # Memory monitoring
├── networking/            # Network functionality
├── ui/                    # User interface
├── renderer/              # Rendering engine
└── plugins/               # Plugin architecture
```

### RULE 2: File Naming Conventions
- **Interfaces:** `i_*.h` (e.g., `i_platform.h`)
- **Implementations:** `*_impl.h/.cpp` (e.g., `platform_impl.cpp`)
- **Platform-specific:** `*_[platform].h/.cpp` (e.g., `platform_android.cpp`)
- **Utilities:** `*_utils.h/.cpp` (e.g., `string_utils.cpp`)
- **Managers:** `*_manager.h/.cpp` (e.g., `memory_manager.cpp`)
- **Controllers:** `*_controller.h/.cpp` (e.g., `hardware_controller.cpp`)

### RULE 3: Header File Organization
```cpp
// 1. License header (if applicable)
// 2. Header guard or #pragma once
// 3. System includes (<>)
// 4. Third-party includes (<>)
// 5. Project includes ("")
// 6. Forward declarations
// 7. Constants and macros
// 8. Type definitions
// 9. Class/struct declarations
// 10. Template implementations (if in header)
```

## Modularization Rules

### RULE 4: Interface Segregation
- Each module MUST have a clear, minimal interface
- Interfaces MUST be platform-agnostic
- Implementation details MUST be hidden behind interfaces
- Dependencies MUST flow towards abstractions, not concretions

### RULE 5: Single Responsibility Modules
- Each file MUST have a single, well-defined responsibility
- Maximum 500 lines per implementation file
- Maximum 200 lines per header file
- Complex functionality MUST be split into multiple modules

### RULE 6: Dependency Inversion
```cpp
// BAD: Direct dependency on concrete class
class Terminal {
    AndroidPlatform platform; // Tight coupling
};

// GOOD: Dependency on interface
class Terminal {
    std::unique_ptr<IPlatform> platform; // Loose coupling
};
```

## C/C++ Optimization Rules

### RULE 7: Memory Management Efficiency
- **Stack allocation preferred** over heap for small, short-lived objects
- **Memory pools** MUST be used for frequent allocations
- **RAII** MUST be used for all resource management
- **Smart pointers** MUST be used instead of raw pointers
- **Move semantics** MUST be used for expensive-to-copy objects

```cpp
// Memory pool for frequent allocations
class CommandBuffer {
private:
    static MemoryPool<CommandBuffer, 1024> pool_;
public:
    void* operator new(size_t size) { return pool_.allocate(); }
    void operator delete(void* ptr) { pool_.deallocate(ptr); }
};
```

### RULE 8: CPU Performance Optimization
- **Cache-friendly data structures** MUST be used
- **Branch prediction** optimization via likely/unlikely hints
- **Vectorization** for data-parallel operations
- **Compile-time computation** via constexpr/templates
- **Inlining** for small, frequently-called functions

```cpp
// Cache-friendly structure layout
struct alignas(64) ProcessInfo { // Cache line aligned
    int pid;                     // Hot data together
    ProcessState state;
    char padding[52];            // Pad to cache line
    std::string name;            // Cold data separate
};
```

### RULE 9: Memory Layout Optimization
- **Structure padding** awareness and optimization
- **Data-oriented design** for performance-critical code
- **Memory alignment** for SIMD operations
- **False sharing** avoidance in multi-threaded code

## Machine Interpretability Rules

### RULE 10: Self-Documenting Code Structure
- **Consistent naming conventions** across all modules
- **Type-safe interfaces** with strong typing
- **Compile-time contracts** via concepts/SFINAE
- **Machine-readable annotations** for tooling

```cpp
// Machine-readable annotations
class CROSS_TERMINAL_API IPlatform {
public:
    // @brief Gets system information
    // @returns SystemInfo structure with OS details
    // @thread_safe true
    // @performance O(1)
    virtual SystemInfo getSystemInfo() const noexcept = 0;
};
```

### RULE 11: Interface Documentation Standards
- **Doxygen-compatible** comments for all public interfaces
- **Contract specifications** (preconditions, postconditions)
- **Performance characteristics** (Big-O notation)
- **Thread safety** guarantees
- **Exception specifications**

### RULE 12: Modular Testing Architecture
```cpp
// Each module MUST have corresponding test module
src/core/terminal.cpp          -> tests/unit/core/test_terminal.cpp
src/platform/android_platform.cpp -> tests/platform/test_android_platform.cpp
src/hardware/gpio_controller.cpp  -> tests/hardware/test_gpio_controller.cpp
```

## Language-Specific Optimization Rules

### RULE 13: C++ Modern Features
- **constexpr** for compile-time computation
- **std::optional** instead of null pointers for optional values
- **std::variant** instead of unions for type-safe variants
- **std::string_view** for string parameters that don't take ownership
- **Concepts** (C++20) for template constraints

```cpp
// Modern C++ optimization example
template<typename T>
requires std::integral<T>
constexpr auto fast_log2(T value) noexcept -> int {
    return value ? 31 - __builtin_clz(value) : -1;
}
```

### RULE 14: Memory Pool Design
```cpp
template<typename T, size_t PoolSize>
class MemoryPool {
private:
    alignas(T) char storage_[sizeof(T) * PoolSize];
    std::bitset<PoolSize> used_;
    std::atomic<size_t> next_free_{0};
    
public:
    T* allocate() noexcept;
    void deallocate(T* ptr) noexcept;
    size_t available() const noexcept;
};
```

### RULE 15: Platform-Specific Optimizations
- **Android:** Use NDK optimizations, minimize JNI calls
- **macOS:** Leverage Grand Central Dispatch for concurrency
- **ARM:** NEON SIMD instructions for data processing
- **x86:** SSE/AVX instructions for vector operations

## Code Quality Enforcement

### RULE 16: Static Analysis Integration
- **clang-tidy** MUST pass with project configuration
- **cppcheck** MUST report no issues
- **Address Sanitizer** MUST pass in debug builds
- **Thread Sanitizer** MUST pass for concurrent code
- **Memory Sanitizer** MUST pass for all code paths

### RULE 17: Performance Monitoring
- **Benchmarking** MUST be integrated for critical paths
- **Memory profiling** MUST be automated in CI
- **CPU profiling** for performance regression detection
- **Cache miss analysis** for memory-intensive operations

### RULE 18: Documentation Generation
- **API documentation** MUST be automatically generated
- **Architecture diagrams** MUST be kept up-to-date
- **Performance characteristics** MUST be documented
- **Memory usage patterns** MUST be documented

## Build System Integration

### RULE 19: Modular Build Configuration
```cmake
# Each module as separate CMake target
add_library(cross_terminal_core STATIC ${CORE_SOURCES})
add_library(cross_terminal_platform STATIC ${PLATFORM_SOURCES})
add_library(cross_terminal_hardware STATIC ${HARDWARE_SOURCES})

# Explicit dependency management
target_link_libraries(cross_terminal_core 
    PRIVATE cross_terminal_memory
    PUBLIC cross_terminal_interfaces
)
```

### RULE 20: Conditional Compilation
```cpp
// Platform-specific optimizations
#if defined(CROSS_TERMINAL_ANDROID)
    #include "android/optimizations.h"
    using PlatformOptimizer = AndroidOptimizer;
#elif defined(CROSS_TERMINAL_MACOS)
    #include "macos/optimizations.h"
    using PlatformOptimizer = MacOSOptimizer;
#endif
```

## Enforcement Mechanisms

### Automated Checks
1. **Pre-commit hooks** verify file organization
2. **CI pipeline** enforces code structure rules
3. **Static analysis** catches optimization violations
4. **Performance tests** detect regressions
5. **Documentation generation** validates comments

### Metrics Tracking
1. **Code complexity** per module
2. **Memory allocation patterns**
3. **CPU usage characteristics**
4. **Cache hit ratios**
5. **Binary size per platform**

### Quick Verification Commands
```bash
# Verify code organization
./scripts/verify-structure.sh

# Check optimization compliance
./scripts/check-optimizations.sh

# Generate performance report
./scripts/performance-audit.sh

# Validate documentation
./scripts/docs-check.sh
```

These rules ensure that the Cross-Terminal codebase remains maintainable, performant, and machine-interpretable while following best practices for C++ development and cross-platform compatibility.