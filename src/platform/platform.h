#pragma once

#include <memory>
#include <string>
#include <vector>

enum class PlatformType {
    Android,
    iOS,
    macOS,
    Windows,
    Linux
};

struct SystemInfo {
    std::string osName;
    std::string osVersion;
    std::string architecture;
    int cpuCores;
    uint64_t totalMemory;
    uint64_t availableMemory;
};

class Platform {
public:
    virtual ~Platform() = default;
    
    static std::unique_ptr<Platform> create();
    static PlatformType getCurrentPlatform();
    
    // System information
    virtual SystemInfo getSystemInfo() = 0;
    virtual std::string getDeviceModel() = 0;
    
    // File system operations
    virtual bool fileExists(const std::string& path) = 0;
    virtual bool createDirectory(const std::string& path) = 0;
    virtual std::vector<std::string> listDirectory(const std::string& path) = 0;
    virtual std::string getCurrentDirectory() = 0;
    virtual bool setCurrentDirectory(const std::string& path) = 0;
    
    // Process management
    virtual int executeCommand(const std::string& command, std::string& output) = 0;
    virtual bool killProcess(int pid) = 0;
    virtual std::vector<int> getRunningProcesses() = 0;
    
    // Hardware access (if supported)
    virtual bool hasHardwareAccess() = 0;
    virtual bool requestHardwarePermissions() = 0;
    
    // Network operations
    virtual bool hasNetworkAccess() = 0;
    virtual std::string getIPAddress() = 0;
    virtual std::vector<std::string> getNetworkInterfaces() = 0;
    
protected:
    Platform() = default;
};