#include "hardware_controller.h"

#ifdef __ANDROID__
#include "android/android_hardware.h"
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include "ios/ios_hardware.h"
#else
#include "macos/macos_hardware.h"
#endif
#elif defined(_WIN32)
#include "windows/windows_hardware.h"
#else
#include "linux/linux_hardware.h"
#endif

std::unique_ptr<HardwareController> HardwareController::create() {
#ifdef __ANDROID__
    return std::make_unique<AndroidHardwareController>();
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
    return std::make_unique<iOSHardwareController>();
#else
    return std::make_unique<macOSHardwareController>();
#endif
#elif defined(_WIN32)
    return std::make_unique<WindowsHardwareController>();
#else
    return std::make_unique<LinuxHardwareController>();
#endif
}