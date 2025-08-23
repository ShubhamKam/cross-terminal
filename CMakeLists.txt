cmake_minimum_required(VERSION 3.16)
project(CrossTerminal VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(ANDROID)
    set(PLATFORM_ANDROID TRUE)
elseif(APPLE)
    if(IOS)
        set(PLATFORM_IOS TRUE)
    else()
        set(PLATFORM_MACOS TRUE)
    endif()
elseif(WIN32)
    set(PLATFORM_WINDOWS TRUE)
else()
    set(PLATFORM_LINUX TRUE)
endif()

# Core source files
set(CORE_SOURCES
    src/core/terminal.cpp
    src/core/shell.cpp
    src/core/command_parser.cpp
    src/core/process_manager.cpp
    src/core/history.cpp
)

# Platform abstraction layer
set(PAL_SOURCES
    src/platform/file_system.cpp
    src/platform/network.cpp
    src/platform/system_info.cpp
)

# Hardware control layer
set(HARDWARE_SOURCES
    src/hardware/gpio_controller.cpp
    src/hardware/sensor_manager.cpp
    src/hardware/system_monitor.cpp
)

# Rendering engine
set(RENDERER_SOURCES
    src/renderer/terminal_renderer.cpp
    src/renderer/text_renderer.cpp
    src/renderer/gpu_buffer.cpp
)

# UI framework
set(UI_SOURCES
    src/ui/terminal_ui.cpp
    src/ui/settings_ui.cpp
    src/ui/hardware_ui.cpp
)

# Platform-specific sources
if(PLATFORM_ANDROID)
    list(APPEND PAL_SOURCES src/platform/android/android_platform.cpp)
    list(APPEND HARDWARE_SOURCES src/hardware/android/android_hardware.cpp)
elseif(PLATFORM_IOS)
    list(APPEND PAL_SOURCES src/platform/ios/ios_platform.mm)
    list(APPEND HARDWARE_SOURCES src/hardware/ios/ios_hardware.mm)
elseif(PLATFORM_MACOS)
    list(APPEND PAL_SOURCES src/platform/macos/macos_platform.mm)
    list(APPEND HARDWARE_SOURCES src/hardware/macos/macos_hardware.mm)
elseif(PLATFORM_WINDOWS)
    list(APPEND PAL_SOURCES src/platform/windows/windows_platform.cpp)
    list(APPEND HARDWARE_SOURCES src/hardware/windows/windows_hardware.cpp)
else()
    list(APPEND PAL_SOURCES src/platform/linux/linux_platform.cpp)
    list(APPEND HARDWARE_SOURCES src/hardware/linux/linux_hardware.cpp)
endif()

# Third-party dependencies
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/glfw)

# Main executable
add_executable(cross-terminal
    src/main.cpp
    ${CORE_SOURCES}
    ${PAL_SOURCES}
    ${HARDWARE_SOURCES}
    ${RENDERER_SOURCES}
    ${UI_SOURCES}
)

# Include directories
target_include_directories(cross-terminal PRIVATE
    src/
    third_party/imgui
    third_party/glfw/include
)

# Link libraries
target_link_libraries(cross-terminal
    imgui
    glfw
)

# Platform-specific linking
if(PLATFORM_ANDROID)
    find_library(log-lib log)
    find_library(android-lib android)
    target_link_libraries(cross-terminal ${log-lib} ${android-lib} EGL GLESv2)
elseif(PLATFORM_IOS OR PLATFORM_MACOS)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
    target_link_libraries(cross-terminal ${FOUNDATION_FRAMEWORK} ${COREGRAPHICS_FRAMEWORK})
    if(PLATFORM_MACOS)
        find_library(COCOA_FRAMEWORK Cocoa)
        find_library(OPENGL_FRAMEWORK OpenGL)
        target_link_libraries(cross-terminal ${COCOA_FRAMEWORK} ${OPENGL_FRAMEWORK})
    endif()
elseif(PLATFORM_WINDOWS)
    target_link_libraries(cross-terminal opengl32 gdi32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(cross-terminal ${OPENGL_LIBRARIES} pthread)
endif()

# Compiler flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(cross-terminal PRIVATE DEBUG=1)
endif()