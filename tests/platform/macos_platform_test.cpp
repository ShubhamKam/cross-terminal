#include <gtest/gtest.h>
#include "platform/macos/macos_platform.h"

class macOSPlatformTest : public ::testing::Test {
protected:
    void SetUp() override {
        platform = std::make_unique<macOSPlatform>();
    }

    void TearDown() override {
        platform.reset();
    }

    std::unique_ptr<macOSPlatform> platform;
};

TEST_F(macOSPlatformTest, SystemInfoRetrieval) {
    SystemInfo info = platform->getSystemInfo();
    
    EXPECT_EQ(info.osName, "macOS");
    EXPECT_FALSE(info.osVersion.empty());
    EXPECT_FALSE(info.architecture.empty());
    EXPECT_GT(info.cpuCores, 0);
    EXPECT_GT(info.totalMemory, 0);
    EXPECT_GT(info.availableMemory, 0);
}

TEST_F(macOSPlatformTest, DeviceModelRetrieval) {
    std::string model = platform->getDeviceModel();
    EXPECT_FALSE(model.empty());
    // Should contain "Mac" for macOS devices
    EXPECT_NE(model.find("Mac"), std::string::npos);
}

TEST_F(macOSPlatformTest, FileSystemOperations) {
    // Test file existence check
    EXPECT_TRUE(platform->fileExists("/"));
    EXPECT_TRUE(platform->fileExists("/System"));
    EXPECT_FALSE(platform->fileExists("/non/existent/path"));
    
    // Test current directory
    std::string currentDir = platform->getCurrentDirectory();
    EXPECT_FALSE(currentDir.empty());
    EXPECT_EQ(currentDir.front(), '/'); // Should be absolute path
}

TEST_F(macOSPlatformTest, DirectoryListing) {
    // Test listing root directory
    auto files = platform->listDirectory("/");
    EXPECT_GT(files.size(), 0);
    
    // Check for common macOS directories
    bool hasCommonDirs = false;
    for (const auto& file : files) {
        if (file == "System" || file == "Users" || file == "Applications") {
            hasCommonDirs = true;
            break;
        }
    }
    EXPECT_TRUE(hasCommonDirs);
    
    // Test listing non-existent directory
    auto emptyFiles = platform->listDirectory("/non/existent/directory");
    EXPECT_TRUE(emptyFiles.empty());
}

TEST_F(macOSPlatformTest, DirectoryCreation) {
    std::string testDir = "/tmp/cross_terminal_test_dir";
    
    // Clean up first if exists
    platform->executeCommand("rm -rf " + testDir, std::string());
    
    // Create directory
    EXPECT_TRUE(platform->createDirectory(testDir));
    EXPECT_TRUE(platform->fileExists(testDir));
    
    // Clean up
    platform->executeCommand("rm -rf " + testDir, std::string());
}

TEST_F(macOSPlatformTest, ProcessOperations) {
    // Test getting running processes
    auto processes = platform->getRunningProcesses();
    EXPECT_GT(processes.size(), 10); // macOS should have many processes
    
    // Test command execution
    std::string output;
    int result = platform->executeCommand("echo 'macOS test'", output);
    EXPECT_EQ(result, 0); // Should succeed
    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("macOS test"), std::string::npos);
}

TEST_F(macOSPlatformTest, NetworkOperations) {
    // Test network access check
    bool hasNetwork = platform->hasNetworkAccess();
    // Most test environments should have network
    EXPECT_TRUE(hasNetwork);
    
    // Test getting network interfaces
    auto interfaces = platform->getNetworkInterfaces();
    EXPECT_GT(interfaces.size(), 0);
    
    // Check for loopback interface
    bool hasLoopback = std::find(interfaces.begin(), interfaces.end(), "lo0") != interfaces.end();
    EXPECT_TRUE(hasLoopback);
    
    // Test IP address retrieval
    std::string ip = platform->getIPAddress();
    if (hasNetwork && !ip.empty()) {
        // Basic IP format validation
        EXPECT_NE(ip.find("."), std::string::npos);
    }
}

TEST_F(macOSPlatformTest, HardwareAccessCheck) {
    // Test hardware access detection
    bool hasAccess = platform->hasHardwareAccess();
    EXPECT_TRUE(hasAccess); // macOS should generally allow system info access
    
    // Test permission request
    bool permissionGranted = platform->requestHardwarePermissions();
    EXPECT_TRUE(permissionGranted);
}

TEST_F(macOSPlatformTest, CommandExecutionVariations) {
    std::string output;
    
    // Test Unix commands available on macOS
    int result = platform->executeCommand("uname -s", output);
    EXPECT_EQ(result, 0);
    EXPECT_NE(output.find("Darwin"), std::string::npos);
    
    // Test path resolution
    result = platform->executeCommand("which ls", output);
    EXPECT_EQ(result, 0);
    EXPECT_NE(output.find("/bin/ls"), std::string::npos);
    
    // Test environment variable access
    result = platform->executeCommand("echo $HOME", output);
    EXPECT_EQ(result, 0);
    EXPECT_FALSE(output.empty());
}

TEST_F(macOSPlatformTest, DirectoryNavigation) {
    std::string originalDir = platform->getCurrentDirectory();
    
    // Try changing to /tmp
    bool success = platform->setCurrentDirectory("/tmp");
    if (success) {
        EXPECT_EQ(platform->getCurrentDirectory(), "/tmp");
        
        // Change back
        platform->setCurrentDirectory(originalDir);
        EXPECT_EQ(platform->getCurrentDirectory(), originalDir);
    }
}