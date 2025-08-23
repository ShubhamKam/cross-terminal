#include <gtest/gtest.h>
#include "platform/android/android_platform.h"

class AndroidPlatformTest : public ::testing::Test {
protected:
    void SetUp() override {
        platform = std::make_unique<AndroidPlatform>();
    }

    void TearDown() override {
        platform.reset();
    }

    std::unique_ptr<AndroidPlatform> platform;
};

TEST_F(AndroidPlatformTest, SystemInfoRetrieval) {
    SystemInfo info = platform->getSystemInfo();
    
    EXPECT_EQ(info.osName, "Android");
    EXPECT_FALSE(info.osVersion.empty());
    EXPECT_FALSE(info.architecture.empty());
    EXPECT_GT(info.cpuCores, 0);
    // Memory values might be 0 if not accessible
}

TEST_F(AndroidPlatformTest, DeviceModelRetrieval) {
    std::string model = platform->getDeviceModel();
    EXPECT_FALSE(model.empty());
}

TEST_F(AndroidPlatformTest, FileSystemOperations) {
    // Test file existence check
    EXPECT_TRUE(platform->fileExists("/system"));
    EXPECT_FALSE(platform->fileExists("/non/existent/path"));
    
    // Test current directory
    std::string currentDir = platform->getCurrentDirectory();
    EXPECT_FALSE(currentDir.empty());
    EXPECT_EQ(currentDir.front(), '/'); // Should be absolute path
}

TEST_F(AndroidPlatformTest, DirectoryListing) {
    // Test listing a directory that should exist
    auto files = platform->listDirectory("/system");
    // /system should have some contents on Android
    // But we can't assume specific files, so just check it doesn't crash
    EXPECT_TRUE(true); // Test completed without crash
    
    // Test listing non-existent directory
    auto emptyFiles = platform->listDirectory("/non/existent/directory");
    EXPECT_TRUE(emptyFiles.empty());
}

TEST_F(AndroidPlatformTest, ProcessOperations) {
    // Test getting running processes
    auto processes = platform->getRunningProcesses();
    EXPECT_GT(processes.size(), 0); // Should have at least some processes
    
    // Test command execution
    std::string output;
    int result = platform->executeCommand("echo 'test'", output);
    EXPECT_EQ(result, 0); // Should succeed
    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("test"), std::string::npos);
}

TEST_F(AndroidPlatformTest, NetworkOperations) {
    // Test network access check
    bool hasNetwork = platform->hasNetworkAccess();
    // Can't assume network state, so just test it doesn't crash
    EXPECT_TRUE(true);
    
    // Test getting network interfaces
    auto interfaces = platform->getNetworkInterfaces();
    // Should have at least loopback interface
    EXPECT_GT(interfaces.size(), 0);
    
    // Check for common Android interfaces
    bool hasCommonInterface = false;
    for (const auto& iface : interfaces) {
        if (iface == "lo" || iface == "wlan0" || iface == "rmnet0") {
            hasCommonInterface = true;
            break;
        }
    }
    EXPECT_TRUE(hasCommonInterface);
}

TEST_F(AndroidPlatformTest, HardwareAccessCheck) {
    // Test hardware access detection
    bool hasAccess = platform->hasHardwareAccess();
    // Can't assume hardware access state
    EXPECT_TRUE(true);
    
    // Test permission request
    bool permissionGranted = platform->requestHardwarePermissions();
    // Can't assume permission state
    EXPECT_TRUE(true);
}

TEST_F(AndroidPlatformTest, CommandExecutionEdgeCases) {
    std::string output;
    
    // Test empty command
    int result = platform->executeCommand("", output);
    // Empty command should fail or return quickly
    
    // Test command with special characters
    result = platform->executeCommand("echo 'hello world'", output);
    EXPECT_EQ(result, 0);
    EXPECT_NE(output.find("hello world"), std::string::npos);
    
    // Test command that should fail
    result = platform->executeCommand("nonexistentcommand123456", output);
    EXPECT_NE(result, 0); // Should fail
}