#pragma once

#include "../platform.h"
#include <mach/mach.h>
#include <sys/types.h>
#include <sys/sysctl.h>

class macOSPlatform : public Platform {
public:
    macOSPlatform();
    virtual ~macOSPlatform();
    
    // System information
    SystemInfo getSystemInfo() override;
    std::string getDeviceModel() override;
    
    // File system operations
    bool fileExists(const std::string& path) override;
    bool createDirectory(const std::string& path) override;
    std::vector<std::string> listDirectory(const std::string& path) override;
    std::string getCurrentDirectory() override;
    bool setCurrentDirectory(const std::string& path) override;
    
    // Process management
    int executeCommand(const std::string& command, std::string& output) override;
    bool killProcess(int pid) override;
    std::vector<int> getRunningProcesses() override;
    
    // Hardware access
    bool hasHardwareAccess() override;
    bool requestHardwarePermissions() override;
    
    // Network operations
    bool hasNetworkAccess() override;
    std::string getIPAddress() override;
    std::vector<std::string> getNetworkInterfaces() override;
    
private:
    void initializemacOSSpecific();
};