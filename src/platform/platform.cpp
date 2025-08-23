#include "platform.h"

#ifdef __ANDROID__
#include "android/android_platform.h"
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include "ios/ios_platform.h"
#else
#include "macos/macos_platform.h"
#endif
#elif defined(_WIN32)
#include "windows/windows_platform.h"
#else
#include "linux/linux_platform.h"
#endif

std::unique_ptr<Platform> Platform::create() {
#ifdef __ANDROID__
    return std::make_unique<AndroidPlatform>();
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
    return std::make_unique<iOSPlatform>();
#else
    return std::make_unique<macOSPlatform>();
#endif
#elif defined(_WIN32)
    return std::make_unique<WindowsPlatform>();
#else
    return std::make_unique<LinuxPlatform>();
#endif
}

PlatformType Platform::getCurrentPlatform() {
#ifdef __ANDROID__
    return PlatformType::Android;
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
    return PlatformType::iOS;
#else
    return PlatformType::macOS;
#endif
#elif defined(_WIN32)
    return PlatformType::Windows;
#else
    return PlatformType::Linux;
#endif
}