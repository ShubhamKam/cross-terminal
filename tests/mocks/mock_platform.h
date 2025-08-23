#pragma once

#include <gmock/gmock.h>
#include "platform/platform.h"

class MockPlatform : public Platform {
public:
    MOCK_METHOD(SystemInfo, getSystemInfo, (), (override));
    MOCK_METHOD(std::string, getDeviceModel, (), (override));
    
    MOCK_METHOD(bool, fileExists, (const std::string& path), (override));
    MOCK_METHOD(bool, createDirectory, (const std::string& path), (override));
    MOCK_METHOD(std::vector<std::string>, listDirectory, (const std::string& path), (override));
    MOCK_METHOD(std::string, getCurrentDirectory, (), (override));
    MOCK_METHOD(bool, setCurrentDirectory, (const std::string& path), (override));
    
    MOCK_METHOD(int, executeCommand, (const std::string& command, std::string& output), (override));
    MOCK_METHOD(bool, killProcess, (int pid), (override));
    MOCK_METHOD(std::vector<int>, getRunningProcesses, (), (override));
    
    MOCK_METHOD(bool, hasHardwareAccess, (), (override));
    MOCK_METHOD(bool, requestHardwarePermissions, (), (override));
    
    MOCK_METHOD(bool, hasNetworkAccess, (), (override));
    MOCK_METHOD(std::string, getIPAddress, (), (override));
    MOCK_METHOD(std::vector<std::string>, getNetworkInterfaces, (), (override));
};