#pragma once

#include "../hardware_controller.h"
#include <map>
#include <set>
#include <thread>
#include <atomic>
#include <sys/statvfs.h>

class AndroidHardwareController : public HardwareController {
public:
    AndroidHardwareController();
    virtual ~AndroidHardwareController();
    
    // GPIO operations
    bool isGPIOSupported() override;
    bool configureGPIO(int pin, GPIOMode mode) override;
    bool writeGPIO(int pin, bool high) override;
    bool readGPIO(int pin) override;
    
    // Sensor access
    std::vector<SensorType> getAvailableSensors() override;
    bool enableSensor(SensorType type) override;
    bool disableSensor(SensorType type) override;
    SensorData readSensor(SensorType type) override;
    
    // System monitoring
    SystemMetrics getSystemMetrics() override;
    void startSystemMonitoring(std::function<void(const SystemMetrics&)> callback) override;
    void stopSystemMonitoring() override;
    
    // Device control
    bool setScreenBrightness(float level) override;
    float getScreenBrightness() override;
    bool enableWiFi(bool enable) override;
    bool enableBluetooth(bool enable) override;
    
    // Audio control
    bool setSystemVolume(float level) override;
    float getSystemVolume() override;
    bool playBeep(int frequency, int duration) override;
    
private:
    std::map<int, GPIOMode> m_configuredPins;
    std::set<SensorType> m_enabledSensors;
    
    // System monitoring
    std::atomic<bool> m_systemMonitoringActive;
    std::thread m_monitoringThread;
    std::function<void(const SystemMetrics&)> m_monitoringCallback;
    
    // Helper methods
    float getCPUUsage();
    float getMemoryUsage();
    float getStorageUsage();
    float readTemperature();
    std::pair<float, bool> getBatteryInfo();
};